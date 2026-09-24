#ifndef WEATHER_CLOCK_PAGE_H
#define WEATHER_CLOCK_PAGE_H

#include "../rtc/weather_rtc.h"
#include "../esp_at/esp_at.h"

void ClockPage_Init(void);
void ClockPage_ShowStartup(void);
void ClockPage_ShowWifiResult(uint8_t connected);
void ClockPage_ShowMain(const char *wifi_ssid);
void ClockPage_UpdateWifiName(const char *wifi_ssid);
void ClockPage_ClearWeather(void);
void ClockPage_ClearTime(void);
void ClockPage_UpdateTime(const weather_rtc_time_t *time);
void ClockPage_UpdateIndoor(uint8_t temperature, uint8_t humidity);
void ClockPage_UpdateCurrentWeather(int16_t temperature, uint8_t code);
void ClockPage_UpdateForecast(int16_t high, int16_t low);

#endif
