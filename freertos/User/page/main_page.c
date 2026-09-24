#include <stdio.h>
#include <string.h>
#include "clock_page.h"
#include "../lcd/bsp_nt35510_lcd.h"
#include "../lcd/nt35510_image.h"
#include "../font/weather_font.h"
#include "../touch/palette.h"

#define PAGE_BACKGROUND_RGB565  0xD73EU
#define WEATHER_PLACEHOLDER_RGB565  0x9CF4U

static const uint8_t text_indoor[] = {
    WEATHER_GLYPH_ROOM, WEATHER_GLYPH_INDOOR
};
static const uint8_t text_shanghai[] = {
    WEATHER_GLYPH_SHANG, WEATHER_GLYPH_HAI, WEATHER_GLYPH_CITY
};
static const uint8_t text_high[] = {
    WEATHER_GLYPH_MOST, WEATHER_GLYPH_HIGH
};
static const uint8_t text_low[] = {
    WEATHER_GLYPH_MOST, WEATHER_GLYPH_LOW
};
static const uint8_t text_sunny[] = { WEATHER_GLYPH_SUNNY };
static const uint8_t text_cloudy[] = { WEATHER_GLYPH_MANY, WEATHER_GLYPH_CLOUD };
static const uint8_t text_overcast[] = { WEATHER_GLYPH_OVERCAST };
static const uint8_t text_shower[] = { WEATHER_GLYPH_SHOWER, WEATHER_GLYPH_RAIN };
static const uint8_t text_thunderstorm[] = { WEATHER_GLYPH_THUNDER, WEATHER_GLYPH_SHOWER, WEATHER_GLYPH_RAIN };
static const uint8_t text_light_rain[] = { WEATHER_GLYPH_SMALL, WEATHER_GLYPH_RAIN };
static const uint8_t text_moderate_rain[] = { WEATHER_GLYPH_MEDIUM, WEATHER_GLYPH_RAIN };
static const uint8_t text_heavy_rain[] = { WEATHER_GLYPH_BIG, WEATHER_GLYPH_RAIN };
static const uint8_t text_storm[] = { WEATHER_GLYPH_STORM, WEATHER_GLYPH_RAIN };
static const uint8_t text_rain[] = { WEATHER_GLYPH_RAIN };
static const uint8_t text_sleet[] = { WEATHER_GLYPH_RAIN, WEATHER_GLYPH_MIXED, WEATHER_GLYPH_SNOW };
static const uint8_t text_snow_shower[] = { WEATHER_GLYPH_SHOWER, WEATHER_GLYPH_SNOW };
static const uint8_t text_light_snow[] = { WEATHER_GLYPH_SMALL, WEATHER_GLYPH_SNOW };
static const uint8_t text_moderate_snow[] = { WEATHER_GLYPH_MEDIUM, WEATHER_GLYPH_SNOW };
static const uint8_t text_heavy_snow[] = { WEATHER_GLYPH_BIG, WEATHER_GLYPH_SNOW };
static const uint8_t text_snowstorm[] = { WEATHER_GLYPH_STORM, WEATHER_GLYPH_SNOW };
static const uint8_t text_dust[] = { WEATHER_GLYPH_SAND, WEATHER_GLYPH_DUST };
static const uint8_t text_fog[] = { WEATHER_GLYPH_FOG };
static const uint8_t text_haze[] = { WEATHER_GLYPH_HAZE };
static const uint8_t text_wind[] = { WEATHER_GLYPH_WIND };
static const uint8_t text_gale[] = { WEATHER_GLYPH_BIG, WEATHER_GLYPH_WIND };
static const uint8_t text_week[] = {
    WEATHER_GLYPH_WEEK, WEATHER_GLYPH_PERIOD
};
static const uint8_t text_weekday[7][3] = {
    { WEATHER_GLYPH_WEEK, WEATHER_GLYPH_PERIOD, WEATHER_GLYPH_ONE },
    { WEATHER_GLYPH_WEEK, WEATHER_GLYPH_PERIOD, WEATHER_GLYPH_TWO },
    { WEATHER_GLYPH_WEEK, WEATHER_GLYPH_PERIOD, WEATHER_GLYPH_THREE },
    { WEATHER_GLYPH_WEEK, WEATHER_GLYPH_PERIOD, WEATHER_GLYPH_FOUR },
    { WEATHER_GLYPH_WEEK, WEATHER_GLYPH_PERIOD, WEATHER_GLYPH_FIVE },
    { WEATHER_GLYPH_WEEK, WEATHER_GLYPH_PERIOD, WEATHER_GLYPH_SIX },
    { WEATHER_GLYPH_WEEK, WEATHER_GLYPH_PERIOD, WEATHER_GLYPH_DAY }
};

