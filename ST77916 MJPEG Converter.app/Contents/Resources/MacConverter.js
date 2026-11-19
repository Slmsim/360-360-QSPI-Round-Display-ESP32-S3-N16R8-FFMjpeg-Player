'use strict';

{
  const bridge = window.webkit.messageHandlers.converter;
  const labels = {
    ko: {
      nativeTitle: '360×360 MJPEG 변환기 · FFmpeg',
      nativeIntro: '영상을 고르고 구간·화질을 조절해 보드용 video.mjpeg로 저장해.',
      nativeSelect: '파일 선택 버튼으로 원본 영상을 골라줘.',
      nativeHint: 'MP4 권장 · 원본은 맥에서 처리해.',
      nativeFPS: '1~60 FPS · 스케치의 VIDEO_FPS에 맞춰줘.',
      nativeQuality: 'JPEG 화질 (%)',
      nativeQualityHelp: '높을수록 선명하고 파일이 커져. 용량을 줄이려면 화질을 낮춰줘.',
      nativePair: 'q7·q8 비교 파일 두 개 만들기',
      nativePairHelp: '켜면 같은 구간을 q=7과 q=8로 저장해. 기본 재생은 video.mjpeg 한 개를 사용해.',
      nativeLight: '7.2 MiB 목표로 품질 자동 조절',
      nativeLightHelp: '선택 구간을 유지하고 필요하면 화질을 낮춰. JPEG 한 장은 20 KiB 이하를 목표로 해.',
      nativePairLight: 'q7·q8 비교에서는 두 화질 계수를 고정해.',
      nativeMask: '원 바깥을 검정으로 채우고 화면 가장자리에 2픽셀 여유를 남겨.',
      nativeFull: '전체 영상 선택',
      nativeRange: '선택 구간 {duration}초 / 전체 {total}초',
      nativeConvert: 'video.mjpeg 만들기',
      nativeConvertPair: 'q7·q8 두 파일 만들기',
      nativeEstimate: '예상 용량 계산',
      nativeSettings: '구간의 시작과 끝을 직접 바꿀 수 있어. 소리는 저장되지 않아.',
      nativeReady: '구간과 화질을 확인한 뒤 변환해줘.',
      nativeProgress: '화질 {quality} · {phase} 중 · {percent}%',
      nativeEncoding: '변환',
      nativeSampling: '샘플 분석',
      nativeFinished: '변환 완료',
      nativeResult: '{name} · 화질 {quality} · {frames}프레임 · {duration}초 · {size} MiB · 최대 JPEG {largest} KiB',
      nativeFolder: '저장 폴더 열기',
      nativeCache: '7.2 MiB를 넘는 파일이 있어. 전체 캐시가 필요하면 구간을 줄이거나 자동 품질을 켜줘.',
      nativeCapacity: 'LittleFS 관리 정보와 기존 파일도 공간을 사용해. 업로드할 파일들의 합을 확인해줘.',
      nativeEstimateSize: '예상 용량 {size} MiB',
      nativeEstimateRow: '{name} · 화질 {quality} · 약 {size} MiB · {samples}장 샘플',
      nativeEstimateHelp: '실제 FFmpeg로 최대 16장을 분석한 추정이야. 장면 변화에 따라 최종 크기가 달라질 수 있어.',
      nativeBoard: '보드 FPS는 시리얼 PLAYBACK_STATS에서 확인해줘.',
      nativeEstimateDone: '용량 분석 완료',
      nativeMissing: 'FFmpeg를 찾지 못했어. FFmpeg를 설치한 뒤 다시 시도해줘.',
      nativeInvalid: '영상과 시작·끝 구간을 다시 확인해줘.',
      nativeBusy: '이미 작업 중이야.',
      nativeCancelled: '작업을 취소했어.',
      nativeMismatch: '두 파일의 프레임 수가 달라. 구간이나 원본을 확인해줘.',
      nativeLarge: '용량 목표를 맞출 수 없어. 구간을 줄이거나 화질을 낮춰줘.',
      nativeFailed: 'FFmpeg 작업에 실패했어. {detail}',
      nativeOpenFailed: '저장 폴더를 열 수 없어. 아래 경로를 확인해줘.',
      nativeSteps: '파일 선택 → 구간·화질 설정 → 예상 용량 확인 → 변환',
      nativeUpload: 'video.mjpeg를 스케치의 data/에 넣고 LittleFS를 올려줘. q7·q8을 따로 만들었다면 시험할 파일 하나를 video.mjpeg로 복사해서 올려줘.',
      nativeFooter: 'FFmpeg는 이 맥에서 실행돼. 영상과 결과는 맥에 저장돼.'
    },
    en: {
      nativeTitle: '360×360 MJPEG Converter · FFmpeg',
      nativeIntro: 'Choose a video, set its range and quality, and save video.mjpeg for your board.',
      nativeSelect: 'Choose a source video.',
      nativeHint: 'MP4 recommended. Conversion runs on your Mac.',
      nativeFPS: '1–60 FPS. Match the sketch’s VIDEO_FPS.',
      nativeQuality: 'JPEG quality (%)',
      nativeQualityHelp: 'Higher values retain more detail and use more space. Lower quality for smaller files.',
      nativePair: 'Create q7 and q8 comparison files',
      nativePairHelp: 'Encode the same range at q=7 and q=8. Standard playback uses one video.mjpeg.',
      nativeLight: 'Adjust quality to target 7.2 MiB',
      nativeLightHelp: 'Keeps your range and lowers quality if needed. Targets at most 20 KiB per JPEG.',
      nativePairLight: 'q7/q8 comparison uses fixed quality factors.',
      nativeMask: 'Black out the corners with a two-pixel margin around the display.',
      nativeFull: 'Select entire video',
      nativeRange: 'Selected {duration}s / total {total}s',
      nativeConvert: 'Create video.mjpeg',
      nativeConvertPair: 'Create both q7 and q8',
      nativeEstimate: 'Estimate file size',
      nativeSettings: 'Edit the start and end times to choose a range. Audio is omitted.',
      nativeReady: 'Check the range and quality, then convert.',
      nativeProgress: 'Quality {quality} · {phase} · {percent}%',
      nativeEncoding: 'Encoding',
      nativeSampling: 'Sampling',
      nativeFinished: 'Conversion complete',
      nativeResult: '{name} · quality {quality} · {frames} frames · {duration}s · {size} MiB · largest JPEG {largest} KiB',
      nativeFolder: 'Open output folder',
      nativeCache: 'A file exceeds 7.2 MiB. Shorten the range or enable automatic quality for full caching.',
      nativeCapacity: 'LittleFS metadata and existing files use space too. Check the total size before uploading.',
      nativeEstimateSize: 'Estimated size {size} MiB',
      nativeEstimateRow: '{name} · quality {quality} · about {size} MiB · {samples} sampled frames',
      nativeEstimateHelp: 'An estimate from up to 16 frames encoded by FFmpeg. Final size depends on scene changes.',
      nativeBoard: 'Check board FPS in the serial PLAYBACK_STATS output.',
      nativeEstimateDone: 'File size estimate complete',
      nativeMissing: 'FFmpeg was not found. Install it and try again.',
      nativeInvalid: 'Check the source and start/end times.',
      nativeBusy: 'A job is already running.',
      nativeCancelled: 'Job cancelled.',
      nativeMismatch: 'The files have different frame counts. Check the range or source.',
      nativeLarge: 'Could not meet the size target. Shorten the range or lower quality.',
      nativeFailed: 'FFmpeg failed. {detail}',
      nativeOpenFailed: 'Could not open the output folder. Check the path below.',
      nativeSteps: 'Choose file → Set range and quality → Estimate size → Convert',
      nativeUpload: 'Put video.mjpeg in the sketch’s data/ folder and upload LittleFS. To compare q7/q8 outputs, copy one at a time as video.mjpeg and upload it.',
      nativeFooter: 'FFmpeg runs on this Mac. The videos and results stay on your Mac.'
    }
  };
  Object.assign(TEXT.ko, labels.ko);
  Object.assign(TEXT.en, labels.en);
  TEXT.ko.statusReady = labels.ko.nativeReady;
  TEXT.en.statusReady = labels.en.nativeReady;
  TEXT.ko.pageTitle = labels.ko.nativeTitle;
  TEXT.en.pageTitle = labels.en.nativeTitle;
  const originalInvalidate = invalidateEstimate;
  invalidateEstimate = () => {
    originalInvalidate();
    $('fpsEstimate').textContent = t('nativeBoard');
  };

  const setLabel = (element, key) => { element.dataset.i18n = key; };
  for (const [element, key] of [
    [document.querySelector('h1'), 'nativeTitle'],
    [document.querySelector('.intro'), 'nativeIntro'],
    [$('placeholder'), 'nativeSelect'],
    [document.querySelector('[data-i18n=videoHint]'), 'nativeHint'],
    [$('sourceInfo'), 'nativeHint'],
    [document.querySelector('[data-i18n=fpsHint]'), 'nativeFPS'],
    [document.querySelector('[data-i18n=qualityLabel]'), 'nativeQuality'],
    [document.querySelector('[data-i18n=lightweightLabel]'), 'nativeLight'],
    [document.querySelector('[data-i18n=lightweightHelp]'), 'nativeLightHelp'],
    [document.querySelector('[data-i18n=roundMaskHelp]'), 'nativeMask'],
    [document.querySelector('[data-i18n=settingsNote]'), 'nativeSettings'],
    [$('estimate'), 'nativeEstimate'],
    [document.querySelector('footer'), 'nativeFooter'],
    [document.querySelector('[data-i18n=instructionsSteps]'), 'nativeSteps']
  ]) setLabel(element, key);
  $('placeholder').replaceChildren();
  const instructions = document.querySelector('.instructions');
  for (const paragraph of Array.from(instructions.children).slice(1)) paragraph.remove();
  const uploadHint = document.createElement('p');
  setLabel(uploadHint, 'nativeUpload');
  instructions.append(uploadHint);

  const pairField = document.createElement('div');
  pairField.className = 'field';
  pairField.innerHTML = '<label class="check"><input type="checkbox" id="pair"><span data-i18n="nativePair"></span></label><small data-i18n="nativePairHelp"></small>';
  $('quality').closest('.row').after(pairField);
  const qHelp = document.createElement('small');
  setLabel(qHelp, 'nativeQualityHelp');
  $('quality').closest('.field').append(qHelp);
  Object.assign($('quality'), { min: '1', max: '100', value: '85' });
  $('lightweight').checked = false;
  Object.assign($('fps'), { max: '60', value: '60' });

  const rangeField = document.createElement('div');
  rangeField.className = 'field';
  rangeField.innerHTML = '<button type="button" id="fullRange" class="secondary" data-i18n="nativeFull" disabled></button><small id="selectedRange"></small>';
  $('end').closest('.row').after(rangeField);

  // WebKit's native file picker supplies the app with the chosen source path.
  for (const type of ['dragenter', 'dragover', 'drop']) {
    window.addEventListener(type, event => {
      event.preventDefault();
      event.stopImmediatePropagation();
    }, true);
  }
  $('upload').addEventListener('change', async event => {
    event.stopImmediatePropagation();
    const loading = loadVideo($('upload').files[0]);
    const current = loadController;
    await loading;
    if (loaded && current === loadController) selectEntireVideo();
    refreshControls();
  }, true);

  function selectEntireVideo() {
    $('start').value = '0';
    $('end').value = String(video.duration);
    invalidateEstimate();
    previewAt(0);
    refreshControls();
  }
  $('fullRange').addEventListener('click', selectEntireVideo);

  // Anchor the app's quality scale at 85% = q7, the previous default encoding.
  // Endpoints remain 100% = q2 and 1% = q31.
  function qualityToQuantizer(percent) {
    if (percent >= 85) return 2 + Math.round((100 - percent) / 3);
    return 7 + Math.round((85 - percent) * 24 / 84);
  }
  function quantizerToQuality(quantizer) {
    if (quantizer <= 7) return 100 - (quantizer - 2) * 3;
    return 85 - Math.round((quantizer - 7) * 84 / 24);
  }
  function qualityLabel(quantizer, settings) {
    const percent = settings && !settings.pair && quantizer === settings.quality
      ? settings.qualityPercent : quantizerToQuality(quantizer);
    return settings?.pair ? 'q' + quantizer + ' (' + percent + '%)' : percent + '%';
  }
  let activeSettings = null;
  readConfiguration = () => {
    const start = Number($('start').value), end = Number($('end').value);
    const fps = Number($('fps').value), qualityPercent = Number($('quality').value);
    if (!loaded || !$('start').value || !$('end').value ||
        !Number.isFinite(start) || !Number.isFinite(end) ||
        start < 0 || end <= start || end > video.duration + 0.001 ||
        !Number.isInteger(fps) || fps < 1 || fps > 60 ||
        !Number.isInteger(qualityPercent) || qualityPercent < 1 || qualityPercent > 100) {
      throw uiError('nativeInvalid');
    }
    return {
      start, end: Math.min(end, video.duration), fps, qualityPercent,
      quality: qualityToQuantizer(qualityPercent),
      fit: $('fit').value, roundMask: $('roundMask').checked,
      pair: $('pair').checked, lightweight: $('lightweight').checked && !$('pair').checked
    };
  };

  function refreshControls() {
    const pair = $('pair').checked;
    $('qualityValue').value = pair
      ? quantizerToQuality(7) + '% / ' + quantizerToQuality(8) + '%'
      : $('quality').value + '%';
    $('quality').disabled = busy || pair;
    $('lightweight').disabled = busy || pair;
    const help = document.querySelector('[data-i18n=nativeLightHelp], [data-i18n=nativePairLight]');
    setLabel(help, pair ? 'nativePairLight' : 'nativeLightHelp');
    help.textContent = t(pair ? 'nativePairLight' : 'nativeLightHelp');
    setLabel($('convert'), pair ? 'nativeConvertPair' : 'nativeConvert');
    $('convert').textContent = t(pair ? 'nativeConvertPair' : 'nativeConvert');
    $('fullRange').disabled = busy || !loaded;
    $('selectedRange').textContent = loaded ? t('nativeRange', {
      duration: Math.max(0, Number($('end').value) - Number($('start').value)).toFixed(2),
      total: video.duration.toFixed(2)
    }) : '';
  }
  $('settings').addEventListener('input', event => {
    if (event.target === $('upload')) return;
    event.stopImmediatePropagation();
    invalidateEstimate();
    $('fpsValue').value = $('fps').value + ' fps';
    refreshControls();
    if (loaded && ['fit', 'roundMask'].includes(event.target.id)) drawFrame();
    if (event.target === $('start')) previewAt(Number($('start').value));
  }, true);

  const folderButton = document.createElement('button');
  folderButton.id = 'download';
  folderButton.type = 'button';
  setLabel(folderButton, 'nativeFolder');
  $('download').replaceWith(folderButton);
  folderButton.addEventListener('click', async () => {
    if (!await bridge.postMessage({ action: 'openFolder' })) status('nativeOpenFailed', true);
  });

  let nativeResult = null;
  const mib = size => (size / (1024 * 1024)).toFixed(2);
  function renderNativeResult() {
    if (!nativeResult) return;
    const { folder, files, fps } = nativeResult;
    $('resultTitle').textContent = t('nativeFinished');
    $('resultDetails').textContent = files.map(file => t('nativeResult', {
      ...file, quality: qualityLabel(file.quality, nativeResult.requestedSettings),
      size: mib(file.bytes), duration: (file.frames / fps).toFixed(2),
      largest: (file.largest / 1024).toFixed(1)
    })).join('\n');
    $('resultDetails').style.whiteSpace = 'pre-line';
    $('resultNote').textContent = (files.some(file => file.bytes > CACHE_LIMIT) ? t('nativeCache') + '\n' : '') +
      t('nativeCapacity') + '\n' + folder;
    $('resultNote').style.whiteSpace = 'pre-line';
    $('resultNote').style.overflowWrap = 'anywhere';
    $('fpsCode').textContent = 'constexpr uint32_t VIDEO_FPS = ' + fps + ';';
    $('result').hidden = false;
  }
  showEstimate = value => {
    const total = value.files.reduce((sum, file) => sum + file.bytes, 0);
    $('estimateSummary').textContent = t('nativeEstimateSize', { size: mib(total) });
    $('estimateDetails').textContent = value.files.map(file => t('nativeEstimateRow', {
      ...file, quality: qualityLabel(file.quality, value.requestedSettings), size: mib(file.bytes)
    })).join('\n');
    $('estimateDetails').style.whiteSpace = 'pre-line';
    $('fpsEstimate').textContent = t('nativeBoard');
    $('estimateAdvice').textContent = t('nativeEstimateHelp');
    $('estimateBox').classList.toggle('over', total >= FILE_LIMIT);
    $('estimateBox').classList.toggle('warning', value.files.some(file => file.bytes > CACHE_LIMIT));
  };
  const originalLanguage = applyLanguage;
  applyLanguage = next => {
    originalLanguage(next);
    refreshControls();
    renderNativeResult();
  };
  // Existing language listeners resolve the updated global function at click time.
  window.nativeConversionProgress = (quality, fraction) => {
    if (!runController || runController.signal.aborted) return;
    $('progress').value = Math.floor(fraction * 100);
    status(() => t('nativeProgress', {
      quality: qualityLabel(quality, activeSettings), percent: Math.floor(fraction * 100),
      phase: t(busyMode === 'estimate' ? 'nativeSampling' : 'nativeEncoding')
    }));
  };
  const errorKeys = {
    cancelled: 'nativeCancelled', missingFFmpeg: 'nativeMissing', invalid: 'nativeInvalid',
    busy: 'nativeBusy', frameCountMismatch: 'nativeMismatch', tooLarge: 'nativeLarge', failed: 'nativeFailed'
  };
  async function runNative(action) {
    if (!loaded || busy) return;
    let config;
    try { config = readConfiguration(); }
    catch (error) { status(error, true); return; }
    const controller = new AbortController();
    runController = controller;
    activeSettings = config;
    controller.signal.addEventListener('abort', () => {
      bridge.postMessage({ action: 'cancel' }).catch(() => {});
    }, { once: true });
    setBusy(true, action);
    refreshControls();
    requestedPreview = null;
    $('progress').max = 100;
    window.nativeConversionProgress(config.quality, 0);
    try {
      const result = await bridge.postMessage({ action, ...config });
      if (result.error) {
        status(() => t(errorKeys[result.error] || 'nativeFailed', { detail: result.detail || '' }),
          result.error !== 'cancelled');
      } else if (action === 'estimate') {
        estimateResult = { ...result, requestedSettings: config };
        showEstimate(estimateResult);
        status('nativeEstimateDone');
      } else {
        nativeResult = { ...result, requestedSettings: config };
        renderNativeResult();
        status('nativeFinished');
      }
    } catch (error) {
      status(() => t('nativeFailed', { detail: error.message }), true);
    } finally {
      runController = null;
      activeSettings = null;
      setBusy(false);
      refreshControls();
    }
  }
  $('estimate').removeEventListener('click', estimateSize);
  $('estimate').addEventListener('click', () => runNative('estimate'));
  $('convert').removeEventListener('click', convert);
  $('convert').addEventListener('click', () => runNative('convert'));
  applyLanguage('en');
}
