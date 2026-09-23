#include "icon_assets.h"

/* Original Image2LCD arrays have an 8-byte header; weather/ arrays are resized pixels only. */
#include "icon_wifi.c"
#include "icon_location.c"
#include "icon_temprature.c"
#include "icon_humidity.c"
#include "icon_sunny.c"
#include "weather/icon_baoxue.c"
#include "weather/icon_baoyu.c"
#include "weather/icon_dafeng.c"
#include "weather/icon_daxue.c"
#include "weather/icon_dayu.c"
#include "weather/icon_duoyun.c"
#include "weather/icon_leizhenyu.c"
#include "weather/icon_mai.c"
#include "weather/icon_shachen.c"
#include "weather/icon_wu.c"
#include "weather/icon_xiaoxue.c"
#include "weather/icon_xiaoyu.c"
#include "weather/icon_yin.c"
#include "weather/icon_yujiaxue.c"
#include "weather/icon_zhenyu.c"
#include "weather/icon_zhongxue.c"
#include "weather/icon_zhongyu.c"

const weather_icon_t icon_wifi = { 74, 64, gImage_icon_wifi + 8 };
const weather_icon_t icon_location = { 57, 67, gImage_icon_location + 8 };
const weather_icon_t icon_temperature = { 57, 108, gImage_icon_temprature + 8 };
const weather_icon_t icon_humidity = { 166, 198, gImage_icon_humidity + 8 };
const weather_icon_t icon_sunny = { 158, 144, gImage_sunny + 8 };
const weather_icon_t icon_cloudy = { 110, 110, gWeather_duoyun };
const weather_icon_t icon_overcast = { 110, 110, gWeather_yin };
const weather_icon_t icon_shower = { 110, 110, gWeather_zhenyu };
const weather_icon_t icon_thunderstorm = { 110, 110, gWeather_leizhenyu };
const weather_icon_t icon_light_rain = { 110, 110, gWeather_xiaoyu };
const weather_icon_t icon_moderate_rain = { 110, 110, gWeather_zhongyu };
const weather_icon_t icon_heavy_rain = { 110, 110, gWeather_dayu };
const weather_icon_t icon_storm = { 110, 110, gWeather_baoyu };
const weather_icon_t icon_sleet = { 110, 110, gWeather_yujiaxue };
const weather_icon_t icon_light_snow = { 110, 110, gWeather_xiaoxue };
const weather_icon_t icon_moderate_snow = { 110, 110, gWeather_zhongxue };
const weather_icon_t icon_heavy_snow = { 110, 110, gWeather_daxue };
const weather_icon_t icon_snowstorm = { 110, 110, gWeather_baoxue };
const weather_icon_t icon_dust = { 110, 110, gWeather_shachen };
const weather_icon_t icon_fog = { 110, 110, gWeather_wu };
const weather_icon_t icon_haze = { 110, 110, gWeather_mai };
const weather_icon_t icon_wind = { 110, 110, gWeather_dafeng };