static void draw_text(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char *text)
{
    /* In the Wildfire driver, zero makes glyph pixels use the text color. */
    NT35510_DisplayStringEx(x, y, width, height, (uint8_t *)text, 0);
}

static void draw_card(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    const uint16_t radius = 20;

    LCD_SetTextColor(CL_WHITE);
    NT35510_DrawRectangle(x + radius, y, width - 2 * radius, height, 1);
    NT35510_DrawRectangle(x, y + radius, width, height - 2 * radius, 1);
    NT35510_DrawCircle(x + radius, y + radius, radius, 1);
    NT35510_DrawCircle(x + width - radius - 1, y + radius, radius, 1);
    NT35510_DrawCircle(x + radius, y + height - radius - 1, radius, 1);
    NT35510_DrawCircle(x + width - radius - 1, y + height - radius - 1, radius, 1);
}

static void draw_chinese20(uint16_t x, uint16_t y, const uint8_t *glyphs,
                           uint16_t count)
{
    WeatherFont_DrawText20(x, y, glyphs, count, CL_BLACK, CL_WHITE);
}

static void draw_chinese24(uint16_t x, uint16_t y, const uint8_t *glyphs,
                           uint16_t count)
{
    WeatherFont_DrawText24(x, y, glyphs, count, CL_BLACK, CL_WHITE);
}

static void draw_temperature_placeholder(uint16_t x, uint16_t y,
                                         uint16_t font_size)
{
    uint16_t degree_radius = font_size / 12U;

    draw_text(x, y, font_size, font_size, "--");
    LCD_SetTextColor(CL_BLACK);
    NT35510_DrawCircle(x + font_size + degree_radius + 2U,
                       y + font_size / 4U, degree_radius, 1);
    draw_text(x + font_size + degree_radius * 2U + 6U, y,
              font_size, font_size, "C");
}

static void draw_temperature_value(uint16_t x, uint16_t y,
                                   uint16_t font_size, int16_t value)
{
    char value_text[5];
    uint16_t character_count;
    uint16_t degree_radius = font_size / 12U;

    sprintf(value_text, "%d", value);
    character_count = (uint16_t)strlen(value_text);
    draw_text(x, y, font_size, font_size, value_text);
    LCD_SetTextColor(CL_BLACK);
    NT35510_DrawCircle(x + character_count * (font_size / 2U) + degree_radius + 2U,
                       y + font_size / 4U, degree_radius, 1);
    draw_text(x + character_count * (font_size / 2U) + degree_radius * 2U + 6U,
              y, font_size, font_size, "C");
}

static void draw_indoor_values(uint8_t temperature, uint8_t humidity)
{
    char humidity_text[5];

    LCD_SetColors(CL_WHITE, CL_WHITE);
    NT35510_Clear(98, 405, 150, 62);
    NT35510_Clear(312, 405, 115, 62);

    LCD_SetColors(CL_BLACK, CL_WHITE);
    draw_temperature_value(102, 412, 50, temperature);
    sprintf(humidity_text, "%u%%", humidity);
    draw_text(316, 412, 50, 50, humidity_text);
}

static void draw_rtc_values(const weather_rtc_time_t *time)
{
    char time_text[6];
    char date_text[11];
    uint8_t weekday_index;

    sprintf(time_text, "%02u:%02u", time->hour, time->minute);
    sprintf(date_text, "20%02u/%02u/%02u", time->year, time->month, time->day);
    weekday_index = (time->weekday >= RTC_Weekday_Monday) &&
                    (time->weekday <= RTC_Weekday_Sunday) ?
                    (uint8_t)(time->weekday - RTC_Weekday_Monday) : 0U;

    LCD_SetColors(CL_WHITE, CL_WHITE);
    NT35510_Clear(112, 104, 258, 104);
    NT35510_Clear(116, 220, 152, 34);
    NT35510_Clear(282, 220, 92, 34);

    LCD_SetColors(CL_BLACK, CL_WHITE);
    draw_text(120, 112, 96, 96, time_text);
    draw_text(121, 226, 24, 24, date_text);
    draw_chinese24(286, 226, text_weekday[weekday_index], 3);
}

