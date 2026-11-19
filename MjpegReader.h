#pragma once

#include <Arduino.h>
#include <FS.h>
#include <esp_heap_caps.h>
#include <stddef.h>
#include <stdint.h>

// Reads an elementary MJPEG stream: consecutive complete JPEG images.
class MjpegReader {
public:
  enum class Result { Frame, End, Invalid, TooLarge };

  explicit MjpegReader(fs::File &file) : file_(file) {}

  ~MjpegReader() { reset(); }

  void reset() {
    heap_caps_free(cache_);
    cache_ = nullptr;
    cacheBytes_ = cachedFrames_ = 0;
    clearIndex();
    pos_ = end_ = streamOffset_ = 0;
  }

  MjpegReader(const MjpegReader &) = delete;
  MjpegReader &operator=(const MjpegReader &) = delete;

  size_t frameCount() const { return indexCount_; }
  size_t largestFrameBytes() const { return largestFrameBytes_; }
  size_t cachedBytes() const { return cache_ ? cacheBytes_ : 0; }
  size_t cachedFrameCount() const { return cache_ ? cachedFrames_ : 0; }

  bool cachedFrameView(size_t number, uint8_t *&data, size_t &length) const {
    data = nullptr; length = 0;
    if (!cache_ || number >= cachedFrames_) return false;
    data = cache_ + index_[number].offset;
    length = index_[number].length;
    return true;
  }

  // Immutable after buildIndex(). Safe for the single consumer while the
  // producer reads the file. No cached JPEG or index entries are modified.
  bool frameInfo(size_t number, size_t &offset, size_t &length) const {
    if (number >= indexCount_) return false;
    offset = index_[number].offset;
    length = index_[number].length;
    return true;
  }

  // Plan complete consecutive JPEGs. Never split a JPEG across buffers.
  bool planChunk(size_t first, size_t capacity, size_t &offset,
                 size_t &bytes, size_t &frames) const {
    offset = bytes = frames = 0;
    if (first >= indexCount_ || index_[first].length > capacity) return false;
    offset = index_[first].offset;
    for (size_t i = first; i < indexCount_; ++i) {
      const FrameEntry &entry = index_[i];
      const size_t span = entry.offset - offset + entry.length;
      if (span > capacity) break;
      bytes = span;
      ++frames;
    }
    return frames != 0;
  }

  // The dispatcher owns LittleFS reads; each call loads one complete JPEG.
  // Yield only if LittleFS returns a partial read.
  bool loadChunk(size_t offset, uint8_t *output, size_t bytes) {
    if (!output || offset > fileBytes_ || bytes > fileBytes_ - offset) return false;
    if (file_.position() != offset && !file_.seek(offset)) return false;
    size_t loaded = 0;
    while (loaded < bytes) {
      const size_t got = file_.read(output + loaded, bytes - loaded);
      if (got == 0 || got > bytes - loaded) return false;
      loaded += got;
      if (loaded < bytes) delay(1);
    }
    return true;
  }

  // Build the frame boundaries once. APP/COM payloads may contain FF D9,
  // so use the JPEG marker parser rather than searching for byte pairs.
  bool buildIndex(size_t maxFrameBytes) {
    reset();
    if (!rewind()) {
      return false;
    }
    size_t capacity = 0;
    for (;;) {
      const size_t offset = streamOffset_;
      size_t length = 0;
      const Result result = readUnindexedFrame(nullptr, maxFrameBytes, length);
      if (result == Result::End) {
        break;
      }
      if (result != Result::Frame || !appendIndex(offset, length, capacity)) {
        clearIndex();
        rewind();
        return false;
      }
      if ((indexCount_ % 32) == 0) {
        delay(1);
      }
    }
    if (indexCount_ == 0 || !rewind()) {
      clearIndex();
      return false;
    }
    fileBytes_ = file_.size();
    return true;
  }

  // Cache the compressed video when it fits the configured PSRAM budget.
  bool cacheVideo(size_t maxBytes) {
    if (indexCount_ == 0 || fileBytes_ == 0 || fileBytes_ > maxBytes) {
      return false;
    }
    return cachePrefix(maxBytes) && cachedFrames_ == indexCount_;
  }

