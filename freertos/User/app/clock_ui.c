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
    CLOCK_UI_INDOOR,
    CLOCK_UI_WEATHER,
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
        esp_weather_t weather;
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
            else if (message.action == CLOCK_UI_INDOOR)
            {
                ClockPage_UpdateIndoor(message.data.indoor.temperature,
                                       message.data.indoor.humidity);
            }
            else if (message.action == CLOCK_UI_WEATHER)
            {
                ClockPage_UpdateWeather(&message.data.weather);
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

void ClockUi_PostIndoor(uint8_t temperature, uint8_t humidity)
{
    clock_ui_message_t message;

    message.action = CLOCK_UI_INDOOR;
    message.data.indoor.temperature = temperature;
    message.data.indoor.humidity = humidity;
    (void)xQueueSend(ui_queue, &message, portMAX_DELAY);
}

void ClockUi_PostWeather(const esp_weather_t *weather)
{
    clock_ui_message_t message;

    message.action = CLOCK_UI_WEATHER;
    message.data.weather = *weather;
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
