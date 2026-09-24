#ifndef WEATHER_PARSER_H
#define WEATHER_PARSER_H

#include <stdint.h>

typedef struct
{
    int16_t temperature;
    int16_t high;
    int16_t low;
    uint8_t code;
} esp_weather_t;

uint8_t WeatherParser_ParseCurrent(const char *response,
                                   esp_weather_t *weather);
uint8_t WeatherParser_ParseForecast(const char *response,
                                    esp_weather_t *weather);

#endif
