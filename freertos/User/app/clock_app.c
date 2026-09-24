#include <stdio.h>
#include <string.h>
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "clock_app.h"
#include "clock_ui.h"
#include "../dht11/dht11.h"
#include "../esp_at/esp_at.h"
#include "../page/clock_page.h"
#include "../rtc/weather_rtc.h"
#include "../usart/bsp_debug_usart.h"

#define WIFI_CONNECT_TIMEOUT_MS  15000U
#define WIFI_CHECK_INTERVAL_MS     5000U
#define WIFI_DISCONNECT_MISSES        2U
#define TIME_RETRY_INTERVAL_MS   10000U
#define WEATHER_INTERVAL_MS     60000U
#define INDOOR_INTERVAL_MS       2000U
#define RTC_REFRESH_INTERVAL_MS   200U

extern volatile uint32_t g_system_ms;

typedef struct
{
    uint8_t rtc_ready;
    uint8_t wifi_connected;
    uint8_t sntp_configured;
    uint8_t esp_time_synced;
    uint8_t time_available;
    uint8_t time_query_attempted;
    uint8_t wifi_misses;
    uint8_t displayed_second;
    char wifi_ssid[33];
    uint32_t last_weather_read;
    uint32_t last_wifi_check;
    uint32_t last_esp_time_try;
    uint32_t last_dht11_read;
    uint32_t last_rtc_read;
} clock_app_state_t;

static clock_app_state_t rtos_state;
static volatile uint8_t rtos_rtc_ready;
static volatile uint8_t rtos_time_available;

static void wait_ms(uint32_t duration)
{
    vTaskDelay(pdMS_TO_TICKS(duration));
}

static void wait_for_wifi(clock_app_state_t *state)
{
    uint32_t wifi_start = g_system_ms;

    wait_ms(1500U);
    do
    {
        if (EspAt_IsWifiConnected(state->wifi_ssid, sizeof(state->wifi_ssid)))
        {
            state->wifi_connected = 1U;
            break;
        }
        if ((g_system_ms - wifi_start) < WIFI_CONNECT_TIMEOUT_MS)
            wait_ms(800U);
    } while ((g_system_ms - wifi_start) < WIFI_CONNECT_TIMEOUT_MS);

    if (state->wifi_connected)
    {
        printf("[WIFI] connected\r\n");
        ClockPage_ShowWifiResult(1U);
        state->sntp_configured = EspAt_ConfigureSntp();
        printf(state->sntp_configured ? "[SNTP] configured\r\n" :
                                        "[SNTP] config failed; retrying\r\n");
    }
    else
    {
        printf("[WIFI] timeout\r\n");
        ClockPage_ShowWifiResult(0U);
    }
    wait_ms(3000U);
}

static void update_wifi(clock_app_state_t *state)
{
    char observed_ssid[33];

    if ((g_system_ms - state->last_wifi_check) < WIFI_CHECK_INTERVAL_MS)
        return;

    state->last_wifi_check = g_system_ms;
    if (EspAt_IsWifiConnected(observed_ssid, sizeof(observed_ssid)))
    {
        state->wifi_misses = 0U;
        if (!state->wifi_connected || strcmp(state->wifi_ssid, observed_ssid) != 0)
        {
            if (state->wifi_connected)
                ClockUi_PostClearWeather();
            strcpy(state->wifi_ssid, observed_ssid);
            state->wifi_connected = 1U;
            state->sntp_configured = 0U;
            state->esp_time_synced = 0U;
            state->time_query_attempted = 0U;
            state->last_esp_time_try = g_system_ms - TIME_RETRY_INTERVAL_MS;
            state->last_weather_read = g_system_ms - WEATHER_INTERVAL_MS;
            ClockUi_PostWifiName(state->wifi_ssid);
            printf("[WIFI] connected\r\n");
        }
    }
    else if (state->wifi_connected && ++state->wifi_misses >= WIFI_DISCONNECT_MISSES)
    {
        state->wifi_connected = 0U;
        state->wifi_ssid[0] = 0;
        state->sntp_configured = 0U;
        state->esp_time_synced = 0U;
        state->time_query_attempted = 0U;
        state->wifi_misses = 0U;
        ClockUi_PostWifiName(NULL);
        ClockUi_PostClearWeather();
        printf("[WIFI] disconnected\r\n");
    }
}

static void update_network_time(clock_app_state_t *state)
{
    weather_rtc_time_t network_time;

    if (!state->wifi_connected || state->esp_time_synced ||
        (g_system_ms - state->last_esp_time_try) < TIME_RETRY_INTERVAL_MS)
        return;

    state->last_esp_time_try = g_system_ms;
    if (!state->sntp_configured)
    {
        state->sntp_configured = EspAt_ConfigureSntp();
        printf(state->sntp_configured ? "[SNTP] configured\r\n" :
                                        "[SNTP] config failed; retrying\r\n");
        if (state->sntp_configured)
            state->last_esp_time_try = g_system_ms - TIME_RETRY_INTERVAL_MS;
        return;
    }

    state->time_query_attempted = 1U;
    if (EspAt_RequestTime(&network_time))
    {
        WeatherRtc_Set(&network_time);
        state->esp_time_synced = 1U;
        state->time_available = 1U;
        state->rtc_ready = 1U;
        state->displayed_second = network_time.second;
        ClockUi_PostTime(&network_time);
        printf("[SNTP] RTC updated\r\n");
    }
    else
    {
        printf("[SNTP] no valid time; retrying\r\n");
        printf("[SNTP] response: %s\r\n", EspAt_LastTimeResponse());
    }
}

