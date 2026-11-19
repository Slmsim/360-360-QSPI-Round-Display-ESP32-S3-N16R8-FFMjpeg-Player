<div align="center">

🇰🇷 **한국어** · 🇺🇸 [English](README.md)

</div>

# ST77916 360×360 원형 디스플레이 MJPEG 플레이어

ESP32-S3 N16R8에서 ST77916 QSPI 원형 디스플레이로 360×360 MJPEG 영상을 재생하는 펌웨어입니다. 이 폴더에는 현재 O3 배포용 스케치, 예제 영상, macOS 컨버터 앱이 들어 있습니다.
<img width="1125" height="2436" alt="IMG_4B28A8A888C6-1" src="https://github.com/user-attachments/assets/72c5223f-74f9-4454-8478-961173e6c880" />

[YouTube demo](https://youtube.com/shorts/5IbLTVlNvVM?si=wautAE7mvhscs9il)


## 포함된 기능

- RGB565 프레임 버퍼 3개와 듀얼 코어 JPEG 디코딩으로 디코딩과 화면 전송을 겹쳐 처리합니다.
- JPEG 출력 버퍼를 23 MCU 폭(5,888픽셀)으로 넓혀 기존 좁은 버퍼보다 출력 콜백 횟수를 줄였습니다.
- ESP32-S3 SIMD `rows2` IDCT 경로를 기본으로 사용하며, 지원하지 않는 블록이나 환경에서는 스칼라 경로로 처리합니다.
- 원형 화면 최적화: 원 밖에 완전히 놓인 MCU는 디코딩을 건너뛰고, 디코더가 만든 픽셀 중 원 밖 영역은 PSRAM 프레임 버퍼에 복사하지 않습니다. 원 테두리와 겹치는 MCU는 디코딩합니다.
- PSRAM 캐시를 자동으로 선택합니다. 압축 영상 전체가 7.2 MiB 목표 한도 안에 들어가면 전체를 캐시하고, 넘으면 앞부분을 캐시한 뒤 나머지는 LittleFS에서 읽습니다.
- FFmpeg로 보드용 MJPEG를 만드는 macOS 앱 `ST77916 MJPEG Converter.app`이 포함되어 있습니다. 이 배포판에서는 이전 브라우저 HTML 변환기 대신 앱을 사용해 주세요.

이 최적화는 소프트웨어 연산과 버퍼 처리 부담을 줄여 주지만 일정한 FPS를 보장하지는 않습니다. 이 보드에서 시험한 영상은 **최대 약 50 FPS**에 도달했습니다. 스케치의 `VIDEO_FPS`는 60 FPS 상한이며 모든 영상이 60 FPS로 재생된다는 뜻은 아닙니다. 실제 속도는 장면 복잡도, JPEG 화질, 캐시 모드와 보드 상태에 따라 달라집니다.

## 영상 변환하기

1. Mac에서 `ST77916 MJPEG Converter.app`을 열고 MP4 등 입력 영상을 선택합니다.
2. 출력 FPS를 `ST77916_MJPEG_O3.ino`의 `VIDEO_FPS`와 맞춥니다. 기본값은 60 FPS입니다. 이는 재생 목표이며 보드가 더 낮은 속도로 재생할 수 있습니다.
3. 먼저 **q7·q8 비교 파일 두 개 만들기**를 켜고 같은 영상 구간을 변환합니다. 출력 폴더에 `q7.mjpeg`와 `q8.mjpeg`가 만들어집니다.
4. 화질을 우선하면 **q7**을 먼저 시험하고, 파일 크기와 디코딩 부담을 줄이는 쪽을 시험하려면 **q8**을 사용해 보세요. 같은 영상과 보드 설정으로 두 결과를 비교하면 됩니다. q 숫자는 JPEG 양자화 계수이며 보편적인 화질 등급은 아닙니다. 보통 q8은 q7보다 세부 화질을 조금 낮추는 대신 스트림 크기를 줄입니다.
5. 펌웨어가 찾는 파일은 `/video.mjpeg` 하나입니다. `q7.mjpeg` 또는 `q8.mjpeg` 중 시험할 파일 하나를 이 스케치의 `data/` 폴더에 복사하고 이름을 `video.mjpeg`로 바꾼 다음 LittleFS 이미지를 업로드합니다. 두 비교 파일을 동시에 `/video.mjpeg`로 사용할 수는 없습니다.

비교 모드를 끄면 파일 하나만 만들 수 있습니다. 앱의 화질 슬라이더는 퍼센트로 표시하며 기본값 **85%는 q7에 해당**합니다. 퍼센트는 앱 내부 조절 눈금이라 사람이 느끼는 화질이 같은 비율로 변한다는 의미는 아닙니다. 컨버터는 펌웨어의 16×16 MCU 원형 스킵 경로에 맞춰 baseline JPEG, 4:2:0 샘플링(`yuvj420p`)으로 인코딩합니다. 오디오는 포함되지 않습니다.

JPEG 한 프레임은 128 KiB 이하여야 합니다. 변환 앱이 크기를 추정하지만, 변환 결과와 보드 시리얼 로그도 확인해 주세요. 전체 영상 PSRAM 캐시의 목표 한도는 7.2 MiB입니다. 이보다 큰 영상은 일부만 캐시하고 나머지를 LittleFS에서 읽을 수 있으며, 이 경우 재생 속도가 낮아지거나 일정하지 않을 수 있습니다.

## 하드웨어 및 배선

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

디스플레이 모듈이 3.3 V 전원과 로직 레벨을 지원하는지 확인해 주세요. 모듈마다 핀 표기가 다를 수 있으니 실제 보드의 라벨도 확인해야 합니다.

## Arduino IDE 설정 및 업로드

### 필요한 도구

- Arduino IDE 및 ESP32 보드 지원 패키지
- Arduino_GFX 라이브러리 1.6.9

원형 화면용으로 수정한 JPEGDEC가 이 폴더에 포함되어 있어 별도로 설치할 필요가 없습니다. Arduino-ESP32 3.3.11 및 ESP-IDF 5.5.5 환경을 기준으로 준비했습니다.

### 보드 설정

Arduino IDE에서 **ESP32S3 Dev Module**을 선택하고 다음과 같이 설정합니다.

| 설정 | 값 |
|---|---|
| Flash Size | 16 MB (128 Mbit) |
| PSRAM | OPI PSRAM |
| USB CDC On Boot | Enabled |
| Partition Scheme | Custom, 이 폴더의 `partitions.csv` 사용 |

커스텀 파티션은 앱에 768 KiB, LittleFS에 14.75 MiB를 배정합니다. 펌웨어와 파일시스템을 올릴 때 같은 파티션 구성을 사용해야 합니다.

### 펌웨어와 영상 업로드

1. 이 폴더의 `ST77916_MJPEG_O3.ino`를 Arduino IDE에서 엽니다.
2. 선택한 영상을 `data/video.mjpeg`로 넣습니다. 파일명은 정확히 `video.mjpeg`여야 합니다.
3. 위 보드 설정으로 스케치를 컴파일하고 업로드합니다.
4. LittleFS 업로드 도구로 **이 폴더 `data/` 안의 내용**을 업로드합니다. 업로드 중에는 시리얼 모니터를 닫아 둡니다.
5. 시리얼 모니터를 115200 baud로 열어 캐시 모드와 프레임 수를 확인합니다. `PLAYBACK_STATS`에 약 2초마다 재생 측정값이 출력됩니다.

처음 설정하거나 파티션 구성을 바꿀 때는 펌웨어와 LittleFS 이미지를 모두 업로드합니다. 영상만 교체할 때는 LittleFS 이미지만 다시 올리면 되고 펌웨어는 다시 올리지 않아도 됩니다.

## 재생 및 시리얼 출력

스케치의 `VIDEO_FPS`는 재생 상한이며 기본값은 60입니다. 컨버터의 출력 FPS도 같은 값으로 맞춰 주세요. 디스플레이가 그 속도를 유지하지 못하면 보드가 처리할 수 있는 속도로 재생하며, 시험 영상에서 관찰한 최대치는 약 50 FPS였습니다.

시작 시 `PLAYBACK`에서 영상 크기, 프레임 수, 캐시 모드를 확인할 수 있습니다. `1-full-cache`는 전체 스트림을 PSRAM에 캐시했다는 뜻이고, `2-prefix-flash`는 앞부분만 캐시하고 나머지 프레임은 LittleFS에서 읽는다는 뜻입니다. `PLAYBACK_STATS`에는 측정 FPS, 평균 프레임 간격, p95 상한 간격, 최대 간격, 긴 간격 횟수, 평균 JPEG 디코딩 시간이 나옵니다. 이 값은 현재 영상과 설정에서 측정한 결과이며 성능 보장값은 아닙니다.

## 폴더 구성

```text
ST77916_MJPEG_O3.ino                  # O3 펌웨어 시작 파일
ST77916 MJPEG Converter.app/          # macOS FFmpeg 컨버터
data/video.mjpeg                      # 예제 영상; 직접 만든 영상으로 교체 가능
JPEGDEC_Circle.*                      # 원형 디스플레이용 JPEG 디코더
SimdIDCT.h, s3_idct_rows2.S            # ESP32-S3 IDCT SIMD 경로
JpegPixelOutput.h                      # RGB565 출력 및 원형 PSRAM 복사 최적화
partitions.csv                        # 앱 및 LittleFS 파티션 구성
```

컨버터 앱은 현재 macOS용입니다. 이 배포판에서는 브라우저 기반 `Converter_Lab.html` 대신 앱을 사용하는 것을 권장합니다.