static void draw_wifi_name(const char *ssid)
{
    char label[19];
    uint8_t length = 0U;
    uint8_t truncated = 0U;
    uint16_t font_size;
    uint16_t x;

    if (ssid == NULL || ssid[0] == 0)
        ssid = "-----";
    while (ssid[length] != 0 && length < sizeof(label) - 1U)
    {
        label[length] = ssid[length];
        length++;
    }
    truncated = (ssid[length] != 0);
    if (truncated)
    {
        label[length - 3U] = '.';
        label[length - 2U] = '.';
        label[length - 1U] = '.';
    }
    label[length] = 0;
    font_size = (length <= 12U) ? 24U : 16U;
    x = (uint16_t)(440U - length * (font_size / 2U));
    LCD_SetColors(CL_BLACK, CL_WHITE);
    draw_text(x, (font_size == 24U) ? 40U : 44U,
              font_size, font_size, label);
}

static void draw_static_ui(const char *wifi_ssid)
{
    LCD_SetColors(PAGE_BACKGROUND_RGB565, PAGE_BACKGROUND_RGB565);
    NT35510_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);

    draw_card(16, 16, 448, 270);
    draw_card(16, 306, 448, 225);
    draw_card(16, 551, 448, 225);

    NT35510_DrawImageScaled(36, 36, 32, 24, &icon_wifi);
    draw_wifi_name(wifi_ssid);
    LCD_SetColors(CL_BLACK, CL_WHITE);
    draw_text(120, 112, 96, 96, "--:--");
    draw_text(121, 226, 24, 24, "----/--/--");
    draw_chinese24(286, 226, text_week, 2);
    draw_text(334, 226, 24, 24, "-");

    NT35510_DrawImageScaled(40, 325, 22, 30, &icon_location);
    LCD_SetColors(CL_BLACK, CL_WHITE);
    draw_chinese24(78, 328, text_indoor, 2);
    NT35510_DrawImageScaled(52, 392, 50, 85, &icon_temperature);
    draw_temperature_placeholder(102, 412, 50);
    NT35510_DrawImageScaled(262, 403, 50, 50, &icon_humidity);
    draw_text(316, 412, 50, 50, "--%");

    NT35510_DrawImageScaled(40, 571, 22, 30, &icon_location);
    LCD_SetColors(CL_BLACK, CL_WHITE);
    draw_chinese24(78, 574, text_shanghai, 3);
    draw_temperature_placeholder(66, 630, 64);
    LCD_SetTextColor(WEATHER_PLACEHOLDER_RGB565);
    NT35510_DrawRectangle(306, 590, 110, 110, 1);
    LCD_SetColors(CL_BLACK, CL_WHITE);
    draw_chinese20(66, 720, text_high, 2);
    draw_temperature_placeholder(110, 720, 20);
    draw_chinese20(176, 720, text_low, 2);
    draw_temperature_placeholder(220, 720, 20);
    draw_text(354, 720, 20, 20, "-");
}

static const weather_icon_t *weather_icon_for_code(uint8_t code,
                                                   const uint8_t **label,
                                                   uint8_t *label_length)
{
#define WEATHER_VISUAL(icon_name, text_name) \
    do { *label = text_name; *label_length = sizeof(text_name); return &icon_name; } while (0)
    switch (code)
    {
        case 0: case 1: case 2: case 3:
            WEATHER_VISUAL(icon_sunny, text_sunny);
        case 4: case 5: case 6: case 7: case 8:
            WEATHER_VISUAL(icon_cloudy, text_cloudy);
        case 9:
            WEATHER_VISUAL(icon_overcast, text_overcast);
        case 10:
            WEATHER_VISUAL(icon_shower, text_shower);
        case 11: case 12:
            WEATHER_VISUAL(icon_thunderstorm, text_thunderstorm);
        case 13:
            WEATHER_VISUAL(icon_light_rain, text_light_rain);
        case 14:
            WEATHER_VISUAL(icon_moderate_rain, text_moderate_rain);
        case 15:
            WEATHER_VISUAL(icon_heavy_rain, text_heavy_rain);
        case 16: case 17: case 18:
            WEATHER_VISUAL(icon_storm, text_storm);
        case 19: /* No freezing-rain asset or matching font glyph. */
            WEATHER_VISUAL(icon_light_rain, text_rain);
        case 20:
            WEATHER_VISUAL(icon_sleet, text_sleet);
        case 21:
            WEATHER_VISUAL(icon_light_snow, text_snow_shower);
        case 22:
            WEATHER_VISUAL(icon_light_snow, text_light_snow);
        case 23:
            WEATHER_VISUAL(icon_moderate_snow, text_moderate_snow);
        case 24:
            WEATHER_VISUAL(icon_heavy_snow, text_heavy_snow);
        case 25:
            WEATHER_VISUAL(icon_snowstorm, text_snowstorm);
        case 26: case 27: case 28: case 29:
            WEATHER_VISUAL(icon_dust, text_dust);
        case 30:
            WEATHER_VISUAL(icon_fog, text_fog);
        case 31:
            WEATHER_VISUAL(icon_haze, text_haze);
        case 32:
            WEATHER_VISUAL(icon_wind, text_wind);
        case 33: case 34: case 35: case 36:
            WEATHER_VISUAL(icon_wind, text_gale);
        default:
            *label = NULL;
            *label_length = 0U;
            return NULL;
    }
#undef WEATHER_VISUAL
}

