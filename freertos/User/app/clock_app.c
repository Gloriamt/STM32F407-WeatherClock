#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "clock_app.h"
#include "clock_ui.h"
#include "../dht11/dht11.h"
#include "../esp_at/esp_at.h"
#include "../page/clock_page.h"
#include "../rtc/weather_rtc.h"

#define WIFI_CHECK_INTERVAL_MS     5000U
#define WIFI_DISCONNECT_MISSES        2U
#define TIME_RETRY_INTERVAL_MS   10000U
#define TIME_SYNC_INTERVAL_MS  3600000U
#define RTC_INIT_RETRY_INTERVAL_MS 60000U
#define WEATHER_INTERVAL_MS     60000U
#define INDOOR_INTERVAL_MS       2000U
#define METRICS_REPORT_INTERVAL_MS 60000U
#define STACK_SAMPLE_INTERVAL_MS 10000U

extern volatile uint32_t g_system_ms;

typedef struct
{
    uint8_t wifi_connected;
    uint8_t sntp_configured;
    uint8_t esp_time_synced;
    uint8_t time_query_attempted;
    uint8_t wifi_misses;
    uint8_t current_weather_valid;
    uint8_t forecast_valid;
    uint8_t complete_weather_time_valid;
    uint8_t complete_weather_hour;
    uint8_t complete_weather_minute;
    char wifi_ssid[33];
    uint32_t last_weather_read;
    uint32_t last_wifi_check;
    uint32_t last_esp_time_try;
} clock_network_state_t;

typedef struct
{
    uint8_t initialization_complete;
    uint8_t rtc_available;
    uint8_t time_valid;
} clock_time_state_t;

/* NetworkTask owns network_state after the startup task clears it. */
static clock_network_state_t network_state;
/* TimeTask and NetworkTask access this state through the helpers below. */
static clock_time_state_t shared_time_state;
static volatile uint32_t time_min_stack_words;
static volatile uint32_t indoor_min_stack_words;
static uint32_t network_min_stack_words;
static volatile uint32_t dht_max_read_ms;
static uint32_t current_weather_max_request_ms;
static uint32_t forecast_max_request_ms;

static clock_time_state_t read_time_state(void)
{
    clock_time_state_t snapshot;

    taskENTER_CRITICAL();
    snapshot = shared_time_state;
    taskEXIT_CRITICAL();
    return snapshot;
}

static void publish_rtc_state(uint8_t rtc_available, uint8_t time_valid)
{
    taskENTER_CRITICAL();
    shared_time_state.rtc_available = rtc_available;
    shared_time_state.time_valid = time_valid;
    shared_time_state.initialization_complete = 1U;
    taskEXIT_CRITICAL();
}

static void publish_time_valid(uint8_t time_valid)
{
    taskENTER_CRITICAL();
    shared_time_state.time_valid = time_valid;
    taskEXIT_CRITICAL();
}

static void print_esp_failure(const char *operation)
{
    esp_at_diagnostics_t diagnostics;

    EspAt_GetDiagnostics(&diagnostics);
    printf("[ESP] %s failed: %s; errors=%lu timeouts=%lu "
           "response_overflows=%lu rx_overflows=%lu dropped=%lu "
           "parse_errors=%lu\r\n",
           operation, EspAt_ResultName(EspAt_LastResult()),
           (unsigned long)diagnostics.at_errors,
           (unsigned long)diagnostics.timeouts,
           (unsigned long)diagnostics.response_overflows,
           (unsigned long)diagnostics.rx_queue_overflows,
           (unsigned long)diagnostics.rx_dropped_bytes,
           (unsigned long)diagnostics.parse_errors);
}

