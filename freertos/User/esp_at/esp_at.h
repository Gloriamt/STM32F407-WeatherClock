#ifndef ESP_AT_H
#define ESP_AT_H

#include "../rtc/weather_rtc.h"
#include "weather_parser.h"

typedef enum
{
    ESP_AT_RESULT_OK = 0,
    ESP_AT_RESULT_AT_ERROR,
    ESP_AT_RESULT_TIMEOUT,
    ESP_AT_RESULT_RESPONSE_OVERFLOW,
    ESP_AT_RESULT_RX_QUEUE_OVERFLOW,
    ESP_AT_RESULT_PARSE_ERROR
} esp_at_result_t;

typedef struct
{
    uint32_t at_errors;
    uint32_t timeouts;
    uint32_t response_overflows;
    uint32_t rx_queue_overflows;
    uint32_t rx_dropped_bytes;
    uint32_t parse_errors;
} esp_at_diagnostics_t;

void EspAt_Init(void);
uint8_t EspAt_IsWifiConnected(char *ssid, uint8_t ssid_size);
uint8_t EspAt_ConfigureSntp(void);
uint8_t EspAt_SyncRtcFromSntp(weather_rtc_time_t *time);
uint8_t EspAt_RequestTime(weather_rtc_time_t *time);
const char *EspAt_LastTimeResponse(void);
esp_at_result_t EspAt_LastResult(void);
const char *EspAt_ResultName(esp_at_result_t result);
void EspAt_GetDiagnostics(esp_at_diagnostics_t *diagnostics);
uint8_t EspAt_RequestWeather(esp_weather_t *weather);
uint8_t EspAt_RequestForecast(esp_weather_t *weather);

#endif
