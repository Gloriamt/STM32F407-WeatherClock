#include "./lcd/bsp_nt35510_lcd.h"
#include "./font/weather_font.h"

static void weather_font_draw(uint16_t x, uint16_t y, const uint8_t *glyphs,
                              uint16_t count, uint16_t glyph_height,
                              uint16_t glyph_count, const uint8_t *font_data,
                              uint16_t text_color, uint16_t back_color)
{
    uint16_t row;
    uint16_t column;
    uint32_t width;

    if ((glyphs == 0) || (count == 0) || (font_data == 0))
    {
        return;
    }

    width = (uint32_t)count * WEATHER_FONT_GLYPH_WIDTH;
    if ((width > LCD_X_LENGTH) ||
        ((uint32_t)x + width > LCD_X_LENGTH) ||
        ((uint32_t)y + glyph_height > LCD_Y_LENGTH))
    {
        return;
    }

    NT35510_OpenWindow(x, y, (uint16_t)width, glyph_height);
    *(volatile uint16_t *)FSMC_Addr_NT35510_CMD = CMD_SetPixel;

    for (row = 0; row < glyph_height; row++)
    {
        uint16_t glyph_index;
        for (glyph_index = 0; glyph_index < count; glyph_index++)
        {
            const uint8_t *glyph = 0;

            if (glyphs[glyph_index] < glyph_count)
            {
                glyph = font_data + (uint32_t)glyphs[glyph_index] *
                        glyph_height * 3U;
            }

            for (column = 0; column < WEATHER_FONT_GLYPH_WIDTH; column++)
            {
                uint16_t color = back_color;

                if ((glyph != 0) &&
                    (glyph[row * 3U + column / 8U] &
                     (0x80U >> (column % 8U))))
                {
                    color = text_color;
                }

                *(volatile uint16_t *)FSMC_Addr_NT35510_DATA = color;
            }
        }
    }
}

void WeatherFont_DrawText20(uint16_t x, uint16_t y, const uint8_t *glyphs,
                            uint16_t count, uint16_t text_color,
                            uint16_t back_color)
{
    weather_font_draw(x, y, glyphs, count, WEATHER_FONT_20_HEIGHT,
                      WEATHER_FONT_20_GLYPH_COUNT,
                      &weather_font_24x20[0][0], text_color, back_color);
}

void WeatherFont_DrawText24(uint16_t x, uint16_t y, const uint8_t *glyphs,
                            uint16_t count, uint16_t text_color,
                            uint16_t back_color)
{
    weather_font_draw(x, y, glyphs, count, WEATHER_FONT_24_HEIGHT,
                      WEATHER_FONT_24_GLYPH_COUNT,
                      &weather_font_24x24[0][0], text_color, back_color);
}