static void report_runtime_metrics(void)
{
    clock_ui_diagnostics_t ui;
    esp_at_diagnostics_t esp;
    uint32_t stack_words;

    stack_words = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
    if (network_min_stack_words == 0U ||
        stack_words < network_min_stack_words)
    {
        network_min_stack_words = stack_words;
    }
    ClockUi_GetDiagnostics(&ui);
    EspAt_GetDiagnostics(&esp);
    printf("[METRICS] stack words ui=%lu time=%lu network=%lu indoor=%lu; "
           "max ms ui=%lu dht=%lu current=%lu forecast=%lu\r\n",
           (unsigned long)ui.min_stack_words,
           (unsigned long)time_min_stack_words,
           (unsigned long)network_min_stack_words,
           (unsigned long)indoor_min_stack_words,
           (unsigned long)ui.max_draw_ms,
           (unsigned long)dht_max_read_ms,
           (unsigned long)current_weather_max_request_ms,
           (unsigned long)forecast_max_request_ms);
    printf("[METRICS] ESP response_overflows=%lu rx_overflows=%lu "
           "dropped=%lu parse_errors=%lu\r\n",
           (unsigned long)esp.response_overflows,
           (unsigned long)esp.rx_queue_overflows,
           (unsigned long)esp.rx_dropped_bytes,
           (unsigned long)esp.parse_errors);
    printf("[METRICS] heap bytes free=%lu minimum=%lu\r\n",
           (unsigned long)xPortGetFreeHeapSize(),
           (unsigned long)xPortGetMinimumEverFreeHeapSize());
}

