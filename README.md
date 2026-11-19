<div align="center">

🇺🇸 **English** · 🇰🇷 [한국어](README_ko.md)

</div>

# 1.8-inch 360×360 ST77916 Round Display MJPEG Player

A dual-core MJPEG player for a 360×360 ST77916 QSPI round display and the ESP32-S3 N16R8. Depending on video size and available PSRAM, it either caches the whole compressed video in PSRAM or caches a prefix and reads the remaining frames from LittleFS.

<p align="center">
  <img width="1125" height="2436" alt="IMG_4B28A8A888C6-1" src="https://github.com/user-attachments/assets/167366d6-ce5e-472c-98b3-b08c90e7feaa" />
</p>

[YouTube demo](https://youtube.com/shorts/5IbLTVlNvVM?si=wautAE7mvhscs9il)

I found a cheap water-cooling unit without its LCD and decided to build a replacement. The display and other parts cost about $8. This project uses the ESP32-S3's dual-core CPU and 8 MiB of PSRAM to play MJPEG on the small round screen.

this is where i bought: [[https://ko.aliexpress.com/item/1005012845006769.html?spm=a2g0o.productlist.main.1.3ab64d72FYriVd&amp;algo_pvid=8d4e4e73-089d-4d7d-b6d7-667d1]](https://ko.aliexpress.com/item/1005012845006769.html?spm=a2g0o.productlist.main.1.3ab64d72FYriVd&algo_pvid=8d4e4e73-089d-4d7d-b6d7-667d178c282f&algo_exp_id=8d4e4e73-089d-4d7d-b6d7-667d178c282f-0&pdp_ext_f=%7B%22order%22%3A%2253%22%2C%22eval%22%3A%221%22%2C%22fromPage%22%3A%22search%22%7D&pdp_npi=6%40dis%21KRW%2124186%2110400%21%21%21117.39%2150.48%21%40214131c817948320791957103e105c%2112000059516890061%21sea%21KR%21906775718%21X%211%210%21n_tag%3A-29919%3Bd%3A862c4685%3Bm03_new_user%3A-29895&curPageLogUid=mneAXOfAJqJL&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005012845006769%7C_p_origin_prod%3A)

## Features and playback modes

- Three RGB565 frame buffers overlap JPEG decoding and display transfer.
- Two JPEG decoder workers run on the two CPU cores.
- Complete 16×16 JPEG blocks that fall entirely outside the circular display are skipped.
- The firmware automatically selects one of two memory modes.

| Mode | Serial label | Behavior |
|---|---|---|
| 1 | `1-full-cache` | Caches the whole video when it is within the 7.2 MiB cache limit and the PSRAM allocation succeeds. |
| 2 | `2-prefix-flash` | Reserves frame input buffers first, then caches a prefix ending at a complete JPEG frame boundary. Remaining frames are read from LittleFS. |

The 7.2 MiB value is the target limit for full-video caching. The amount that can actually be allocated depends on memory already in use, including the RGB buffers, and PSRAM fragmentation. Mode 2 supports larger videos, but playback can slow down or stutter while frames are being read from LittleFS.

The custom partition table allocates 14.75 MiB to LittleFS for video storage. This is separate from the PSRAM cache limit, so videos larger than 7.2 MiB can still be stored and played in mode 2. Filesystem overhead means the usable file space is slightly smaller than the partition size.

## Playback speed

The default `VIDEO_FPS` cap is 60 FPS. This is a playback target, not a guaranteed frame rate. Actual speed depends on scene complexity, JPEG quality, and whether the video is cached in PSRAM. In tests on this board, simple clips reached about 45 FPS, while more complex clips ran around the 20 FPS range.

MJPEG does not store a playback interval for each frame, so set `VIDEO_FPS` to match the frame rate used when converting the video. To change the playback target, edit this value in the selected sketch:

```cpp
constexpr uint32_t VIDEO_FPS = 60;
```

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

Check that the display module accepts 3.3 V power and logic levels. Pin labels can vary between modules, so verify the labels on your board before wiring it.

## Preparing a video

The firmware opens `/video.mjpeg`. Save the converted video as `data/video.mjpeg` inside the sketch folder you chose, then upload it to LittleFS.

- The file must be a sequence of 360×360 baseline JPEG frames concatenated into an MJPEG stream.
- Each JPEG frame must be no larger than 128 KiB.
- MP4 files cannot be played directly; convert them to MJPEG first.
- Match the converter's frame rate to the firmware's `VIDEO_FPS` value.
- An example video is included in the `data/video.mjpeg` file in each sketch folder.
- Audio is not supported.

## Arduino IDE setup and upload

### Requirements

- Arduino IDE with the ESP32 board support package
- Arduino_GFX library (version 1.6.9)

JPEGDEC is bundled in each sketch folder with the circular-display decoder changes, so a separate JPEGDEC installation is not required. The project was prepared with Arduino-ESP32 3.3.11 and ESP-IDF 5.5.5.

### Board settings

Select `ESP32S3 Dev Module` in Arduino IDE and use these settings:

| Setting | Value |
|---|---|
| Flash Size | 16 MB (128 Mbit) |
| PSRAM | OPI PSRAM |
| USB CDC On Boot | Enabled |
| Partition Scheme | Custom, using the `partitions.csv` in the selected sketch folder |

The custom partition table provides 768 KiB for the app and 14.75 MiB for LittleFS. Use the same partition layout for the firmware and LittleFS image.

### Flashing the firmware and video

1. Choose `ST77916_MJPEG_O2/` for the standard build or `ST77916_MJPEG_O3_Boosted/` for the optimized comparison build.
2. Open the matching `.ino` file inside that folder in Arduino IDE.
3. Replace `data/video.mjpeg` with your video. Keep the exact filename `video.mjpeg`.
4. Select the board settings above, then compile and upload the sketch.
5. Use a LittleFS upload tool to upload the **contents of the selected sketch folder's `data/` directory**. Close Serial Monitor while uploading.
6. Open Serial Monitor at 115200 baud to check the selected playback mode and frame statistics.

When using a board for the first time or changing the partition layout, upload both the firmware and the LittleFS image. When only replacing the video, upload the LittleFS image again; the firmware does not need to be re-uploaded.

## O2 and O3 builds

| Folder | Compiler option | Description |
|---|---|---|
| `ST77916_MJPEG_O2/` | `-O2` | Standard build |
| `ST77916_MJPEG_O3_Boosted/` | `-O3` | Higher-optimization comparison build |

Both sketches use the same playback code and memory policy. The compiler optimization option in `build_opt.h` is the intended difference. Keep the video and board settings the same when comparing them. O3 does not guarantee faster playback for every video.

## Serial output

At startup, the `PLAYBACK` line reports the automatically selected cache mode, frame count, and cached size. For example:

```text
PLAYBACK,mode=1-full-cache,video_bytes=...,frames=...,cached_bytes=...,cached_frames=...,input_bytes=...
```

`PLAYBACK_STATS` is printed about every two seconds. It reports actual FPS, frame interval summaries, the maximum interval, and the number of intervals at or above 100 ms. Toggle this output with the `REPORT_FRAME_STATS` setting. If the file is missing or its JPEG format is unsupported, the firmware prints an `ERROR` line.

## Repository contents

```text
ST77916_MJPEG_O2/          # Standard -O2 build and example video
ST77916_MJPEG_O3_Boosted/  # -O3 comparison build and example video
licenses/                 # Third-party licenses
THIRD_PARTY.md             # Third-party code and license notices
```

See [THIRD_PARTY.md](THIRD_PARTY.md) for third-party code and license details. Check the usage rights of any video or library you replace before redistributing it. Improvements and pull requests are welcome.
