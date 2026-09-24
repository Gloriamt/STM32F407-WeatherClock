#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <string.h>
#include "clock_ui.h"
#include "../page/clock_page.h"

#define CLOCK_UI_DIRTY_TIME                 (1U << 0)
#define CLOCK_UI_DIRTY_INDOOR               (1U << 1)
#define CLOCK_UI_DIRTY_CURRENT_WEATHER      (1U << 2)
#define CLOCK_UI_DIRTY_FORECAST             (1U << 3)
#define CLOCK_UI_DIRTY_WEATHER_UPDATED_AT   (1U << 4)
#define CLOCK_UI_DIRTY_WIFI_NAME            (1U << 5)
#define CLOCK_UI_DIRTY_CLEAR_WEATHER        (1U << 6)

typedef struct
{
    uint16_t dirty;
    uint8_t time_visible;
    weather_rtc_time_t time;
    struct
    {
        uint8_t temperature;
        uint8_t humidity;
    } indoor;
    struct
    {
        int16_t temperature;
        uint8_t code;
    } current_weather;
    struct
    {
        int16_t high;
        int16_t low;
    } forecast;
    uint8_t weather_updated_at_visible;
    struct
    {
        uint8_t hour;
        uint8_t minute;
    } weather_updated_at;
    char wifi_ssid[33];
} clock_ui_pending_t;

static QueueHandle_t ui_wake_queue;
static clock_ui_pending_t ui_pending;

static void clock_ui_wake(void)
{
    uint8_t signal = 1U;

    (void)xQueueOverwrite(ui_wake_queue, &signal);
}

static void clock_ui_task(void *argument)
{
    clock_ui_pending_t snapshot;
    uint8_t signal;

    (void)argument;
    for (;;)
    {
        if (xQueueReceive(ui_wake_queue, &signal, portMAX_DELAY) != pdPASS)
            continue;

        taskENTER_CRITICAL();
        snapshot = ui_pending;
        ui_pending.dirty = 0U;
        taskEXIT_CRITICAL();

        if ((snapshot.dirty & CLOCK_UI_DIRTY_CLEAR_WEATHER) != 0U)
            ClockPage_ClearWeather();

        if ((snapshot.dirty & CLOCK_UI_DIRTY_TIME) != 0U)
        {
            if (snapshot.time_visible)
                ClockPage_UpdateTime(&snapshot.time);
            else
                ClockPage_ClearTime();
        }

        if ((snapshot.dirty & CLOCK_UI_DIRTY_INDOOR) != 0U)
        {
            ClockPage_UpdateIndoor(snapshot.indoor.temperature,
                                   snapshot.indoor.humidity);
        }

        if ((snapshot.dirty & CLOCK_UI_DIRTY_CURRENT_WEATHER) != 0U)
        {
            ClockPage_UpdateCurrentWeather(
                snapshot.current_weather.temperature,
                snapshot.current_weather.code);
        }

        if ((snapshot.dirty & CLOCK_UI_DIRTY_FORECAST) != 0U)
        {
            ClockPage_UpdateForecast(snapshot.forecast.high,
                                     snapshot.forecast.low);
        }

        if ((snapshot.dirty & CLOCK_UI_DIRTY_WEATHER_UPDATED_AT) != 0U)
        {
            if (snapshot.weather_updated_at_visible)
            {
                ClockPage_UpdateWeatherTime(
                    snapshot.weather_updated_at.hour,
                    snapshot.weather_updated_at.minute);
            }
            else
            {
                ClockPage_ClearWeatherTime();
            }
        }

        if ((snapshot.dirty & CLOCK_UI_DIRTY_WIFI_NAME) != 0U)
        {
            ClockPage_UpdateWifiName(snapshot.wifi_ssid[0] != 0 ?
                                     snapshot.wifi_ssid : NULL);
        }
    }
}

