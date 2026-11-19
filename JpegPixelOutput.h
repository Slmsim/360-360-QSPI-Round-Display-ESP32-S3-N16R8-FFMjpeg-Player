#pragma once
#include <string.h>
#include "JPEGDEC_Circle.h"

// Writes decoded RGB565 MCU blocks directly into their PSRAM frame buffer.
struct JpegPixelOutput {
  uint16_t *frame = nullptr;

  void reset(uint16_t *target) {
    frame = target;
  }

  static int draw(JPEGDRAW *block) {
    auto &self = *static_cast<JpegPixelOutput *>(block->pUser);
    if (block->x < 0 || block->x >= 360 || block->y < 0 || block->y >= 360 ||
        block->iWidth <= 0 || block->iHeight <= 0) return 0;
    const int start = block->copyStart;
    const int end = min(block->copyEnd, 360 - block->x);
    if (end <= start) return 1; // All MCUs in this group are outside the circle.
    const size_t bytes = static_cast<size_t>(end - start) * sizeof(uint16_t);
    const int rows = min(block->iHeight, 360 - block->y);
    for (int row = 0; row < rows; ++row) {
      memcpy(self.frame + (block->y + row) * 360 + block->x + start,
             block->pPixels + row * block->iWidth + start, bytes);
    }
    return 1;
  }
};
