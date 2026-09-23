#ifndef NT35510_IMAGE_H
#define NT35510_IMAGE_H

#include <stdint.h>
#include "./image/icon_assets.h"

void NT35510_DrawImageScaled(uint16_t x, uint16_t y, uint16_t width,
                             uint16_t height, const weather_icon_t *icon);

#endif
