#pragma once
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

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

void drawDigit(Adafruit_ST7735 &tft, uint8_t digit) {
  tft.setFont(&FreeSansBold18pt7b);
  tft.fillRect(65, 65, 30, 40, 0);
  tft.setTextColor(0xFFFF);
  tft.setCursor(70, 95);
  tft.print(digit);
}

void drawChar(Adafruit_ST7735 &tft, char chr) {
  tft.setFont(&FreeSansBold18pt7b);
  tft.fillRect(65, 65, 30, 40, 0);
  tft.setTextColor(0xFFFF);
  tft.setCursor(68, 95);
  tft.print(chr);
}

void drawClock(Adafruit_ST7735 &tft, uint8_t hour, uint8_t minute, uint16_t ox, uint16_t oy) {
  tft.setFont();
  tft.setTextSize(4);
  tft.setTextColor(0xFFFF, 0);
  
  tft.setCursor(25+ox, 50+oy);
  if (hour < 10) {
    tft.print(0);
  }
  tft.print(hour);
  
  tft.setCursor(70+ox, 50+oy);
  tft.setTextColor(0xFFFF);
  tft.print(":");
  tft.setTextColor(0xFFFF, 0);
  
  tft.setCursor(90+ox, 50+oy);
  if (minute < 10) {
    tft.print(0);
  }
  tft.print(minute);
  tft.setTextSize(1);
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