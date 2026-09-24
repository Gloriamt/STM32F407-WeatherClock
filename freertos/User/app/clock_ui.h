#ifndef WEATHER_CLOCK_UI_H
#define WEATHER_CLOCK_UI_H

#include <stdint.h>
#include "../rtc/weather_rtc.h"
#include "../esp_at/esp_at.h"

uint8_t ClockUi_Init(void);
void ClockUi_PostTime(const weather_rtc_time_t *time);
void ClockUi_PostIndoor(uint8_t temperature, uint8_t humidity);
void ClockUi_PostCurrentWeather(int16_t temperature, uint8_t code);
void ClockUi_PostForecast(int16_t high, int16_t low);
void ClockUi_PostWifiName(const char *ssid);
void ClockUi_PostClearWeather(void);

#endif
