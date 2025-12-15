#pragma once
#include "common.h"

static inline uint8_t imgW(const uint8_t *data) { return pgm_read_byte(&data[0]); }
static inline uint8_t imgH(const uint8_t *data) { return pgm_read_byte(&data[1]); }

static inline uint16_t offsetsOffset() { return 2; }
static inline uint16_t coordsOffset()  { return 2 + 257 * 2; }

static inline uint16_t readOffset(const uint8_t *data, uint8_t idx) {
  uint16_t off = offsetsOffset() + (uint16_t)idx * 2;
  uint8_t lo = pgm_read_byte(&data[off]);
  uint8_t hi = pgm_read_byte(&data[off + 1]);
  return (uint16_t)lo | ((uint16_t)hi << 8);
}

static inline uint16_t readCoord(const uint8_t *data, uint16_t i) {
  uint16_t off = coordsOffset() + i * 2;
  uint8_t lo = pgm_read_byte(&data[off]);
  uint8_t hi = pgm_read_byte(&data[off + 1]);
  return (uint16_t)lo | ((uint16_t)hi << 8);
}

void drawPROGMEMGradient(const uint8_t *data, uint8_t idx, int16_t ox, int16_t oy, uint16_t color565, Adafruit_ST7735 &tft) {
  uint16_t start = readOffset(data, idx);
  uint16_t end   = readOffset(data, idx + 1);

  tft.startWrite();
  for (uint16_t i = start; i < end; i++) {
    uint16_t packed = readCoord(data, i);
    int16_t x = ox + unpackX(packed);
    int16_t y = oy + unpackY(packed);

    if (x < 0 || y < 0 || x >= 128 || y >= 160) continue;
    tft.writePixel(x, y, color565);
  }
  tft.endWrite();
}