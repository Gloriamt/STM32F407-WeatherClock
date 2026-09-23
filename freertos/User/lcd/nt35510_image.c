#include "./lcd/bsp_nt35510_lcd.h"
#include "./lcd/nt35510_image.h"

void NT35510_DrawImageScaled(uint16_t x, uint16_t y, uint16_t width,
                             uint16_t height, const weather_icon_t *icon)
{
    uint16_t row;
    uint16_t column;

    if ((icon == 0) || (width == 0) || (height == 0) ||
        ((uint32_t)x + width > LCD_X_LENGTH) ||
        ((uint32_t)y + height > LCD_Y_LENGTH))
    {
        return;
    }

    NT35510_OpenWindow(x, y, width, height);
    *(volatile uint16_t *)FSMC_Addr_NT35510_CMD = CMD_SetPixel;
    for (row = 0; row < height; row++)
    {
        uint16_t source_row = ((uint32_t)row * icon->height) / height;
        for (column = 0; column < width; column++)
        {
            uint16_t source_column = ((uint32_t)column * icon->width) / width;
            const uint8_t *pixel = icon->pixels +
                2U * ((uint32_t)source_row * icon->width + source_column);
            *(volatile uint16_t *)FSMC_Addr_NT35510_DATA =
                /* Image2LCD stores the RGB565 value little-endian. */
                ((uint16_t)pixel[1] << 8) | pixel[0];
        }
    }
}
