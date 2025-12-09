#pragma once

static inline uint8_t unpackX(uint16_t p) { return p & 0x7F; }
static inline uint8_t unpackY(uint16_t p) { return p >> 7; }
