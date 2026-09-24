#ifndef WEATHER_CLOCK_PAGE_H
#define WEATHER_CLOCK_PAGE_H

#include "../rtc/weather_rtc.h"

void ClockPage_Init(void);
void ClockPage_ShowMain(const char *wifi_ssid);
void ClockPage_UpdateWifiName(const char *wifi_ssid);
void ClockPage_ClearWeather(void);
void ClockPage_ClearTime(void);
void ClockPage_UpdateTime(const weather_rtc_time_t *time);
void ClockPage_UpdateIndoor(uint8_t temperature, uint8_t humidity);
void ClockPage_UpdateCurrentWeather(int16_t temperature, uint8_t code);
void ClockPage_UpdateForecast(int16_t high, int16_t low);
void ClockPage_UpdateWeatherTime(uint8_t hour, uint8_t minute);
void ClockPage_ClearWeatherTime(void);

#endif
