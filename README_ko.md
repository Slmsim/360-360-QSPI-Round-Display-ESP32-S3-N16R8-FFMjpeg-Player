<div align="center">

🇰🇷 **한국어** · 🇺🇸 [English](README.md)

</div>

# 1.8인치 360×360 ST77916 원형 디스플레이 MJPEG 플레이어

ESP32-S3 N16R8으로 360×360 ST77916 원형 QSPI 디스플레이에서 MJPEG 영상을 반복 재생하는 프로젝트입니다. 영상 크기와 PSRAM 여유에 따라 압축 영상 전체를 PSRAM에 캐시하거나, 앞부분만 캐시한 뒤 나머지 프레임을 LittleFS에서 읽습니다.

<p align="center">
  <img width="1125" height="2436" alt="IMG_4B28A8A888C6-1" src="https://github.com/user-attachments/assets/72c5223f-74f9-4454-8478-961173e6c880" />
</p>

[재생 영상 보기 (YouTube Shorts)](https://youtube.com/shorts/5IbLTVlNvVM?si=wautAE7mvhscs9il)

LCD가 빠진 수냉쿨러를 당근에서 저렴하게 구해서 화면을 직접 만들었습니다. 디스플레이와 부품에 든 비용은 약 만 원입니다. ESP32-S3 N16R8의 듀얼 코어, 8 MiB PSRAM, 16 MiB 플래시를 활용해 작은 원형 화면에서 MJPEG를 재생합니다. 바이브코딩으로 여러 최적화를 하나씩 시험해 보면서 여기까지 왔습니다.

구매처:[[https://ko.aliexpress.com/item/1005012845006769.html?spm=a2g0o.productlist.main.1.3ab64d72FYriVd&amp;algo_pvid=8d4e4e73-089d-4d7d-b6d7-667d1]](https://ko.aliexpress.com/item/1005012845006769.html?spm=a2g0o.productlist.main.1.3ab64d72FYriVd&algo_pvid=8d4e4e73-089d-4d7d-b6d7-667d178c282f&algo_exp_id=8d4e4e73-089d-4d7d-b6d7-667d178c282f-0&pdp_ext_f=%7B%22order%22%3A%2253%22%2C%22eval%22%3A%221%22%2C%22fromPage%22%3A%22search%22%7D&pdp_npi=6%40dis%21KRW%2124186%2110400%21%21%21117.39%2150.48%21%40214131c817948320791957103e105c%2112000059516890061%21sea%21KR%21906775718%21X%211%210%21n_tag%3A-29919%3Bd%3A862c4685%3Bm03_new_user%3A-29895&curPageLogUid=mneAXOfAJqJL&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005012845006769%7C_p_origin_prod%3A)

**사용 방법**

1. 아래 배선표에 따라 디스플레이를 연결합니다.
2. macOS 컨버터 앱에서 영상을 변환해 `data/video.mjpeg`로 넣습니다.
3. Arduino IDE에서 보드 설정을 맞추고 O3 펌웨어를 업로드합니다.
4. 시리얼 모니터를 닫고 LittleFS로 영상을 올립니다.

## 기능과 재생 모드

- RGB565 프레임 버퍼 3개로 디코딩과 화면 전송을 겹쳐 처리합니다.
- JPEG 디코더 워커 2개가 CPU 두 코어에서 실행됩니다.
- 360×360 baseline JPEG에서 16×16 MCU가 원 밖에 완전히 놓이면 역변환과 색 변환을 생략합니다. 뒤에 나오는 MCU를 읽을 수 있도록 압축 데이터는 계속 읽습니다. 이 MCU의 RGB565 픽셀은 PSRAM에 복사하지 않으며 원 테두리에 걸친 MCU는 그대로 처리합니다.
- JPEG 출력 버퍼를 **가로 23 MCU(5,888픽셀)**로 넓혀 16픽셀 높이의 한 띠를 콜백 한 번으로 넘깁니다.
- ESP32-S3의 SIMD **rows2 IDCT**는 조건을 만족하는 JPEG 블록의 행 방향 역변환을 가속합니다. 나머지 블록은 기존 스칼라 경로에서 처리합니다.
- 영상에 따라 두 메모리 모드 중 하나를 자동으로 선택합니다.

| 모드 | 시리얼 로그 | 동작 |
|---|---|---|
| 1 | `1-full-cache` | 영상이 7.2 MiB 캐시 한도 안에 있고 PSRAM 할당에 성공하면 전체 영상을 캐시합니다. |
| 2 | `2-prefix-flash` | 프레임 입력 버퍼를 먼저 확보한 뒤, 완전한 JPEG 프레임 경계까지 앞부분을 캐시합니다. 나머지 프레임은 LittleFS에서 읽습니다. |

7.2 MiB는 전체 영상 캐시의 목표 한도입니다. RGB 버퍼 등 이미 사용 중인 메모리와 PSRAM 단편화에 따라 실제 할당량은 달라질 수 있습니다. 2번 모드는 더 큰 영상을 재생할 수 있지만 LittleFS에서 읽는 구간에서 속도가 낮아지거나 끊김이 생길 수 있습니다.

커스텀 파티션은 영상 저장용 LittleFS에 14.75 MiB를 배정합니다. PSRAM 캐시와 별도의 공간이라 7.2 MiB를 넘는 영상도 LittleFS에 저장해 2번 모드로 재생할 수 있습니다. 파일시스템 오버헤드 때문에 실제 파일에 쓸 수 있는 용량은 파티션 크기보다 조금 작습니다.

## 재생 속도

기본 `VIDEO_FPS` 상한은 60 FPS입니다. O3 배포판에서 시험 영상을 PSRAM에 캐시했을 때 **최대 약 50 FPS**를 기록했습니다. 실제 재생 속도는 장면 복잡도, JPEG 화질, 캐시 모드에 따라 달라집니다.

MJPEG에는 프레임별 재생 간격이 저장되지 않으므로, 변환할 때 설정한 FPS와 `VIDEO_FPS`를 맞춰 주세요. 재생 목표 속도를 바꾸려면 `ST77916_MJPEG_O3.ino`에서 다음 값을 수정하면 됩니다.

```cpp
constexpr uint32_t VIDEO_FPS = 60;
```

## 하드웨어와 배선

- ESP32-S3 N16R8 보드 (16 MiB Flash, 8 MiB OPI PSRAM)
- 360×360 ST77916 QSPI 원형 디스플레이

| 디스플레이 | ESP32-S3 |
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
| TE | 연결하지 않음 |

디스플레이 모듈의 전원과 로직 레벨이 3.3 V를 지원하는지 확인해 주세요. 보드마다 핀 표기가 다를 수 있으니 실제 모듈의 라벨도 확인하는 것이 좋습니다.

## 영상 파일 준비

`ST77916 MJPEG Converter.app`에서 FFmpeg로 원본 영상을 변환합니다. 동봉된 앱은 macOS 12 이상의 Apple Silicon Mac에서 실행되며, 별도로 설치된 FFmpeg를 사용합니다. Homebrew가 지원되는 macOS에서는 `brew install ffmpeg`로 설치할 수 있습니다. [Homebrew FFmpeg](https://formulae.brew.sh/formula/ffmpeg) 설치 후 영상 변환은 앱에서 진행하면 됩니다.

1. 앱을 열어 영상을 선택하고 구간, 화면 맞춤 방식, 출력 FPS를 설정합니다. 출력 FPS는 스케치의 `VIDEO_FPS`와 맞춰 주세요.
2. 처음에는 **Create q7 and q8 comparison files**(q7·q8 비교 파일 두 개 만들기)를 켜 보세요. 같은 구간을 `q7.mjpeg`, `q8.mjpeg`로 저장합니다. **q7은 화질 우선**, **q8은 재생 성능 우선**으로 시험해 보세요. q8은 보통 파일이 더 작고 디코딩 시간이 줄어들 수 있습니다. 같은 보드에서 화면 화질, 파일 크기, `fps`, `decode_avg_ms`를 비교하면 됩니다.
3. **Open output folder**(저장 폴더 열기)에서 비교 파일 하나를 골라 스케치의 `data/video.mjpeg`로 복사한 뒤 LittleFS에 올립니다. 다른 파일을 시험할 때도 같은 경로에 덮어쓰고 다시 올립니다. 펌웨어는 항상 `/video.mjpeg`를 엽니다.

일반 변환에서는 비교 옵션을 끄면 `video.mjpeg` 하나가 만들어집니다. 기본 JPEG 화질은 **85%(q7)**입니다. 결과가 전체 캐시에 담기에는 너무 크다면 구간을 줄이거나 단일 파일 모드에서 **Adjust quality to target 7.2 MiB**(7.2 MiB 목표로 품질 자동 조절)를 사용해 보세요.

- 영상 파일은 360×360 baseline JPEG 프레임을 이어 붙인 형태입니다. 앱은 4:2:0 색차 샘플링(`yuvj420p`)으로 인코딩해 원형 화면 최적화에 쓰이는 16×16 MCU를 만듭니다.
- JPEG 한 프레임의 최대 크기는 128 KiB입니다. MP4는 먼저 변환해야 하며 오디오는 재생하지 않습니다.
- 예제 영상은 `data/video.mjpeg`에 들어 있습니다.

## Arduino IDE 설정 및 업로드

### 필요한 도구

- Arduino IDE와 ESP32 보드 지원 패키지
- Arduino_GFX 라이브러리 (버전 1.6.9)

원형 화면 디코더 수정 사항을 포함한 JPEGDEC 소스가 스케치 폴더에 들어 있으므로 별도 JPEGDEC 설치는 필요하지 않습니다. Arduino-ESP32 3.3.11 및 ESP-IDF 5.5.5 환경을 기준으로 정리했습니다.

### 보드 설정

Arduino IDE에서 `ESP32S3 Dev Module`을 선택하고 다음 설정을 사용합니다.

| 설정 | 값 |
|---|---|
| Flash Size | 16 MB (128 Mbit) |
| PSRAM | OPI PSRAM |
| USB CDC On Boot | Enabled |
| Partition Scheme | Custom, 스케치 폴더의 `partitions.csv` 사용 |

커스텀 파티션은 앱 768 KiB와 LittleFS 14.75 MiB를 배정합니다. 펌웨어와 LittleFS 이미지를 같은 파티션 구성으로 올려야 합니다.

### 펌웨어와 영상 올리기

1. 배포 파일의 압축을 풉니다. GitHub에서 받은 폴더 이름에 `-main`이 붙어 있거나 이름이 다르면, 스케치가 들어 있는 폴더를 `ST77916_MJPEG_O3`로 바꿔 `.ino` 파일명과 맞춥니다.
2. 폴더 안의 `ST77916_MJPEG_O3.ino`를 Arduino IDE에서 엽니다.
3. `data/video.mjpeg`를 원하는 영상으로 교체합니다. 파일명은 정확히 `video.mjpeg`여야 합니다.
4. 위 보드 설정을 적용하고 스케치를 컴파일·업로드합니다.
5. LittleFS 업로드 도구로 **스케치의 `data/` 폴더 내용**을 업로드합니다. 업로드 중에는 시리얼 모니터를 닫아 둡니다.
6. 시리얼 모니터를 115200 baud로 열어 재생 모드와 프레임 통계를 확인합니다.

새 보드에 처음 올리거나 파티션 구성을 바꿀 때는 펌웨어와 LittleFS 이미지를 모두 업로드해 주세요. 영상만 교체한다면 LittleFS 이미지만 다시 올리면 됩니다.

## O3 배포 빌드

배포판은 `build_opt.h`에서 `-O3` 컴파일 옵션을 사용합니다. 위에서 설명한 와이드 JPEG 버퍼, 원형 화면에 맞춘 PSRAM 복사, ESP32-S3 SIMD IDCT도 포함되어 있습니다. 이전 빌드와 비교할 때는 영상과 보드 설정을 같게 맞춰 주세요.

## 시리얼 로그

시작 시 `PLAYBACK` 줄에서 자동 선택된 캐시 모드, 영상 프레임 수와 캐시된 크기를 확인할 수 있습니다.

```text
PLAYBACK,mode=1-full-cache,video_bytes=...,frames=...,cached_bytes=...,cached_frames=...,input_bytes=...
```

부팅 로그에서 `WIDECIRCLE_IDCT,profile=1,rows2=1`이면 SIMD 경로가 활성화된 상태입니다. `JPEG_OUTPUT_BUFFER,mcu_per_band=23,buffer_pixels=5888,circle_copy_skip=1`에서는 와이드 버퍼와 원 밖 복사 생략 설정을 확인할 수 있습니다.

`PLAYBACK_STATS`는 약 2초마다 실제 FPS, 프레임 간격 요약, 최대 간격, 100 ms 이상 지연된 간격 수와 평균 디코딩 시간(`decode_avg_ms`)을 출력합니다. `REPORT_FRAME_STATS` 설정으로 출력을 켜거나 끌 수 있습니다. `/video.mjpeg`가 없으면 `ERROR`가 출력됩니다.

## 저장소 구성

```text
ST77916_MJPEG_O3.ino          # O3 재생 스케치
ST77916 MJPEG Converter.app/  # macOS FFmpeg 컨버터
data/video.mjpeg              # 예제 영상
JPEGDEC_Circle.*              # 원형 출력 기능을 포함한 JPEG 디코더
SimdIDCT.h, s3_idct_rows2.S    # ESP32-S3 IDCT SIMD
ST77916Panel.h, PanelInit.h   # QSPI 화면 전송 및 초기화
partitions.csv                # 앱과 LittleFS 파티션 구성
```

동봉된 디코더는 Larry Bank / BitBank Software의 JPEGDEC를 바탕으로 수정했습니다. QSPI 명령 형식은 Arduino_GFX 구현을 따릅니다. [JPEGDEC 소스와 라이선스](https://github.com/bitbank2/JPEGDEC), [Arduino_GFX 소스와 라이선스](https://github.com/moononournation/Arduino_GFX)

영상이나 라이브러리를 교체해 공개할 때는 각 파일의 사용 권한도 확인해 주세요. 개선 사항은 Pull Request로 공유해 주시면 좋겠습니다.