uint8_t ClockUi_Init(void)
{
    memset(&ui_pending, 0, sizeof(ui_pending));
    ui_wake_queue = xQueueCreate(1U, sizeof(uint8_t));
    if (ui_wake_queue == NULL)
        return 0U;

    if (xTaskCreate(clock_ui_task, "ui", 1024U, NULL, 2U, NULL) != pdPASS)
        return 0U;

    return 1U;
}

void ClockUi_PostTime(const weather_rtc_time_t *time)
{
    if (time == NULL)
        return;

    taskENTER_CRITICAL();
    ui_pending.time = *time;
    ui_pending.time_visible = 1U;
    ui_pending.dirty |= CLOCK_UI_DIRTY_TIME;
    taskEXIT_CRITICAL();
    clock_ui_wake();
}

void ClockUi_PostClearTime(void)
{
    taskENTER_CRITICAL();
    ui_pending.time_visible = 0U;
    ui_pending.dirty |= CLOCK_UI_DIRTY_TIME;
    taskEXIT_CRITICAL();
    clock_ui_wake();
}

void ClockUi_PostIndoor(uint8_t temperature, uint8_t humidity)
{
    taskENTER_CRITICAL();
    ui_pending.indoor.temperature = temperature;
    ui_pending.indoor.humidity = humidity;
    ui_pending.dirty |= CLOCK_UI_DIRTY_INDOOR;
    taskEXIT_CRITICAL();
    clock_ui_wake();
}

void ClockUi_PostCurrentWeather(int16_t temperature, uint8_t code)
{
    taskENTER_CRITICAL();
    ui_pending.current_weather.temperature = temperature;
    ui_pending.current_weather.code = code;
    ui_pending.dirty |= CLOCK_UI_DIRTY_CURRENT_WEATHER;
    taskEXIT_CRITICAL();
    clock_ui_wake();
}

void ClockUi_PostForecast(int16_t high, int16_t low)
{
    taskENTER_CRITICAL();
    ui_pending.forecast.high = high;
    ui_pending.forecast.low = low;
    ui_pending.dirty |= CLOCK_UI_DIRTY_FORECAST;
    taskEXIT_CRITICAL();
    clock_ui_wake();
}

void ClockUi_PostWeatherUpdatedAt(uint8_t hour, uint8_t minute)
{
    taskENTER_CRITICAL();
    ui_pending.weather_updated_at.hour = hour;
    ui_pending.weather_updated_at.minute = minute;
    ui_pending.weather_updated_at_visible = 1U;
    ui_pending.dirty |= CLOCK_UI_DIRTY_WEATHER_UPDATED_AT;
    taskEXIT_CRITICAL();
    clock_ui_wake();
}

void ClockUi_PostClearWeatherUpdateTime(void)
{
    taskENTER_CRITICAL();
    ui_pending.weather_updated_at_visible = 0U;
    ui_pending.dirty |= CLOCK_UI_DIRTY_WEATHER_UPDATED_AT;
    taskEXIT_CRITICAL();
    clock_ui_wake();
}

void ClockUi_PostWifiName(const char *ssid)
{
    taskENTER_CRITICAL();
    if (ssid == NULL)
    {
        ui_pending.wifi_ssid[0] = 0;
    }
    else
    {
        strncpy(ui_pending.wifi_ssid, ssid,
                sizeof(ui_pending.wifi_ssid) - 1U);
        ui_pending.wifi_ssid[sizeof(ui_pending.wifi_ssid) - 1U] = 0;
    }
    ui_pending.dirty |= CLOCK_UI_DIRTY_WIFI_NAME;
    taskEXIT_CRITICAL();
    clock_ui_wake();
}

void ClockUi_PostClearWeather(void)
{
    taskENTER_CRITICAL();
    ui_pending.dirty &= (uint16_t)~(CLOCK_UI_DIRTY_CURRENT_WEATHER |
                                    CLOCK_UI_DIRTY_FORECAST);
    ui_pending.dirty |= CLOCK_UI_DIRTY_CLEAR_WEATHER;
    taskEXIT_CRITICAL();
    clock_ui_wake();
}