  // Keep a fixed prefix across loops and stream only the tail when the full
  // video is larger than the available PSRAM budget.
  bool cachePrefix(size_t maxBytes) {
    if (cache_) return true;
    size_t offset = 0, bytes = 0, frames = 0;
    if (!planChunk(0, maxBytes, offset, bytes, frames) || offset != 0) return false;
    auto *memory = static_cast<uint8_t *>(
        heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!memory) {
      return false;
    }
    if (!file_.seek(0)) {
      heap_caps_free(memory);
      return false;
    }
    size_t loaded = 0;
    while (loaded < bytes) {
      const size_t request = min(bytes - loaded, size_t(64 * 1024));
      const size_t received = file_.read(memory + loaded, request);
      if (received == 0) {
        heap_caps_free(memory);
        rewind();
        return false;
      }
      loaded += received;
      delay(1);
    }
    cache_ = memory;
    cacheBytes_ = bytes;
    cachedFrames_ = frames;
    rewind();
    return true;
  }

  bool rewind() {
    pos_ = end_ = 0;
    streamOffset_ = 0;
    if (cache_ && cachedFrames_ == indexCount_) {
      return true;
    }
    return file_.seek(0);
  }


private:
  struct FrameEntry {
    size_t offset;
    size_t length;
  };

  void clearIndex() {
    heap_caps_free(index_);
    index_ = nullptr;
    indexCount_ = largestFrameBytes_ = fileBytes_ = 0;
  }

  bool appendIndex(size_t offset, size_t length, size_t &capacity) {
    if (indexCount_ == capacity) {
      if (capacity > SIZE_MAX / (2 * sizeof(FrameEntry))) {
        return false;
      }
      const size_t nextCapacity = capacity ? capacity * 2 : 64;
      auto *expanded = static_cast<FrameEntry *>(heap_caps_realloc(
          index_, nextCapacity * sizeof(FrameEntry), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
      if (!expanded) {
        return false;
      }
      index_ = expanded;
      capacity = nextCapacity;
    }
    index_[indexCount_++] = {offset, length};
    if (length > largestFrameBytes_) {
      largestFrameBytes_ = length;
    }
    return true;
  }

  Result readUnindexedFrame(uint8_t *output, size_t capacity, size_t &length) {
    length = 0;
    const int start = nextByte();
    if (start < 0) {
      return Result::End;
    }
    if (start != 0xff || nextByte() != 0xd8) {
      return Result::Invalid;
    }
    if (capacity < 2) {
      return Result::TooLarge;
    }
    if (output) {
      output[0] = 0xff;
      output[1] = 0xd8;
    }
    length = 2;

    auto appendByte = [&]() -> int {
      if (length == capacity) {
        return -2;
      }
      const int byte = nextByte();
      if (byte >= 0) {
        if (output) {
          output[length] = static_cast<uint8_t>(byte);
        }
        ++length;
      }
      return byte;
    };
    auto readError = [](int byte) {
      return byte == -2 ? Result::TooLarge : Result::Invalid;
    };

    bool inScan = false;
    bool sawScan = false;
    for (;;) {
      const int prefix = appendByte();
      if (prefix < 0) {
        return readError(prefix);
      }
      if (prefix != 0xff) {
        if (inScan) {
          continue;
        }
        return Result::Invalid;
      }

      int marker;
      do {
        marker = appendByte();
      } while (marker == 0xff);
      if (marker < 0) {
        return readError(marker);
      }
      if (inScan && (marker == 0x00 || (marker >= 0xd0 && marker <= 0xd7))) {
        continue; // Stuffed data byte or restart marker inside a scan.
      }
      if (marker == 0xd9) {
        return sawScan ? Result::Frame : Result::Invalid;
      }
      if (marker < 0xc0 || (marker >= 0xd0 && marker <= 0xd8)) {
        return Result::Invalid;
      }

      const int high = appendByte();
      if (high < 0) {
        return readError(high);
      }
      const int low = appendByte();
      if (low < 0) {
        return readError(low);
      }
      const size_t segmentLength = (high << 8) | low;
      if (segmentLength < 2) {
        return Result::Invalid;
      }
      // Markers inside APP/COM payloads are ordinary data.
      for (size_t i = 2; i < segmentLength; ++i) {
        const int byte = appendByte();
        if (byte < 0) {
          return readError(byte);
        }
      }
      inScan = (marker == 0xda);
      sawScan = sawScan || inScan;
    }
  }

  int nextByte() {
    if (pos_ == end_) {
      end_ = file_.read(buffer_, sizeof(buffer_));
      pos_ = 0;
      if (end_ == 0) {
        return -1;
      }
    }
    ++streamOffset_;
    return buffer_[pos_++];
  }

  fs::File &file_;
  uint8_t buffer_[8192];
  size_t pos_ = 0;
  size_t end_ = 0;
  size_t streamOffset_ = 0;
  FrameEntry *index_ = nullptr;
  size_t indexCount_ = 0;
  size_t largestFrameBytes_ = 0;
  size_t fileBytes_ = 0;
  uint8_t *cache_ = nullptr;
  size_t cacheBytes_ = 0, cachedFrames_ = 0;
};
