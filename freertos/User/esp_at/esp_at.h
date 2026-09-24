#ifndef ESP_AT_H
#define ESP_AT_H

#include "../rtc/weather_rtc.h"
#include "weather_parser.h"

void EspAt_Init(void);
uint8_t EspAt_IsWifiConnected(char *ssid, uint8_t ssid_size);
uint8_t EspAt_ConfigureSntp(void);
uint8_t EspAt_SyncRtcFromSntp(weather_rtc_time_t *time);
uint8_t EspAt_RequestTime(weather_rtc_time_t *time);
const char *EspAt_LastTimeResponse(void);
uint8_t EspAt_RequestWeather(esp_weather_t *weather);
uint8_t EspAt_RequestForecast(esp_weather_t *weather);

#endif