static void update_wifi(clock_network_state_t *state)
{
    char observed_ssid[33];
    clock_time_state_t time_state;
    weather_rtc_time_t offline_time;

    if ((g_system_ms - state->last_wifi_check) < WIFI_CHECK_INTERVAL_MS)
        return;

    state->last_wifi_check = g_system_ms;
    if (EspAt_IsWifiConnected(observed_ssid, sizeof(observed_ssid)))
    {
        state->wifi_misses = 0U;
        if (!state->wifi_connected || strcmp(state->wifi_ssid, observed_ssid) != 0)
        {
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
        if (state->current_weather_valid || state->forecast_valid)
            printf("[WEATHER] offline; keeping last successful data\r\n");
        if (state->complete_weather_time_valid)
        {
            ClockUi_PostWeatherUpdatedAt(state->complete_weather_hour,
                                         state->complete_weather_minute);
        }
        time_state = read_time_state();
        if (time_state.time_valid)
        {
            WeatherRtc_Get(&offline_time);
            printf("[RTC] offline time %02u-%02u-%02u %02u:%02u:%02u\r\n",
                   offline_time.year, offline_time.month, offline_time.day,
                   offline_time.hour, offline_time.minute,
                   offline_time.second);
        }
        printf("[WIFI] disconnected\r\n");
    }
}

static void update_network_time(clock_network_state_t *state)
{
    clock_time_state_t time_state;
    weather_rtc_time_t network_time;
    uint32_t sync_interval;

    sync_interval = state->esp_time_synced ? TIME_SYNC_INTERVAL_MS :
                                              TIME_RETRY_INTERVAL_MS;
    if (!state->wifi_connected ||
        (g_system_ms - state->last_esp_time_try) < sync_interval)
        return;

    state->last_esp_time_try = g_system_ms;
    if (!state->sntp_configured)
    {
        state->sntp_configured = EspAt_ConfigureSntp();
        if (state->sntp_configured)
            printf("[SNTP] configured\r\n");
        else
            print_esp_failure("SNTP config");
        if (state->sntp_configured)
            state->last_esp_time_try = g_system_ms - TIME_RETRY_INTERVAL_MS;
        else
            state->esp_time_synced = 0U;
        return;
    }

    time_state = read_time_state();
    if (!time_state.rtc_available)
    {
        if (!state->time_query_attempted)
        {
            printf("[RTC] unavailable; deferring network time query\r\n");
        }
        state->time_query_attempted = 1U;
        state->esp_time_synced = 0U;
        return;
    }

    state->time_query_attempted = 1U;
    if (EspAt_RequestTime(&network_time))
    {
        if (WeatherRtc_Set(&network_time))
        {
            state->esp_time_synced = 1U;
            publish_time_valid(1U);
            ClockUi_PostTime(&network_time);
            printf("[SNTP] RTC updated\r\n");
        }
        else
        {
            state->esp_time_synced = 0U;
            time_state.time_valid = WeatherRtc_IsTimeValid();
            publish_time_valid(time_state.time_valid);
            if (time_state.time_valid)
            {
                printf("[RTC] update failed; previous time restored\r\n");
            }
            else
            {
                ClockUi_PostClearTime();
                printf("[RTC] update and recovery failed; time invalid\r\n");
            }
        }
    }
    else
    {
        state->esp_time_synced = 0U;
        print_esp_failure("SNTP time");
        printf("[SNTP] response: %s\r\n", EspAt_LastTimeResponse());
    }
}

static void update_weather(clock_network_state_t *state)
{
    clock_time_state_t time_state;
    esp_weather_t weather;
    weather_rtc_time_t updated_time;
    uint8_t current_updated = 0U;
    uint8_t forecast_updated = 0U;
    uint8_t request_ok;
    uint32_t request_elapsed;
    uint32_t request_started;

    if (!state->wifi_connected || !state->time_query_attempted ||
        (g_system_ms - state->last_weather_read) < WEATHER_INTERVAL_MS)
        return;

    state->last_weather_read = g_system_ms;
    request_started = g_system_ms;
    request_ok = EspAt_RequestWeather(&weather);
    request_elapsed = g_system_ms - request_started;
    if (request_elapsed > current_weather_max_request_ms)
        current_weather_max_request_ms = request_elapsed;
    if (request_ok)
    {
        state->current_weather_valid = 1U;
        current_updated = 1U;
        ClockUi_PostCurrentWeather(weather.temperature, weather.code);
        printf("[WEATHER] current temperature=%d code=%u\r\n",
               weather.temperature, weather.code);
    }
    else
    {
        print_esp_failure("current weather");
        printf("[WEATHER] current request failed; keeping previous data\r\n");
    }

    request_started = g_system_ms;
    request_ok = EspAt_RequestForecast(&weather);
    request_elapsed = g_system_ms - request_started;
    if (request_elapsed > forecast_max_request_ms)
        forecast_max_request_ms = request_elapsed;
    if (request_ok)
    {
        state->forecast_valid = 1U;
        forecast_updated = 1U;
        ClockUi_PostForecast(weather.high, weather.low);
        printf("[WEATHER] forecast high=%d low=%d\r\n",
               weather.high, weather.low);
    }
    else
    {
        print_esp_failure("forecast");
        printf("[WEATHER] forecast request failed; keeping previous data\r\n");
    }

    time_state = read_time_state();
    if (current_updated && forecast_updated && time_state.time_valid)
    {
        WeatherRtc_Get(&updated_time);
        state->complete_weather_hour = updated_time.hour;
        state->complete_weather_minute = updated_time.minute;
        state->complete_weather_time_valid = 1U;
        ClockUi_PostClearWeatherUpdateTime();
    }
}

void ClockApp_RunStartupStage(void)
{
    memset(&network_state, 0, sizeof(network_state));
    memset(&shared_time_state, 0, sizeof(shared_time_state));
    time_min_stack_words = 0U;
    indoor_min_stack_words = 0U;
    network_min_stack_words = 0U;
    dht_max_read_ms = 0U;
    current_weather_max_request_ms = 0U;
    forecast_max_request_ms = 0U;

    ClockPage_Init();
    ClockPage_ShowMain(NULL);
    EspAt_Init();
    printf("[RTOS] main page ready; Wi-Fi connecting in background\r\n");
}

void ClockApp_TimeTask(void *argument)
{
    clock_time_state_t time_state;
    weather_rtc_time_t rtc_time;
    uint8_t rtc_available;
    uint8_t time_valid;
    uint8_t displayed_second = 0xFFU;
    uint32_t last_rtc_init_try;
    uint32_t last_stack_sample = 0U;
    uint32_t stack_words;

    (void)argument;

    rtc_available = WeatherRtc_Init();
    time_valid = rtc_available ? WeatherRtc_IsTimeValid() : 0U;
    publish_rtc_state(rtc_available, time_valid);
    last_rtc_init_try = g_system_ms;
    time_min_stack_words =
        (uint32_t)uxTaskGetStackHighWaterMark(NULL);
    if (!rtc_available)
    {
        printf("[RTC] initialization failed\r\n");
    }
    else if (time_valid)
    {
        WeatherRtc_Get(&rtc_time);
        printf("[RTC] valid saved time %02u-%02u-%02u %02u:%02u:%02u\r\n",
               rtc_time.year, rtc_time.month, rtc_time.day,
               rtc_time.hour, rtc_time.minute, rtc_time.second);
    }
    else
    {
        printf("[RTC] waiting for first valid network time\r\n");
    }

    for (;;)
    {
        time_state = read_time_state();
        if (!time_state.rtc_available &&
            (g_system_ms - last_rtc_init_try) >= RTC_INIT_RETRY_INTERVAL_MS)
        {
            last_rtc_init_try = g_system_ms;
            rtc_available = WeatherRtc_Init();
            time_valid = rtc_available ? WeatherRtc_IsTimeValid() : 0U;
            if (rtc_available)
            {
                publish_rtc_state(rtc_available, time_valid);
                printf("[RTC] initialization recovered\r\n");
            }
            else
            {
                printf("[RTC] initialization retry failed\r\n");
            }
            time_state = read_time_state();
        }
        if (time_state.rtc_available && time_state.time_valid)
        {
            WeatherRtc_Get(&rtc_time);
            if (rtc_time.second != displayed_second)
            {
                displayed_second = rtc_time.second;
                ClockUi_PostTime(&rtc_time);
            }
        }
        if ((g_system_ms - last_stack_sample) >= STACK_SAMPLE_INTERVAL_MS)
        {
            last_stack_sample = g_system_ms;
            stack_words = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
            if (stack_words < time_min_stack_words)
                time_min_stack_words = stack_words;
        }
        vTaskDelay(pdMS_TO_TICKS(50U));
    }
}

void ClockApp_NetworkTask(void *argument)
{
    clock_time_state_t time_state;
    uint32_t last_metrics_report;

    (void)argument;

    do
    {
        time_state = read_time_state();
        if (!time_state.initialization_complete)
            vTaskDelay(pdMS_TO_TICKS(10U));
    } while (!time_state.initialization_complete);

    network_state.last_esp_time_try = g_system_ms - TIME_RETRY_INTERVAL_MS;
    network_state.last_weather_read = g_system_ms - WEATHER_INTERVAL_MS;
    network_state.last_wifi_check = g_system_ms - WIFI_CHECK_INTERVAL_MS;
    last_metrics_report = g_system_ms;

    for (;;)
    {
        update_wifi(&network_state);
        update_network_time(&network_state);
        update_weather(&network_state);
        if ((g_system_ms - last_metrics_report) >=
            METRICS_REPORT_INTERVAL_MS)
        {
            last_metrics_report = g_system_ms;
            report_runtime_metrics();
        }
        vTaskDelay(pdMS_TO_TICKS(50U));
    }
}

void ClockApp_IndoorTask(void *argument)
{
    uint8_t temperature;
    uint8_t humidity;
    uint8_t read_ok;
    TickType_t last_wake_time;
    uint32_t read_elapsed;
    uint32_t read_started;
    uint32_t stack_words;

    (void)argument;
    DHT11_Init();
    last_wake_time = xTaskGetTickCount();
    indoor_min_stack_words =
        (uint32_t)uxTaskGetStackHighWaterMark(NULL);

    for (;;)
    {
        /* DHT11 has microsecond pulse widths; prevent another task from
           preempting the transaction while leaving interrupts enabled. */
        read_started = g_system_ms;
        vTaskSuspendAll();
        read_ok = DHT11_Read(&temperature, &humidity);
        (void)xTaskResumeAll();
        read_elapsed = g_system_ms - read_started;
        if (read_elapsed > dht_max_read_ms)
            dht_max_read_ms = read_elapsed;
        if (read_ok)
            ClockUi_PostIndoor(temperature, humidity);

        stack_words = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
        if (stack_words < indoor_min_stack_words)
            indoor_min_stack_words = stack_words;

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(INDOOR_INTERVAL_MS));
    }
}
