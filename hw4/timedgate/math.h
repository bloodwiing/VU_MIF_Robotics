#pragma once

static inline float easeInOutPow(float t, float p) {
  if (t <= 0.0f) return 0.0f;
  if (t >= 1.0f) return 1.0f;

  if (t < 0.5f) {
    float a = 2.0f * t;
    return 0.5f * powf(a, p);
  } else {
    float a = 2.0f * (1.0f - t);
    return 1.0f - 0.5f * powf(a, p);
  }
}

float easeIncrementSnappy(int i, int N, float p) {
  if (N <= 0) return 0.0f;
  if (i < 0 || i >= N) return 0.0f;

  float t0 = (float)i / (float)N;
  float t1 = (float)(i + 1) / (float)N;
  return easeInOutPow(t1, p) - easeInOutPow(t0, p);
}
