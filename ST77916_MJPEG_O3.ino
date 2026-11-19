#include <Arduino.h>
#include <atomic>
#include <new>
#include <string.h>
#include "JPEGDEC_Circle.h"
#include "SimdIDCT.h"
#include <LittleFS.h>
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "JpegPixelOutput.h"
#include "ST77916Panel.h"
#include "HybridFrameInput.h"
#include "AdaptivePrefixCache.h"
#include "PanelInit.h"

// Upper playback rate. Match this to the encoded video frame rate if desired.
constexpr uint32_t VIDEO_FPS = 60;
constexpr uint32_t FRAME_PERIOD_US = 1000000UL / VIDEO_FPS;
constexpr int LCD_BACKLIGHT_PIN = 11;
constexpr int LCD_RESET_PIN = 9;
constexpr size_t MAX_JPEG_BYTES = 128 * 1024;
constexpr size_t CACHE_LIMIT = (72 * 1024 * 1024) / 10; // 7.2 MiB, leaves PSRAM margin after RGB slots.
constexpr size_t CACHE_HEADROOM = 64 * 1024;
constexpr unsigned RGB_SLOTS = 3;
constexpr bool REPORT_FRAME_STATS = true;
constexpr uint32_t FRAME_STATS_PERIOD_US = 2000000;
constexpr size_t FRAME_INTERVAL_BUCKETS = 101; // 10 ms buckets through 1 second.

constexpr uint16_t YIELD_EVERY_JOBS = 1;
constexpr uint32_t WATCHDOG_TIMEOUT_MS = 5000;

struct Frame { uint8_t slot; uint32_t sequence; uint32_t decodeUs; bool ok; };
struct Job { Frame frame; uint8_t *data; size_t bytes; };
File video;
MjpegReader reader(video);
HybridFrameInput frameInput;
ST77916Panel panel;
JPEGDEC firstDecoder;
JPEGDEC *decoders[2] = {};
uint16_t *rgb[RGB_SLOTS] = {};
QueueHandle_t freeFrames, jobs[2], results[2];
std::atomic<bool> playing{false};
uint32_t appliedWatchdogTimeoutMs = 0;
uint32_t sequence = 0;
size_t frameNumber = 0;

void fail(const char *message) {
  Serial.printf("ERROR,%s\n", message);
  playing = false;
}

bool configureTaskWatchdog(uint32_t timeoutMs, esp_err_t &result) {
  esp_task_wdt_config_t config = {};
  config.timeout_ms = timeoutMs;
  config.idle_core_mask = (1U << portNUM_PROCESSORS) - 1U;
  // Keep the task watchdog fail-fast on timeout.
  config.trigger_panic = true;
  result = esp_task_wdt_reconfigure(&config);
  if (result == ESP_ERR_INVALID_STATE) {
    // The application may be built with automatic TWDT startup disabled.
    result = esp_task_wdt_init(&config);
  }
  return result == ESP_OK;
}

void decodeWorker(void *argument) {
  const unsigned worker = reinterpret_cast<uintptr_t>(argument);
  JPEGDEC &decoder = *decoders[worker];
  JpegPixelOutput pixels;
  for (;;) {
    Job job;
    xQueueReceive(jobs[worker], &job, portMAX_DELAY);
    bool ok = false;
    const uint32_t decodeStarted = micros();
    if (decoder.openRAM(job.data, job.bytes, JpegPixelOutput::draw)) {
      if (decoder.getWidth() == 360 && decoder.getHeight() == 360 &&
          decoder.getJPEGType() == JPEG_MODE_BASELINE) {
        pixels.reset(rgb[job.frame.slot]);
        decoder.setUserPointer(&pixels);
        decoder.setPixelType(RGB565_BIG_ENDIAN);
        ok = decoder.decode(0, 0, 0) && decoder.getLastError() == JPEG_SUCCESS;
      }
      decoder.close();
    }
    job.frame.decodeUs = micros() - decodeStarted;
    job.frame.ok = ok;
    xQueueSend(results[worker], &job.frame, portMAX_DELAY);
    delay(1); // Yield so the per-core idle task can run after every job.
  }
}

