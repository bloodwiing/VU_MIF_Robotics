#pragma once
#include <stdint.h>
#include "Arduino.h"
#include "draw_shapes.h"
#include "door_gradient.h"
#include "door_part.h"

struct DoorConfig {
  uint16_t x;
  uint16_t y;
  uint16_t dark_color;
  uint16_t color;
  uint16_t min;
  uint16_t max;
  uint16_t value;
};

uint16_t getDoorGradientIndex(DoorConfig* door) {
  return map(door->value, door->min, door->max, 0, 255);
}

void redrawDoor(DoorConfig* door, Adafruit_ST7735 &tft) {
  drawPROGMEMImage(door_frame_coords, door_frame_count, door->x, door->y, 0xFFFF, tft);

  uint16_t gradient_val = getDoorGradientIndex(door);

  for (int i = 0; i < 255; i++) {
    drawPROGMEMGradient(door_gradient_data, i, door->x, door->y, i < gradient_val ? door->color : door->dark_color, tft);
  }
}

DoorConfig* createDoor(uint16_t ox, uint16_t oy, uint16_t dark_color, uint16_t color, uint16_t min, uint16_t max) {
  DoorConfig* door = new DoorConfig();
  door->x = ox;
  door->y = oy;
  door->dark_color = dark_color;
  door->color = color;
  door->min = min;
  door->max = max;
  door->value = min;

  return door;
}

void updateDoor(DoorConfig* door, uint16_t value, Adafruit_ST7735 &tft) {
  uint16_t old_gradient_val = getDoorGradientIndex(door);
  
  door->value = value;
  if (door->value > door->max) {
    door->value = door->max;
  }
  if (door->value < door->min) {
    door->value = door->min;
  }

  uint16_t new_gradient_val = getDoorGradientIndex(door);

  if (new_gradient_val > old_gradient_val) {
    for (uint16_t i = old_gradient_val; i < new_gradient_val; i++) {
      drawPROGMEMGradient(door_gradient_data, i, door->x, door->y, door->color, tft);
    }
  }
  else if (new_gradient_val < old_gradient_val) {
    for (uint16_t i = old_gradient_val; i > new_gradient_val; i--) {
      drawPROGMEMGradient(door_gradient_data, i, door->x, door->y, door->dark_color, tft);
    }
  }
}

void removeDoor(DoorConfig* door) {
  delete door;
}