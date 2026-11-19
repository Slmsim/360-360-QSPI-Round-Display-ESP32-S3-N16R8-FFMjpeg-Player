#pragma once
#include <stddef.h>

struct AdaptiveCacheInfo {
  size_t budget = 0;
  size_t attempts = 0;
};

// Reserve working memory first, then cache only complete JPEGs. The decreasing
// budget also handles allocation overhead and a fragmented PSRAM heap.
template <typename Reader>
bool cacheAdaptivePrefix(Reader &reader, size_t cap, size_t freeBytes,
                         size_t largestBlock, size_t headroom, AdaptiveCacheInfo &info) {
  info = {};
  size_t firstOffset = 0, firstLength = 0;
  if (!reader.frameInfo(0, firstOffset, firstLength) || firstOffset || !firstLength || freeBytes <= headroom) return false;
  size_t budget = freeBytes - headroom;
  if (budget > cap) budget = cap;
  if (budget > largestBlock) budget = largestBlock;
  info.budget = budget;
  constexpr size_t step = 128 * 1024;
  while (budget >= firstLength) {
    ++info.attempts;
    if (reader.cachePrefix(budget)) return true;
    if (budget == firstLength) break;
    budget = budget - firstLength > step ? budget - step : firstLength;
  }
  return false;
}
