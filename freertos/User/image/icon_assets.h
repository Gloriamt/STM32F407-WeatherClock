#ifndef WEATHER_CLOCK_ICON_ASSETS_H
#define WEATHER_CLOCK_ICON_ASSETS_H

#include <stdint.h>

typedef struct
{
    uint16_t width;
    uint16_t height;
    const uint8_t *pixels;
} weather_icon_t;

extern const weather_icon_t icon_wifi;
extern const weather_icon_t icon_location;
extern const weather_icon_t icon_temperature;
extern const weather_icon_t icon_humidity;
extern const weather_icon_t icon_sunny;
extern const weather_icon_t icon_cloudy;
extern const weather_icon_t icon_overcast;
extern const weather_icon_t icon_shower;
extern const weather_icon_t icon_thunderstorm;
extern const weather_icon_t icon_light_rain;
extern const weather_icon_t icon_moderate_rain;
extern const weather_icon_t icon_heavy_rain;
extern const weather_icon_t icon_storm;
extern const weather_icon_t icon_sleet;
extern const weather_icon_t icon_light_snow;
extern const weather_icon_t icon_moderate_snow;
extern const weather_icon_t icon_heavy_snow;
extern const weather_icon_t icon_snowstorm;
extern const weather_icon_t icon_dust;
extern const weather_icon_t icon_fog;
extern const weather_icon_t icon_haze;
extern const weather_icon_t icon_wind;

#endif
