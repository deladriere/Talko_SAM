#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <string>
using uint = unsigned;
using std::min;
using std::max;
#define __isr
#define __not_in_flash_func(name) name
#define constrain(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
inline uint32_t millis() { static uint32_t n = 0; return n += 80; }
inline long random(long n) { return std::rand() % n; }
inline void tight_loop_contents() {}

struct HostSerial {
  bool connected = true;
  int capacity = 64;
  std::string tx;
  explicit operator bool() const { return connected; }
  int available() const { return 0; }
  int read() { return -1; }
  int availableForWrite() const { return capacity; }
  size_t write(const uint8_t *bytes, size_t count) {
    tx.append(reinterpret_cast<const char *>(bytes), count);
    return count;
  }
};
inline HostSerial Serial;
