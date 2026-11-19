#pragma once
#include <Arduino.h>
#include <Arduino_DataBus.h>
#include <driver/spi_master.h>
#include <esp_heap_caps.h>
#include <string.h>

// Full-frame writer using two internal SRAM DMA buffers.
class ST77916Panel {
public:
  bool begin(size_t chunkBytes, int isrCore) {
    chunkBytes_ = chunkBytes;
    if (chunkBytes_ < 64 || chunkBytes_ > 16384 || chunkBytes_ % 64) return false;
    for (unsigned i = 0; i < DMA_SLOTS; ++i) {
      dma_[i] = static_cast<uint8_t *>(heap_caps_aligned_alloc(
          64, chunkBytes_, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
      if (!dma_[i]) { freeBuffers(); return false; }
    }

    pinMode(CS, OUTPUT);
    digitalWrite(CS, HIGH);
    spi_bus_config_t bus = {};
    bus.mosi_io_num = 4;
    bus.miso_io_num = 5;
    bus.sclk_io_num = 17;
    bus.quadwp_io_num = 6;
    bus.quadhd_io_num = 7;
    bus.data4_io_num = bus.data5_io_num = bus.data6_io_num = bus.data7_io_num = -1;
    bus.max_transfer_sz = 16384;
    bus.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS;
    bus.isr_cpu_id = isrCore == 0 ? ESP_INTR_CPU_AFFINITY_0 : ESP_INTR_CPU_AFFINITY_1;
    if (spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK) {
      freeBuffers(); return false;
    }

    spi_device_interface_config_t dev = {};
    dev.command_bits = 8;
    dev.address_bits = 24;
    dev.mode = 0;
    dev.clock_speed_hz = 40000000;
    dev.spics_io_num = -1;
    dev.flags = SPI_DEVICE_HALFDUPLEX;
    dev.queue_size = DMA_SLOTS;
    if (spi_bus_add_device(SPI2_HOST, &dev, &device_) != ESP_OK) {
      spi_bus_free(SPI2_HOST); freeBuffers(); return false;
    }
    return true;
  }

  void initialize(const uint8_t *ops, size_t length) {
    size_t p = 0;
    while (p < length) {
      const uint8_t op = ops[p++];
      if (op == BEGIN_WRITE || op == END_WRITE) continue;
      if (op == DELAY) { require(p < length); delay(ops[p++]); continue; }
      require(p < length);
      const uint8_t cmd = ops[p++];
      size_t n = 0;
      switch (op) {
        case WRITE_COMMAND_8: break;
        case WRITE_C8_D8: n = 1; break;
        case WRITE_C8_D16: n = 2; break;
        case WRITE_C8_BYTES: require(p < length); n = ops[p++]; break;
        default: require(false);
      }
      require(n <= length - p);
      command(cmd, ops + p, n);
      p += n;
    }
  }

  void writeFrame(const uint16_t *pixels) {
    ESP_ERROR_CHECK(spi_device_acquire_bus(device_, portMAX_DELAY));
    const uint8_t extent[] = {0, 0, 1, 0x67};
    const uint8_t vertical[] = {0, 0, 1, 0x67};
    command(0x2a, extent, 4);
    command(0x2b, vertical, 4);
    command(0x2c, nullptr, 0);

    const auto *data = reinterpret_cast<const uint8_t *>(pixels);
    size_t remain = 360 * 360 * sizeof(uint16_t);
    digitalWrite(CS, LOW);
    bool first = true;
    unsigned pending = 0, submitted = 0;
    while (remain) {
      const size_t bytes = min(remain, chunkBytes_);
      const unsigned slot = submitted % DMA_SLOTS;
      if (pending == DMA_SLOTS) {
        reap(&transactions_[slot].base);
        --pending;
      }

      memcpy(dma_[slot], data, bytes);
      auto &transaction = transactions_[slot];
      transaction = {};
      transaction.base.flags = SPI_TRANS_MODE_QIO;
      if (first) {
        transaction.base.cmd = 0x32;
        transaction.base.addr = 0x003c00;
      } else {
        transaction.base.flags |= SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR |
                                  SPI_TRANS_VARIABLE_DUMMY;
      }
      transaction.base.tx_buffer = dma_[slot];
      transaction.base.length = bytes * 8;
      ESP_ERROR_CHECK(spi_device_queue_trans(device_, &transaction.base, portMAX_DELAY));
      ++pending;
      first = false;
      ++submitted;
      data += bytes;
      remain -= bytes;
    }

    while (pending) {
      const unsigned slot = (submitted - pending) % DMA_SLOTS;
      reap(&transactions_[slot].base);
      --pending;
    }
    digitalWrite(CS, HIGH);
    spi_device_release_bus(device_);
  }

private:
  static constexpr int CS = 10;
  static constexpr unsigned DMA_SLOTS = 2;
  size_t chunkBytes_ = 8192;
  spi_device_handle_t device_ = nullptr;
  uint8_t *dma_[DMA_SLOTS] = {};
  spi_transaction_ext_t transactions_[DMA_SLOTS] = {};

  void freeBuffers() {
    for (auto &p : dma_) { heap_caps_free(p); p = nullptr; }
  }
  static void require(bool condition) { if (!condition) abort(); }

  void command(uint8_t cmd, const uint8_t *data, size_t bytes) {
    spi_transaction_t transaction = {};
    transaction.cmd = 0x02;
    transaction.addr = static_cast<uint32_t>(cmd) << 8;
    transaction.tx_buffer = bytes ? data : nullptr;
    transaction.length = bytes * 8;
    digitalWrite(CS, LOW);
    ESP_ERROR_CHECK(spi_device_polling_transmit(device_, &transaction));
    digitalWrite(CS, HIGH);
  }

  void reap(spi_transaction_t *expected) {
    spi_transaction_t *completed = nullptr;
    ESP_ERROR_CHECK(spi_device_get_trans_result(device_, &completed, portMAX_DELAY));
    require(completed == expected);
  }
};
