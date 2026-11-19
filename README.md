<div align="center">

🇺🇸 **English** · 🇰🇷 [한국어](README_ko.md)

</div>

# ST77916 360×360 Round Display MJPEG Player

An ESP32-S3 N16R8 firmware project for playing 360×360 MJPEG video on an ST77916 QSPI round display. This folder contains the current O3 release sketch, an example video, and a macOS converter app.
<img width="1125" height="2436" alt="IMG_4B28A8A888C6-1" src="https://github.com/user-attachments/assets/72c5223f-74f9-4454-8478-961173e6c880" />

[YouTube demo](https://youtube.com/shorts/5IbLTVlNvVM?si=wautAE7mvhscs9il)

## What is included

- Dual-core JPEG decoding with three RGB565 frame buffers, so decoding and display transfer can overlap.
- A 23-MCU-wide JPEG output buffer (5,888 pixels), reducing output callback overhead compared with the original narrow buffer.
- An ESP32-S3 SIMD `rows2` IDCT path, enabled by default, with a scalar fallback for unsupported blocks or targets.
- Circular-display optimizations: fully out-of-circle MCUs are skipped, and decoded pixels outside the circle are omitted from the PSRAM copy. Edge MCUs that intersect the circle are still decoded.
- Automatic PSRAM caching: the firmware caches the complete compressed video when it fits within its 7.2 MiB target; otherwise it caches a prefix and reads remaining frames from LittleFS.
- `ST77916 MJPEG Converter.app`, a macOS app that uses FFmpeg to create the board-ready MJPEG file. Use the app instead of the older browser HTML converter.

These optimizations reduce software work and buffering overhead; they do not guarantee a fixed frame rate. In board tests, playback reached about **50 FPS** on the test clip. The sketch's `VIDEO_FPS` setting is a 60 FPS upper limit, not a promise that every video will reach 60 FPS. Results depend on scene complexity, JPEG quality, cache mode, and the board.

## Convert a video

1. Open `ST77916 MJPEG Converter.app` on a Mac and select an MP4 or other supported input video.
2. Set the output frame rate to match `VIDEO_FPS` in `ST77916_MJPEG_O3.ino` (60 FPS by default). The frame rate is a playback target; the board may run slower.
3. First enable **Create q7 and q8 comparison files** and convert the same clip and time range. The app creates `q7.mjpeg` and `q8.mjpeg` in its output folder.
4. Try **q7** first when image quality matters more, then **q8** when you want to test a smaller/easier-to-decode stream. Compare both on the board using the same video and settings. The q number is a JPEG quantizer value, not a universal quality standard; q8 usually trades some image detail for a smaller stream.
5. The firmware expects exactly one file named `video.mjpeg`. Copy either `q7.mjpeg` or `q8.mjpeg` into this sketch's `data/` folder and rename it to `video.mjpeg` before uploading the LittleFS image. The two comparison files cannot both be used as the firmware's `/video.mjpeg` at once.

For a single output, turn off the comparison option. The app's quality slider uses percentages; its default **85% corresponds to q7**. The percentage is the app's control scale and is not a promise of a perceptually linear quality change. The converter encodes baseline 360×360 JPEG frames with 4:2:0 sampling (`yuvj420p`), suitable for this firmware's 16×16 MCU circle-skip path. Audio is not included.

Keep each JPEG frame at or below 128 KiB. The converter estimates output size, but check the resulting file and board logs. Full-video PSRAM caching has a 7.2 MiB target; larger files can use prefix-cache/LittleFS playback and may have lower or less consistent performance.

## Hardware and wiring

- ESP32-S3 N16R8 board (16 MiB Flash, 8 MiB OPI PSRAM)
- 360×360 ST77916 QSPI round display

| Display | ESP32-S3 |
|---|---:|
| GND | GND |
| VCC | 3V3 |
| SCL | GPIO 17 |
| SDA / IO0 | GPIO 4 |
| IO1 | GPIO 5 |
| IO2 | GPIO 6 |
| IO3 | GPIO 7 |
| RST | GPIO 9 |
| CS | GPIO 10 |
| BL | GPIO 11 |
| TE | Not connected |

Check that the display module accepts 3.3 V power and logic levels. Pin labels can vary between modules; verify them on your board.

## Arduino IDE setup and upload

### Requirements

- Arduino IDE with the ESP32 board support package
- Arduino_GFX library 1.6.9

JPEGDEC with the circular-display changes is included in this sketch folder; no separate JPEGDEC installation is needed. The project was prepared with Arduino-ESP32 3.3.11 and ESP-IDF 5.5.5.

### Board settings

Select **ESP32S3 Dev Module** and use:

| Setting | Value |
|---|---|
| Flash Size | 16 MB (128 Mbit) |
| PSRAM | OPI PSRAM |
| USB CDC On Boot | Enabled |
| Partition Scheme | Custom, using this folder's `partitions.csv` |

The custom partition table allocates 768 KiB to the app and 14.75 MiB to LittleFS. Use this same partition layout for firmware and filesystem uploads.

### Flash the firmware and video

1. Open `ST77916_MJPEG_O3.ino` from this folder in Arduino IDE.
2. Put the selected video at `data/video.mjpeg`, with that exact filename.
3. Select the board settings above, then compile and upload the sketch.
4. Use a LittleFS upload tool to upload the **contents of this folder's `data/` directory**. Close Serial Monitor while uploading.
5. Open Serial Monitor at 115200 baud. At startup, confirm the cache mode and frame count; `PLAYBACK_STATS` reports playback measurements about every two seconds.

On first setup, or after changing the partition layout, upload both firmware and the LittleFS image. When only changing the video, upload the LittleFS image again; the firmware does not need to be uploaded again.

## Playback and serial output

`VIDEO_FPS` in the sketch is the playback cap (60 by default). Set the converter's output FPS to the same value. If the display cannot sustain that rate, the firmware plays at the rate it can process frames; the observed maximum on the test clip was around 50 FPS.

At startup, `PLAYBACK` reports video size, frame count, and the selected cache mode. `1-full-cache` means the complete stream was cached in PSRAM; `2-prefix-flash` means only a prefix was cached and remaining frames are read from LittleFS. `PLAYBACK_STATS` reports measured FPS, average frame interval, upper-bound p95 interval, maximum interval, long intervals, and average JPEG decode time. Treat these as measurements of the current clip and setup, not guaranteed performance ratings.

## Folder contents

```text
ST77916_MJPEG_O3.ino                 # O3 firmware entry point
ST77916 MJPEG Converter.app/         # macOS FFmpeg converter
data/video.mjpeg                     # Example video; replace with your own
JPEGDEC_Circle.*                     # JPEG decoder with circular-display support
SimdIDCT.h, s3_idct_rows2.S           # ESP32-S3 IDCT SIMD path
JpegPixelOutput.h                     # RGB565 output and circle-aware PSRAM copy
partitions.csv                       # Custom app and LittleFS layout
```

The converter app currently targets macOS. The browser-based `Converter_Lab.html` is not the recommended conversion workflow for this release.
