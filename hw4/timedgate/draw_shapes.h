#pragma once
#include "common.h"

void drawPROGMEMImage(const uint16_t *coords, uint16_t count, int16_t ox, int16_t oy, uint16_t color, Adafruit_ST7735 &tft) {
  for (uint16_t i = 0; i < count; i++) {
    uint16_t packed = pgm_read_word(&coords[i]);
    int16_t x = ox + unpackX(packed);
    int16_t y = oy + unpackY(packed);

    // bounds safety
    if (x < 0 || y < 0 || x >= 128 || y >= 160) continue;

    tft.drawPixel(x, y, color);
  }
}