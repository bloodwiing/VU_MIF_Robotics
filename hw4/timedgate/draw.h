#pragma once
#include <string.h>
#include <stddef.h>
#include <stdint.h>
// #include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Adafruit_GFX.h>
#include "math.h"
#include "constants.h"

GFXcanvas1 charBuf(30, 40);  // digit or char
GFXcanvas1 optionsBuf(160, 128);  // options menu

void drawTitleSubtitle(Adafruit_ST7735 &tft, const char* title, const char* subtitle) {
  tft.setFont();
  tft.setTextColor(0xB5B6);
  tft.setCursor(10, 8);
  tft.print(title);

  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(0xFFFF);
  tft.setCursor(10, 32);
  tft.print(subtitle);
}

void drawTitle(Adafruit_ST7735 &tft, const char* title) {
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(0xFFFF);
  tft.setCursor(10, 24);
  tft.print(title);
}

void drawDigit(Adafruit_ST7735 &tft, uint8_t digit, uint32_t frame) {
  // tft.setFont(&FreeSansBold18pt7b);
  // tft.fillRect(65, 65, 30, 40, 0);
  // tft.setTextColor(0xFFFF);
  // tft.setCursor(70, 95);
  // tft.print(digit);
  uint16_t moveY = (uint16_t)(easeIncrementSnappy(frame, ANIMATION_FRAMES, ANIMATION_SNAPPINESS) * 40);
  Serial.println(moveY);
  uint16_t topY = 40 - (uint16_t)(easeInOutPow((float)frame / (float)(ANIMATION_FRAMES), ANIMATION_SNAPPINESS) * 40);
  // charBuf.fillScreen(0);
  charBuf.setFont(&FreeSansBold9pt7b);
  charBuf.setTextSize(2);
  // charBuf.drawBitmap(0, moveY, charBuf.getBuffer(), 30, 40-moveY, 1, 0);
  charBuf.fillRect(0, 0, 30, 0, 0);
  charBuf.setCursor(5, 30-topY);
  charBuf.setTextColor(1);
  charBuf.print(digit);
  tft.drawBitmap(65, 65, charBuf.getBuffer(), 30, 40, 0xFFFF, 0);
}

void drawChar(Adafruit_ST7735 &tft, char chr) {
  // tft.setFont(&FreeSansBold18pt7b);
  tft.fillRect(65, 65, 30, 40, 0);
  tft.setTextColor(0xFFFF);
  tft.setCursor(68, 95);
  tft.print(chr);
}

void drawHours(Adafruit_ST7735 &tft, uint8_t hours, uint16_t ox, uint16_t oy, uint16_t color) {
  tft.setFont();
  tft.setTextSize(4);
  tft.setTextColor(color, 0);
  
  tft.setCursor(25+ox, 50+oy);
  if (hours < 10) {
    tft.print(0);
  }
  tft.print(hours);

  tft.setTextSize(1);
}

void drawClockSeparator(Adafruit_ST7735 &tft, uint16_t ox, uint16_t oy, uint16_t color) {
  tft.setFont();
  tft.setTextSize(4);
  tft.setCursor(70+ox, 50+oy);
  tft.setTextColor(color);
  tft.print(":");

  tft.setTextSize(1);
}

void drawMinutes(Adafruit_ST7735 &tft, uint8_t minutes, uint16_t ox, uint16_t oy, uint16_t color) {
  tft.setFont();
  tft.setTextSize(4);
  tft.setTextColor(color, 0);
  
  tft.setCursor(90+ox, 50+oy);
  if (minutes < 10) {
    tft.print(0);
  }
  tft.print(minutes);

  tft.setTextSize(1);
}

void drawClock(Adafruit_ST7735 &tft, uint8_t hours, uint8_t minutes, uint16_t ox, uint16_t oy) {
  drawHours(tft, hours, ox, oy, 0xFFFF);
  drawClockSeparator(tft, ox, oy, 0xFFFF);
  drawMinutes(tft, minutes, ox, oy, 0xFFFF);
}

template <size_t ROWS>
void drawOptionsMenu(Adafruit_ST7735 &tft, const char* const* options, const char (*values)[ROWS], uint8_t total, uint8_t min, uint8_t selected) {
  uint16_t y = 65;
  uint16_t x = 12;

  tft.fillRect(0, 40, 160, 128, 0);

  tft.setFont(&FreeSansBold9pt7b);

  for (; min < total; min++) {
    if (min == selected) {
      tft.setTextColor(0x37E6);
    } else {
      tft.setTextColor(0xB5B6);
    }
    tft.setCursor(x, y);
    tft.print(options[min]);

    size_t len = strlen(values[min]);

    tft.setCursor(150 - 10 * len, y);
    tft.print(values[min]);

    y += 22;
  }
}