#ifndef WEATHER_RTC_H
#define WEATHER_RTC_H

#include "stm32f4xx.h"

typedef struct
{
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} weather_rtc_time_t;

uint8_t WeatherRtc_Init(void);
uint8_t WeatherRtc_IsTimeValid(void);
void WeatherRtc_Get(weather_rtc_time_t *time);
uint8_t WeatherRtc_Set(const weather_rtc_time_t *time);

#endif
