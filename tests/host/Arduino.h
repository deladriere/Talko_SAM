#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
using std::min;
using std::max;
#define constrain(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
inline uint32_t millis() { static uint32_t n = 0; return n += 80; }
inline long random(long n) { return std::rand() % n; }