static void draw_current_weather(int16_t temperature, uint8_t code)
{
    const weather_icon_t *icon;
    const uint8_t *label;
    uint8_t label_length;

    icon = weather_icon_for_code(code, &label, &label_length);
    LCD_SetColors(CL_WHITE, CL_WHITE);
    NT35510_Clear(58, 620, 190, 82);
    NT35510_Clear(306, 590, 110, 110);
    NT35510_Clear(306, 716, 110, 28);
    LCD_SetColors(CL_BLACK, CL_WHITE);
    draw_temperature_value(66, 630, 64, temperature);
    if (icon != NULL)
    {
        NT35510_DrawImageScaled(306, 590, 110, 110, icon);
        draw_chinese20((uint16_t)(306U + (110U - 24U * label_length) / 2U),
                       720, label, label_length);
    }
    else
    {
        LCD_SetTextColor(WEATHER_PLACEHOLDER_RGB565);
        NT35510_DrawRectangle(306, 590, 110, 110, 1);
        LCD_SetColors(CL_BLACK, CL_WHITE);
        draw_text(354, 720, 20, 20, "-");
    }
}

static void draw_forecast(int16_t high, int16_t low)
{
    LCD_SetColors(CL_WHITE, CL_WHITE);
    NT35510_Clear(108, 716, 64, 28);
    NT35510_Clear(218, 716, 64, 28);
    LCD_SetColors(CL_BLACK, CL_WHITE);
    draw_chinese20(66, 720, text_high, 2);
    draw_chinese20(176, 720, text_low, 2);
    draw_temperature_value(110, 720, 20, high);
    draw_temperature_value(220, 720, 20, low);
}

void ClockPage_ShowMain(const char *wifi_ssid)
{
    draw_static_ui(wifi_ssid);
}

void ClockPage_UpdateWifiName(const char *wifi_ssid)
{
    LCD_SetColors(CL_WHITE, CL_WHITE);
    NT35510_Clear(286, 36, 164, 36);
    draw_wifi_name(wifi_ssid);
}

void ClockPage_ClearWeather(void)
{
    LCD_SetColors(CL_WHITE, CL_WHITE);
    NT35510_Clear(58, 620, 190, 82);
    NT35510_Clear(108, 716, 64, 28);
    NT35510_Clear(218, 716, 64, 28);
    NT35510_Clear(306, 590, 110, 110);
    NT35510_Clear(306, 716, 110, 28);

    LCD_SetColors(CL_BLACK, CL_WHITE);
    draw_chinese20(66, 720, text_high, 2);
    draw_chinese20(176, 720, text_low, 2);
    draw_temperature_placeholder(66, 630, 64);
    draw_temperature_placeholder(110, 720, 20);
    draw_temperature_placeholder(220, 720, 20);
    LCD_SetTextColor(WEATHER_PLACEHOLDER_RGB565);
    NT35510_DrawRectangle(306, 590, 110, 110, 1);
    LCD_SetColors(CL_BLACK, CL_WHITE);
    draw_text(354, 720, 20, 20, "-");
}

void ClockPage_ClearTime(void)
{
    LCD_SetColors(CL_WHITE, CL_WHITE);
    NT35510_Clear(112, 104, 258, 104);
    NT35510_Clear(116, 220, 258, 34);

    LCD_SetColors(CL_BLACK, CL_WHITE);
    draw_text(120, 112, 96, 96, "--:--");
    draw_text(121, 226, 24, 24, "----/--/--");
    draw_chinese24(286, 226, text_week, 2);
    draw_text(334, 226, 24, 24, "-");
}

void ClockPage_UpdateTime(const weather_rtc_time_t *time)
{
    draw_rtc_values(time);
}

void ClockPage_UpdateIndoor(uint8_t temperature, uint8_t humidity)
{
    draw_indoor_values(temperature, humidity);
}

void ClockPage_UpdateCurrentWeather(int16_t temperature, uint8_t code)
{
    draw_current_weather(temperature, code);
}

void ClockPage_UpdateForecast(int16_t high, int16_t low)
{
    draw_forecast(high, low);
}
