// ESP32-S3 PIE IDCT row pass for blocks with only columns 0/1 occupied.
// Original implementation for this player, Apache-2.0.
// Instruction semantics: Espressif ESP32-S3 TRM, Chapter 1 (PIE).
#include "SimdIDCT.h"

#if JPEG_ENABLE_S3_IDCT_ROWS2 && JPEG_S3_IDCT_AVAILABLE
    .text
    .align 4
    .global s3_idct_rows2
    .type s3_idct_rows2,@function

// a2: aligned int16_t[16], column 0 followed by column 1
// a3: aligned output, 8x8 uint8_t (aliases the caller's int16_t block)
// a4: int16_t constants {217,145,51,1,128,255}
// q0=DC, q1=AC, q2=AC*217>>8, q3=AC*145>>8, q4=AC*51>>8
// q5=temporary constant/zero, q6=128, q7=output column
s3_idct_rows2:
    entry a1,160
    memw
    rsr a14,SAR
    ee.vld.128.ip q0,a2,16
    ee.vld.128.ip q1,a2,0
    movi a8,8
    wsr a8,SAR
    ee.vldbc.16.ip q5,a4,2
    ee.vmul.s16 q2,q1,q5
    ee.vldbc.16.ip q5,a4,2
    ee.vmul.s16 q3,q1,q5
    ee.vldbc.16.ip q5,a4,2
    ee.vmul.s16 q4,q1,q5
    addi a9,a4,2
    addi a10,a4,4
    ee.vldbc.16.ip q6,a9,0
    movi a8,5
    wsr a8,SAR
    addi a5,a1,16

    // Scale by 32, add JPEG's 128 bias, clamp, narrow to bytes.
    // Every column is stored as 8 bytes followed by 8 zero bytes.
    .macro emit_column
    ee.vldbc.16.ip q5,a4,0
    ee.vmul.s16 q7,q7,q5
    ee.vadds.s16 q7,q7,q6
    ee.xorq q5,q5,q5
    ee.vmax.s16 q7,q7,q5
    ee.vldbc.16.ip q5,a10,0
    ee.vmin.s16 q7,q7,q5
    ee.xorq q5,q5,q5
    ee.vunzip.8 q7,q5
    ee.vst.128.ip q7,a5,16
    .endm

    ee.vadds.s16 q7,q0,q1
    emit_column
    ee.vadds.s16 q7,q0,q2
    emit_column
    ee.vadds.s16 q7,q0,q3
    emit_column
    // The scalar code uses -floor(AC*51/256), hence DC+floor here.
    ee.vadds.s16 q7,q0,q4
    emit_column
    ee.vsubs.s16 q7,q0,q4
    emit_column
    ee.vsubs.s16 q7,q0,q3
    emit_column
    ee.vsubs.s16 q7,q0,q2
    emit_column
    ee.vsubs.s16 q7,q0,q1
    emit_column

    // Transpose eight byte columns back into row-major 8x8 pixels.
    addi a5,a1,16
    ee.vld.128.ip q0,a5,16
    ee.vld.128.ip q1,a5,16
    ee.vld.128.ip q2,a5,16
    ee.vld.128.ip q3,a5,16
    ee.vld.128.ip q4,a5,16
    ee.vld.128.ip q5,a5,16
    ee.vld.128.ip q6,a5,16
    ee.vld.128.ip q7,a5,0
    ee.vzip.8 q0,q1
    ee.vzip.8 q2,q3
    ee.vzip.8 q4,q5
    ee.vzip.8 q6,q7
    ee.vzip.16 q0,q2
    ee.vzip.16 q4,q6
    ee.vzip.32 q0,q4
    ee.vzip.32 q2,q6
    ee.vst.128.ip q0,a3,16
    ee.vst.128.ip q4,a3,16
    ee.vst.128.ip q2,a3,16
    ee.vst.128.ip q6,a3,0
    memw
    wsr a14,SAR
    retw.n
    .size s3_idct_rows2,.-s3_idct_rows2
#endif
