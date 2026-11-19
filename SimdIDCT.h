#pragma once

// 0 uses the original scalar row pass. 1 enables rows2 on supported ESP32-S3 builds.
#ifndef JPEG_IDCT_TEST_PROFILE
#define JPEG_IDCT_TEST_PROFILE 1
#endif
#if JPEG_IDCT_TEST_PROFILE != 0 && JPEG_IDCT_TEST_PROFILE != 1
#error "JPEG_IDCT_TEST_PROFILE must be 0 or 1"
#endif
#define JPEG_ENABLE_S3_IDCT_ROWS2 JPEG_IDCT_TEST_PROFILE

#define JPEG_S3_IDCT_AVAILABLE 0
#if defined(ARDUINO_ARCH_ESP32) && !defined(NO_SIMD)
#if __has_include("dsps_fft2r_platform.h")
#include "dsps_fft2r_platform.h"
#if dsps_fft2r_sc16_aes3_enabled == 1
#undef JPEG_S3_IDCT_AVAILABLE
#define JPEG_S3_IDCT_AVAILABLE 1
#endif
#endif
#endif

#if JPEG_ENABLE_S3_IDCT_ROWS2 && JPEG_S3_IDCT_AVAILABLE && !defined(__ASSEMBLER__)
#include <stdint.h>

extern "C" void s3_idct_rows2(const int16_t *columns, uint8_t *output,
                             const int16_t *constants);

static inline bool JPEGTryS3IDCTRows2(int16_t *block)
{
    alignas(16) int16_t columns[16];
    static const int16_t constants[] = {217, 145, 51, 1, 128, 255};
    for (unsigned row = 0; row < 8; ++row) {
        const int dc = block[row * 8];
        const int ac = block[row * 8 + 1];
        const int magnitude = (dc < 0 ? -dc : dc) + (ac < 0 ? -ac : ac);
        // Keep signed 16-bit intermediates and the scalar lookup index in range.
        if (magnitude > 16000) return false;
        columns[row] = dc;
        columns[8 + row] = ac;
    }
    s3_idct_rows2(columns, reinterpret_cast<uint8_t *>(block), constants);
    return true;
}
#endif
