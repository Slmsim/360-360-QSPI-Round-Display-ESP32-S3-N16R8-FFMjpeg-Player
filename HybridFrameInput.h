#pragma once
#include "MjpegReader.h"

// The dispatch task is the only File owner. A compressed tail buffer belongs
// to the same free-frame slot as its RGB output, and remains untouched until
// the display returns that slot (strictly later than decoder.close()).
class HybridFrameInput {
public:
  static constexpr unsigned MAX_SLOTS = 3;
  bool begin(MjpegReader &reader, unsigned slots) {
    if (reader_ || slots < 2 || slots > MAX_SLOTS || !reader.frameCount()) return false;
    reader_ = &reader;
    slots_ = slots;
    capacity_ = reader.largestFrameBytes();
    // No extra buffers when the entire video is cached.
    if (reader.cachedFrameCount() == reader.frameCount()) return true;
    for (unsigned i = 0; i < slots_; ++i) {
      buffers_[i] = static_cast<uint8_t *>(heap_caps_aligned_alloc(
          64, capacity_, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
      if (!buffers_[i]) { reset(); return false; }
    }
    return true;
  }

  // Called by the single dispatch task after it receives the matching free
  // output slot. No asynchronous file owner or whole-JPEG staging copy.
  bool read(size_t number, unsigned slot, uint8_t *&data, size_t &length) {
    data = nullptr; length = 0;
    if (!reader_ || slot >= slots_) return false;
    if (number < reader_->cachedFrameCount()) return reader_->cachedFrameView(number, data, length);
    size_t offset = 0;
    if (!buffers_[slot] || !reader_->frameInfo(number, offset, length) ||
        !length || length > capacity_) { length = 0; return false; }
    // One complete frame per call on the LittleFS path.
    if (!reader_->loadChunk(offset, buffers_[slot], length)) {
      length = 0; return false;
    }
    data = buffers_[slot];
    return true;
  }
  size_t allocatedBytes() const { return buffers_[0] ? slots_ * capacity_ : 0; }
  void reset() {
    for (auto &p : buffers_) { heap_caps_free(p); p = nullptr; }
    reader_ = nullptr; slots_ = 0; capacity_ = 0;
  }
private:
  MjpegReader *reader_ = nullptr;
  uint8_t *buffers_[MAX_SLOTS] = {};
  unsigned slots_ = 0;
  size_t capacity_ = 0;
};
