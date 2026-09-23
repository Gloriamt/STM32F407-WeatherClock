#include "clock_page.h"
#include "../lcd/bsp_nt35510_lcd.h"
#include "../font/weather_font.h"
#include "../touch/palette.h"

#define PAGE_BACKGROUND_RGB565  0xD73EU

static const uint8_t text_startup[] = { WEATHER_GLYPH_SMART, WEATHER_GLYPH_CAN, WEATHER_GLYPH_WEATHER, WEATHER_GLYPH_AIR, WEATHER_GLYPH_TIME, WEATHER_GLYPH_CLOCK };
static const uint8_t text_wifi_wait[] = { WEATHER_GLYPH_CONNECT, WEATHER_GLYPH_LINK, WEATHER_GLYPH_IN_PROGRESS };
static const uint8_t text_wifi_success[] = { WEATHER_GLYPH_CONNECT, WEATHER_GLYPH_LINK, WEATHER_GLYPH_SUCCESS, WEATHER_GLYPH_MERIT };
static const uint8_t text_wifi_failed[] = { WEATHER_GLYPH_CONNECT, WEATHER_GLYPH_LINK, WEATHER_GLYPH_FAILURE, WEATHER_GLYPH_DEFEAT };

static void draw_text(uint16_t x, uint16_t y, uint16_t width,
                      uint16_t height, char *text)
{
    NT35510_DisplayStringEx(x, y, width, height, (uint8_t *)text, 0);
}

static void draw_wifi_status(const uint8_t *glyphs, uint16_t count)
{
    LCD_SetColors(PAGE_BACKGROUND_RGB565, PAGE_BACKGROUND_RGB565);
    NT35510_Clear(192, 382, 180, 40);
    WeatherFont_DrawText24(196, 390, glyphs, count,
                           CL_BLACK, PAGE_BACKGROUND_RGB565);
    LCD_SetColors(CL_BLACK, PAGE_BACKGROUND_RGB565);
    if (count == 3U)
        draw_text(268, 390, 24, 24, "...");
    else
        draw_text(292, 390, 24, 24, "!");
}

static void draw_startup_page(void)
{
    LCD_SetColors(PAGE_BACKGROUND_RGB565, PAGE_BACKGROUND_RGB565);
    NT35510_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);
    LCD_SetColors(CL_BLACK, PAGE_BACKGROUND_RGB565);
    draw_text(120, 330, 24, 24, "FreeRTOS");
    WeatherFont_DrawText24(216, 330, text_startup, 6,
                           CL_BLACK, PAGE_BACKGROUND_RGB565);
    draw_text(136, 390, 24, 24, "WIFI");
    draw_wifi_status(text_wifi_wait, 3);
}

void ClockPage_Init(void)
{
    NT35510_Init();
    NT35510_GramScan(0);
}

void ClockPage_ShowStartup(void)
{
    draw_startup_page();
}

void ClockPage_ShowWifiResult(uint8_t connected)
{
    if (connected)
        draw_wifi_status(text_wifi_success, 4U);
    else
        draw_wifi_status(text_wifi_failed, 4U);
}
