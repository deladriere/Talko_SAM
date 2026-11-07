#pragma once
#include <cassert>
#include <cstring>
#include <deque>
#include <vector>

// Single-threaded host simulation of the SDK queue calls used by speech.cpp.
struct queue_t {
  size_t itemSize = 0;
  size_t capacity = 0;
  std::deque<std::vector<unsigned char>> items;
};
inline void queue_init(queue_t *q, size_t itemSize, size_t capacity) {
  q->itemSize = itemSize;
  q->capacity = capacity;
  q->items.clear();
}
inline bool queue_try_add(queue_t *q, const void *item) {
  if (q->items.size() >= q->capacity) return false;
  const auto *bytes = static_cast<const unsigned char *>(item);
  q->items.emplace_back(bytes, bytes + q->itemSize);
  return true;
}
inline bool queue_try_remove(queue_t *q, void *item) {
  if (q->items.empty()) return false;
  std::memcpy(item, q->items.front().data(), q->itemSize);
  q->items.pop_front();
  return true;
}
inline void queue_remove_blocking(queue_t *q, void *item) {
  bool removed = queue_try_remove(q, item);
  assert(removed);
}