static void update_weather(clock_app_state_t *state)
{
    esp_weather_t weather;

    if (!state->wifi_connected || !state->time_query_attempted ||
        (g_system_ms - state->last_weather_read) < WEATHER_INTERVAL_MS)
        return;

    state->last_weather_read = g_system_ms;
    if (!EspAt_RequestWeather(&weather))
    {
        printf("[WEATHER] current request failed\r\n");
        return;
    }

    printf("[WEATHER] current temperature=%d code=%u\r\n",
           weather.temperature, weather.code);
    if (!EspAt_RequestForecast(&weather))
    {
        printf("[WEATHER] forecast request failed\r\n");
        return;
    }

    printf("[WEATHER] forecast high=%d low=%d\r\n",
           weather.high, weather.low);
    ClockUi_PostWeather(&weather);
}

static void update_indoor(clock_app_state_t *state)
{
    uint8_t temperature;
    uint8_t humidity;

    if ((g_system_ms - state->last_dht11_read) < INDOOR_INTERVAL_MS)
        return;

    state->last_dht11_read = g_system_ms;
    if (DHT11_Read(&temperature, &humidity))
        ClockUi_PostIndoor(temperature, humidity);
}

static void update_rtc(clock_app_state_t *state)
{
    weather_rtc_time_t rtc_time;

    if (!state->time_available ||
        !state->rtc_ready ||
        (g_system_ms - state->last_rtc_read) < RTC_REFRESH_INTERVAL_MS)
        return;

    state->last_rtc_read = g_system_ms;
    WeatherRtc_Get(&rtc_time);
    if (rtc_time.second != state->displayed_second)
    {
        state->displayed_second = rtc_time.second;
        ClockUi_PostTime(&rtc_time);
    }
}

uint8_t ClockApp_RunStartupStage(void)
{
    memset(&rtos_state, 0, sizeof(rtos_state));

    ClockPage_Init();
    ClockPage_ShowStartup();
    EspAt_Init();
    wait_for_wifi(&rtos_state);
    ClockPage_ShowMain(rtos_state.wifi_connected ? rtos_state.wifi_ssid : NULL);
    printf(rtos_state.wifi_connected ?
           "[RTOS] startup stage complete; Wi-Fi connected\r\n" :
           "[RTOS] startup stage complete; Wi-Fi unavailable\r\n");
    return rtos_state.wifi_connected;
}

void ClockApp_TimeTask(void *argument)
{
    weather_rtc_time_t rtc_time;
    uint8_t displayed_second = 0xFFU;

    (void)argument;

    rtos_rtc_ready = WeatherRtc_Init();

    for (;;)
    {
        if (rtos_rtc_ready && rtos_time_available)
        {
            WeatherRtc_Get(&rtc_time);
            if (rtc_time.second != displayed_second)
            {
                displayed_second = rtc_time.second;
                ClockUi_PostTime(&rtc_time);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50U));
    }
}

void ClockApp_NetworkTask(void *argument)
{
    (void)argument;

    while (!rtos_rtc_ready)
        vTaskDelay(pdMS_TO_TICKS(10U));

    rtos_state.rtc_ready = 1U;
    rtos_state.displayed_second = 0xFFU;
    rtos_state.last_esp_time_try = g_system_ms - TIME_RETRY_INTERVAL_MS;
    rtos_state.last_weather_read = g_system_ms - WEATHER_INTERVAL_MS;
    rtos_state.last_wifi_check = g_system_ms;

    for (;;)
    {
        update_wifi(&rtos_state);
        update_network_time(&rtos_state);
        if (rtos_state.time_available)
            rtos_time_available = 1U;
        update_weather(&rtos_state);
        vTaskDelay(pdMS_TO_TICKS(50U));
    }
}

void ClockApp_IndoorTask(void *argument)
{
    uint8_t temperature;
    uint8_t humidity;
    uint8_t read_ok;
    TickType_t last_wake_time;

    (void)argument;
    DHT11_Init();
    last_wake_time = xTaskGetTickCount();

    for (;;)
    {
        /* DHT11 has microsecond pulse widths; prevent another task from
           preempting the transaction while leaving interrupts enabled. */
        vTaskSuspendAll();
        read_ok = DHT11_Read(&temperature, &humidity);
        (void)xTaskResumeAll();
        if (read_ok)
            ClockUi_PostIndoor(temperature, humidity);

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(INDOOR_INTERVAL_MS));
    }
}

void ClockApp_Run(void)
{
    clock_app_state_t state = {0};

    ClockPage_Init();
    Debug_USART_Config();
    ClockPage_ShowStartup();
    EspAt_Init();
    wait_for_wifi(&state);

    ClockPage_ShowMain(state.wifi_connected ? state.wifi_ssid : NULL);
    DHT11_Init();
    state.rtc_ready = WeatherRtc_Init();
    state.displayed_second = 0xFFU;
    state.last_weather_read = g_system_ms - WEATHER_INTERVAL_MS;
    state.last_esp_time_try = g_system_ms - 7000U;
    state.last_wifi_check = g_system_ms;

    while (1)
    {
        update_wifi(&state);
        update_network_time(&state);
        update_weather(&state);
        update_indoor(&state);
        update_rtc(&state);
        vTaskDelay(pdMS_TO_TICKS(10U));
    }
}
