#pragma once
#include <stdint.h>
#include "Arduino.h"
#include "dial_gradient.h"
#include "draw_shapes.h"
#include "dial_part.h"

struct DialConfig {
  uint16_t x;
  uint16_t y;
  uint16_t dark_color;
  uint16_t color;
  uint16_t min;
  uint16_t max;
  uint16_t value;
};

uint16_t getDialGradientIndex(DialConfig* dial) {
  return map(dial->value, dial->min, dial->max, 0, 255);
}

void redrawDialWheel(DialConfig* dial, Adafruit_ST7735 &tft) {
  drawPROGMEMImage(dial_sides_coords, dial_sides, dial->x, dial->y, 0xFFFF, tft);

  uint16_t gradient_val = getDialGradientIndex(dial);

  for (int i = 0; i < 255; i++) {
    drawPROGMEMGradient(dial_gradient_data, i, dial->x, dial->y, i < gradient_val ? dial->color : dial->dark_color, tft);
  }
}

DialConfig* createDialWheel(uint16_t ox, uint16_t oy, uint16_t dark_color, uint16_t color, uint16_t min, uint16_t max) {
  DialConfig* dial = new DialConfig();
  dial->x = ox;
  dial->y = oy;
  dial->dark_color = dark_color;
  dial->color = color;
  dial->min = min;
  dial->max = max;
  dial->value = min;

  return dial;
}

void updateDialWheel(DialConfig* dial, uint16_t value, Adafruit_ST7735 &tft) {
  uint16_t old_gradient_val = getDialGradientIndex(dial);
  
  dial->value = value;
  if (dial->value > dial->max) {
    dial->value = dial->max;
  }
  if (dial->value < dial->min) {
    dial->value = dial->min;
  }

  uint16_t new_gradient_val = getDialGradientIndex(dial);

  if (new_gradient_val > old_gradient_val) {
    for (uint16_t i = old_gradient_val; i < new_gradient_val; i++) {
      drawPROGMEMGradient(dial_gradient_data, i, dial->x, dial->y, dial->color, tft);
    }
  }
  else if (new_gradient_val < old_gradient_val) {
    for (uint16_t i = old_gradient_val; i > new_gradient_val; i--) {
      drawPROGMEMGradient(dial_gradient_data, i, dial->x, dial->y, dial->dark_color, tft);
    }
  }
}

void removeDialWheel(DialConfig* dial) {
  delete dial;
}