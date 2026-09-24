#include "weather_parser.h"
#include <string.h>

#define WEATHER_TEMPERATURE_MIN  (-99)
#define WEATHER_TEMPERATURE_MAX    99
#define WEATHER_CODE_MIN             0
#define WEATHER_CODE_MAX            99

static uint8_t parse_quoted_integer(const char *response, const char *prefix,
                                    int16_t minimum, int16_t maximum,
                                    int16_t *result)
{
    const char *cursor;
    int32_t magnitude = 0;
    int32_t limit;
    int32_t value;
    int32_t digit;
    uint8_t negative = 0U;
    uint8_t digits = 0U;

    if (response == NULL || prefix == NULL || result == NULL)
        return 0U;

    cursor = strstr(response, prefix);
    if (cursor == NULL)
        return 0U;
    cursor += strlen(prefix);

    if (*cursor == '-')
    {
        negative = 1U;
        cursor++;
    }

    limit = negative ? -(int32_t)minimum : (int32_t)maximum;
    if (limit < 0)
        return 0U;

    while (*cursor >= '0' && *cursor <= '9')
    {
        digit = (int32_t)(*cursor - '0');

        if (magnitude > (limit - digit) / 10)
            return 0U;
        magnitude = magnitude * 10 + digit;
        digits++;
        cursor++;
    }

    if (digits == 0U || *cursor != '"')
        return 0U;

    value = negative ? -magnitude : magnitude;
    if (value < minimum || value > maximum)
        return 0U;

    *result = (int16_t)value;
    return 1U;
}

uint8_t WeatherParser_ParseCurrent(const char *response,
                                   esp_weather_t *weather)
{
    int16_t temperature;
    int16_t code;

    if (weather == NULL ||
        !parse_quoted_integer(response, "\"temperature\":\"",
                              WEATHER_TEMPERATURE_MIN,
                              WEATHER_TEMPERATURE_MAX, &temperature) ||
        !parse_quoted_integer(response, "\"code\":\"", WEATHER_CODE_MIN,
                              WEATHER_CODE_MAX, &code))
        return 0U;

    weather->temperature = temperature;
    weather->code = (uint8_t)code;
    return 1U;
}

uint8_t WeatherParser_ParseForecast(const char *response,
                                    esp_weather_t *weather)
{
    int16_t high;
    int16_t low;

    if (weather == NULL ||
        !parse_quoted_integer(response, "\"high\":\"",
                              WEATHER_TEMPERATURE_MIN,
                              WEATHER_TEMPERATURE_MAX, &high) ||
        !parse_quoted_integer(response, "\"low\":\"",
                              WEATHER_TEMPERATURE_MIN,
                              WEATHER_TEMPERATURE_MAX, &low))
        return 0U;

    weather->high = high;
    weather->low = low;
    return 1U;
}
