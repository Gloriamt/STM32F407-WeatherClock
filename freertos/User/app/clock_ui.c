#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <string.h>
#include "clock_ui.h"
#include "../page/clock_page.h"

#define CLOCK_UI_QUEUE_LENGTH  8U

typedef enum
{
    CLOCK_UI_TIME,
    CLOCK_UI_CLEAR_TIME,
    CLOCK_UI_INDOOR,
    CLOCK_UI_CURRENT_WEATHER,
    CLOCK_UI_FORECAST,
    CLOCK_UI_WEATHER_UPDATED_AT,
    CLOCK_UI_CLEAR_WEATHER_UPDATE_TIME,
    CLOCK_UI_WIFI_NAME,
    CLOCK_UI_CLEAR_WEATHER
} clock_ui_action_t;

typedef struct
{
    clock_ui_action_t action;
    union
    {
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
        struct
        {
            uint8_t hour;
            uint8_t minute;
        } weather_updated_at;
        char wifi_ssid[33];
    } data;
} clock_ui_message_t;

static QueueHandle_t ui_queue;

static void clock_ui_task(void *argument)
{
    clock_ui_message_t message;

    (void)argument;
    for (;;)
    {
        if (xQueueReceive(ui_queue, &message, portMAX_DELAY) == pdPASS)
        {
            if (message.action == CLOCK_UI_TIME)
            {
                ClockPage_UpdateTime(&message.data.time);
            }
            else if (message.action == CLOCK_UI_CLEAR_TIME)
            {
                ClockPage_ClearTime();
            }
            else if (message.action == CLOCK_UI_INDOOR)
            {
                ClockPage_UpdateIndoor(message.data.indoor.temperature,
                                       message.data.indoor.humidity);
            }
            else if (message.action == CLOCK_UI_CURRENT_WEATHER)
            {
                ClockPage_UpdateCurrentWeather(
                    message.data.current_weather.temperature,
                    message.data.current_weather.code);
            }
            else if (message.action == CLOCK_UI_FORECAST)
            {
                ClockPage_UpdateForecast(message.data.forecast.high,
                                         message.data.forecast.low);
            }
            else if (message.action == CLOCK_UI_WEATHER_UPDATED_AT)
            {
                ClockPage_UpdateWeatherTime(
                    message.data.weather_updated_at.hour,
                    message.data.weather_updated_at.minute);
            }
            else if (message.action == CLOCK_UI_CLEAR_WEATHER_UPDATE_TIME)
            {
                ClockPage_ClearWeatherTime();
            }
            else if (message.action == CLOCK_UI_WIFI_NAME)
            {
                ClockPage_UpdateWifiName(message.data.wifi_ssid[0] != 0 ?
                                         message.data.wifi_ssid : NULL);
            }
            else if (message.action == CLOCK_UI_CLEAR_WEATHER)
            {
                ClockPage_ClearWeather();
            }
        }
    }
}

uint8_t ClockUi_Init(void)
{
    ui_queue = xQueueCreate(CLOCK_UI_QUEUE_LENGTH, sizeof(clock_ui_message_t));
    if (ui_queue == NULL)
        return 0U;

    if (xTaskCreate(clock_ui_task, "ui", 1024U, NULL, 2U, NULL) != pdPASS)
        return 0U;

    return 1U;
}

void ClockUi_PostTime(const weather_rtc_time_t *time)
{
    clock_ui_message_t message;

    message.action = CLOCK_UI_TIME;
    message.data.time = *time;
    (void)xQueueSend(ui_queue, &message, portMAX_DELAY);
}

void ClockUi_PostClearTime(void)
{
    clock_ui_message_t message;

    message.action = CLOCK_UI_CLEAR_TIME;
    (void)xQueueSend(ui_queue, &message, portMAX_DELAY);
}

void ClockUi_PostIndoor(uint8_t temperature, uint8_t humidity)
{
    clock_ui_message_t message;

    message.action = CLOCK_UI_INDOOR;
    message.data.indoor.temperature = temperature;
    message.data.indoor.humidity = humidity;
    (void)xQueueSend(ui_queue, &message, portMAX_DELAY);
}

void ClockUi_PostCurrentWeather(int16_t temperature, uint8_t code)
{
    clock_ui_message_t message;

    message.action = CLOCK_UI_CURRENT_WEATHER;
    message.data.current_weather.temperature = temperature;
    message.data.current_weather.code = code;
    (void)xQueueSend(ui_queue, &message, portMAX_DELAY);
}

void ClockUi_PostForecast(int16_t high, int16_t low)
{
    clock_ui_message_t message;

    message.action = CLOCK_UI_FORECAST;
    message.data.forecast.high = high;
    message.data.forecast.low = low;
    (void)xQueueSend(ui_queue, &message, portMAX_DELAY);
}

void ClockUi_PostWeatherUpdatedAt(uint8_t hour, uint8_t minute)
{
    clock_ui_message_t message;

    message.action = CLOCK_UI_WEATHER_UPDATED_AT;
    message.data.weather_updated_at.hour = hour;
    message.data.weather_updated_at.minute = minute;
    (void)xQueueSend(ui_queue, &message, portMAX_DELAY);
}

void ClockUi_PostClearWeatherUpdateTime(void)
{
    clock_ui_message_t message;

    message.action = CLOCK_UI_CLEAR_WEATHER_UPDATE_TIME;
    (void)xQueueSend(ui_queue, &message, portMAX_DELAY);
}

void ClockUi_PostWifiName(const char *ssid)
{
    clock_ui_message_t message;

    message.action = CLOCK_UI_WIFI_NAME;
    if (ssid == NULL)
    {
        message.data.wifi_ssid[0] = 0;
    }
    else
    {
        strncpy(message.data.wifi_ssid, ssid,
                sizeof(message.data.wifi_ssid) - 1U);
        message.data.wifi_ssid[sizeof(message.data.wifi_ssid) - 1U] = 0;
    }
    (void)xQueueSend(ui_queue, &message, portMAX_DELAY);
}

void ClockUi_PostClearWeather(void)
{
    clock_ui_message_t message;

    message.action = CLOCK_UI_CLEAR_WEATHER;
    (void)xQueueSend(ui_queue, &message, portMAX_DELAY);
}