void displayTask(void *) {
  uint32_t displayed = 0, nextAt = 0;
  uint32_t statsWindowStart = micros(), lastDisplayComplete = 0, windowFrames = 0;
  uint32_t intervalCount = 0, maxIntervalUs = 0, intervalsOver100ms = 0;
  uint64_t intervalSumUs = 0, decodeSumUs = 0;
  uint16_t intervalHistogram[FRAME_INTERVAL_BUCKETS] = {};
  for (;;) {
    Frame frame;
    xQueueReceive(results[displayed % 2], &frame, portMAX_DELAY);
    if (!frame.ok || frame.sequence != displayed) {
      fail("JPEG decode or frame order failed");
      xQueueSend(freeFrames, &frame.slot, portMAX_DELAY);
      vTaskDelete(nullptr); return;
    }
    if (displayed) while (static_cast<int32_t>(nextAt - micros()) > 0) delay(1);
    const uint32_t started = micros();
    panel.writeFrame(rgb[frame.slot]);
    const uint32_t completedAt = micros();
    xQueueSend(freeFrames, &frame.slot, portMAX_DELAY);
    if (!displayed) digitalWrite(LCD_BACKLIGHT_PIN, HIGH);

    if (REPORT_FRAME_STATS) {
      ++windowFrames;
      decodeSumUs += frame.decodeUs;
      if (lastDisplayComplete) {
        const uint32_t intervalUs = completedAt - lastDisplayComplete;
        size_t bucket = intervalUs / 10000UL;
        if (bucket >= FRAME_INTERVAL_BUCKETS) bucket = FRAME_INTERVAL_BUCKETS - 1;
        ++intervalHistogram[bucket];
        ++intervalCount;
        intervalSumUs += intervalUs;
        maxIntervalUs = max(maxIntervalUs, intervalUs);
        if (intervalUs >= 100000) ++intervalsOver100ms;
      }
      lastDisplayComplete = completedAt;

      const uint32_t elapsedUs = completedAt - statsWindowStart;
      if (elapsedUs >= FRAME_STATS_PERIOD_US) {
        const uint64_t fpsHundredths =
            (static_cast<uint64_t>(windowFrames) * 100000000ULL + elapsedUs / 2) / elapsedUs;
        const uint32_t avgMsTenths = intervalCount
            ? static_cast<uint32_t>((intervalSumUs + static_cast<uint64_t>(intervalCount) * 50) /
                                    (static_cast<uint64_t>(intervalCount) * 100))
            : 0;
        const uint32_t decodeAvgMsTenths =
            (decodeSumUs + static_cast<uint64_t>(windowFrames) * 50) /
            (static_cast<uint64_t>(windowFrames) * 100);
        const uint32_t p95Target = intervalCount ? (intervalCount * 95 + 99) / 100 : 0;
        uint32_t p95UpperMs = 0, cumulative = 0;
        for (size_t i = 0; i < FRAME_INTERVAL_BUCKETS && p95Target; ++i) {
          cumulative += intervalHistogram[i];
          if (cumulative >= p95Target) {
            p95UpperMs = static_cast<uint32_t>((i + 1) * 10);
            break;
          }
        }
        // Keep serial output off the critical path if the USB buffer is backed up.
        if (Serial.availableForWrite() >= 200) {
          Serial.printf("PLAYBACK_STATS,frames=%u,total=%u,fps=%u.%02u,avg_ms=%u.%u,p95_upper_ms=%u,max_ms=%u,ge100ms=%u,decode_avg_ms=%u.%u\n",
              static_cast<unsigned>(windowFrames), static_cast<unsigned>(displayed + 1),
              static_cast<unsigned>(fpsHundredths / 100), static_cast<unsigned>(fpsHundredths % 100),
              static_cast<unsigned>(avgMsTenths / 10), static_cast<unsigned>(avgMsTenths % 10),
              static_cast<unsigned>(p95UpperMs), static_cast<unsigned>((maxIntervalUs + 500) / 1000),
              static_cast<unsigned>(intervalsOver100ms),
              static_cast<unsigned>(decodeAvgMsTenths / 10), static_cast<unsigned>(decodeAvgMsTenths % 10));
        }
        statsWindowStart = completedAt;
        windowFrames = intervalCount = maxIntervalUs = intervalsOver100ms = 0;
        intervalSumUs = decodeSumUs = 0;
        memset(intervalHistogram, 0, sizeof(intervalHistogram));
      }
    }

    if (!displayed || static_cast<int32_t>(started - nextAt) >= static_cast<int32_t>(FRAME_PERIOD_US)) nextAt = started;
    nextAt += FRAME_PERIOD_US;
    ++displayed;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("WIDECIRCLE_IDCT,profile=%u,rows2=%u\n",
      static_cast<unsigned>(JPEG_IDCT_TEST_PROFILE),
      static_cast<unsigned>(JPEG_ENABLE_S3_IDCT_ROWS2 && JPEG_S3_IDCT_AVAILABLE));
  Serial.printf("JPEG_OUTPUT_BUFFER,mcu_per_band=%u,buffer_pixels=%u,circle_copy_skip=1\n",
                unsigned(MAX_BUFFERED_PIXELS / 256), unsigned(MAX_BUFFERED_PIXELS));
  esp_err_t watchdogResult = ESP_OK;
  const bool watchdogConfigured = configureTaskWatchdog(WATCHDOG_TIMEOUT_MS, watchdogResult);
  appliedWatchdogTimeoutMs = watchdogConfigured ? WATCHDOG_TIMEOUT_MS : 0;
  Serial.printf("PLAYBACK_CONFIG,yield_every_jobs=%u,wdt_requested_ms=%u,wdt_applied_ms=%u\n",
                static_cast<unsigned>(YIELD_EVERY_JOBS), static_cast<unsigned>(WATCHDOG_TIMEOUT_MS),
                static_cast<unsigned>(appliedWatchdogTimeoutMs));
  if (!watchdogConfigured) Serial.printf("WDT_CONFIG_ERROR,err=%s\n", esp_err_to_name(watchdogResult));
  pinMode(LCD_BACKLIGHT_PIN, OUTPUT); digitalWrite(LCD_BACKLIGHT_PIN, LOW);
  if (!LittleFS.begin(false)) { fail("LittleFS mount failed"); return; }
  video = LittleFS.open("/video.mjpeg", "r");
  if (!video || video.isDirectory() || !video.size()) { fail("missing /video.mjpeg"); return; }
  for (unsigned i = 0; i < RGB_SLOTS; ++i) {
    rgb[i] = static_cast<uint16_t *>(heap_caps_aligned_alloc(64, 360 * 360 * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!rgb[i]) { fail("RGB PSRAM allocation failed"); return; }
    memset(rgb[i], 0, 360 * 360 * sizeof(uint16_t)); // Seed untouched corners once.
  }
  Serial.println("Indexing JPEG frames...");
  if (!reader.buildIndex(MAX_JPEG_BYTES)) { fail("invalid video or JPEG larger than 128 KiB"); return; }

  // Mode 1 caches the whole video. Mode 2 reserves tail input buffers first,
  // then caches a prefix ending at a complete JPEG boundary.
  const bool full = reader.cacheVideo(CACHE_LIMIT);
  if (!frameInput.begin(reader, RGB_SLOTS)) { fail("JPEG input allocation failed"); return; }
  if (!full) {
    const unsigned caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
    const size_t freeBytes = heap_caps_get_free_size(caps);
    const size_t largest = heap_caps_get_largest_free_block(caps);
    AdaptiveCacheInfo info;
    if (!cacheAdaptivePrefix(reader, CACHE_LIMIT, freeBytes, largest, CACHE_HEADROOM, info)) {
      fail("prefix cache allocation or flash read failed"); return;
    }
  }
  const bool whole = reader.cachedFrameCount() == reader.frameCount();
  Serial.printf("PLAYBACK,mode=%s,video_bytes=%u,frames=%u,cached_bytes=%u,cached_frames=%u,input_bytes=%u\n",
      whole ? "1-full-cache" : "2-prefix-flash", static_cast<unsigned>(video.size()),
      static_cast<unsigned>(reader.frameCount()), static_cast<unsigned>(reader.cachedBytes()),
      static_cast<unsigned>(reader.cachedFrameCount()), static_cast<unsigned>(frameInput.allocatedBytes()));
  const unsigned caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
  Serial.printf("PSRAM,free=%u,largest=%u\n", static_cast<unsigned>(heap_caps_get_free_size(caps)),
      static_cast<unsigned>(heap_caps_get_largest_free_block(caps)));
  Serial.println("OUTPUT,mode=callback-psram");
  Serial.println("CIRCLE_MCU_SKIP,enabled=baseline-360x360-16x16-only,partial-edge-mcus=decoded");

  const int displayCore = xPortGetCoreID() == 0 ? 1 : 0;
  if (!panel.begin(8192, displayCore)) { fail("LCD initialization failed"); return; }
  freeFrames = xQueueCreate(RGB_SLOTS, sizeof(uint8_t));
  if (!freeFrames) { fail("frame queue allocation failed"); return; }
  for (uint8_t i = 0; i < RGB_SLOTS; ++i) xQueueSend(freeFrames, &i, 0);
  decoders[0] = &firstDecoder;
  void *workspace = heap_caps_aligned_alloc(16, sizeof(JPEGDEC), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!workspace) { fail("second decoder allocation failed"); return; }
  decoders[1] = new (workspace) JPEGDEC;
  for (unsigned i = 0; i < 2; ++i) {
    jobs[i] = xQueueCreate(1, sizeof(Job)); results[i] = xQueueCreate(1, sizeof(Frame));
    if (!jobs[i] || !results[i]) { fail("decoder queue allocation failed"); return; }
  }
  pinMode(LCD_RESET_PIN, OUTPUT); digitalWrite(LCD_RESET_PIN, HIGH); delay(20);
  digitalWrite(LCD_RESET_PIN, LOW); delay(120); digitalWrite(LCD_RESET_PIN, HIGH); delay(120);
  panel.initialize(panelInit, sizeof(panelInit));
  for (unsigned i = 0; i < 2; ++i) {
    if (xTaskCreatePinnedToCore(decodeWorker, i ? "jpeg-core1" : "jpeg-core0", 8192,
        reinterpret_cast<void *>(static_cast<uintptr_t>(i)), 1, nullptr, i) != pdPASS) {
      fail("decoder task creation failed"); return;
    }
  }
  if (xTaskCreatePinnedToCore(displayTask, "lcd-player", 6144, nullptr, 2, nullptr, displayCore) != pdPASS) {
    fail("display task creation failed"); return;
  }
  playing = true;
}

void loop() {
  if (!playing) { delay(20); return; }
  Frame frame{};
  if (xQueueReceive(freeFrames, &frame.slot, pdMS_TO_TICKS(10)) != pdTRUE) return;
  frame.sequence = sequence;
  uint8_t *data = nullptr; size_t bytes = 0;
  if (!frameInput.read(frameNumber, frame.slot, data, bytes)) {
    xQueueSend(freeFrames, &frame.slot, portMAX_DELAY);
    fail("frame read failed"); return;
  }
  Job job{frame, data, bytes};
  xQueueSend(jobs[sequence % 2], &job, portMAX_DELAY);
  ++sequence;
  frameNumber = (frameNumber + 1) % reader.frameCount();
}
