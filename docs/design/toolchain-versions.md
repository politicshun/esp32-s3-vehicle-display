# docs/design/toolchain-versions.md
> 이 파일이 비어있거나 오래됐으면, 코드를 쓰기 전에 `idf.py --version`과
> `main/idf_component.yml` 결과를 사용자에게 요청해서 채운다. 절대 "일반적으로 알려진 API"로
> 추정해서 코드를 쓰지 않는다 — ESP-IDF는 마이너 버전 사이에도 구조체 필드가 삭제/변경된다
> (실제로 esp_lcd_rgb_panel_config_t의 bits_per_pixel/psram_trans_align 필드가 사라진 사례 있음).

| 항목 | 버전 | 확인 시점 / 방법 |
|---|---|---|
| ESP-IDF | v6.0.2 | 빌드 에러 로그 경로(`C:/esp/v6.0.2/esp-idf`)로 확인 |
| lvgl | ^8.3.11 | `main/idf_component.yml` |
| esp_lcd_touch_gt911 | ^1.0.0 | `main/idf_component.yml` |

## 변경 이력
- 2026-07-29: sdkconfig.defaults 최신화 완료 (idf.py save-defconfig 기준, 날짜: 2026-07-29)
- 2026-07-30: 한글 미표시 문제 — `lv_font_montserrat_24/14`가 한글 글리프를 지원하지 않아
  실기기에서 한글 라벨이 렌더링되지 않던 문제 확인. 맑은고딕 기반 서브셋 폰트(`ui_font_kr_24/14`,
  `lv_font_conv`로 생성, montserrat를 fallback으로 연결)로 시도했으나 실기기에서 여전히
  텍스트가 안 보여 원인 미해결 상태로 보류. 사용자 판단으로 UI 텍스트를 전부 영문으로 전환해
  우회함(실차 클러스터도 영문 표기가 흔함). 자세한 내용은 `docs/design/ui-layout.md` 참고.
- 2026-07-30: PSRAM(Octal, 8MB, 80MHz) 활성화. 실기기(COM12) 부팅 로그에서
  `Embedded PSRAM 8MB (AP_3v3)` 실측 확인 후 `CONFIG_SPIRAM`/`CONFIG_SPIRAM_MODE_OCT`/
  `CONFIG_SPIRAM_SPEED_80M`을 sdkconfig 및 sdkconfig.defaults에 반영. 이전엔 PSRAM 미설정 상태라
  `esp_lcd_new_rgb_panel()`의 `fb_in_psram=true`가 "no mem for frame buffer"로 실패했음.
- 2026-07-30: 앱 파티션 크기 1M → 4M로 확장 (`partitions.csv` 신규 추가,
  `CONFIG_PARTITION_TABLE_CUSTOM`/`CONFIG_PARTITION_TABLE_CUSTOM_FILENAME`으로 전환).
  기본 제공 `partitions_singleapp.csv`는 8MB 플래시 중 factory 앱에 1M만 할당해서
  UI 글로우 이미지 에셋을 조금 추가한 것만으로 여유가 17%까지 떨어짐(`docs/design/ui-layout.md`
  참고). 8MB 중 nvs(24K)+phy_init(4K)+factory(4M)를 합쳐도 4.03M만 쓰므로 나머지 ~4MB는
  향후 OTA 파티션 등에 여유로 남겨둠. 빌드 게이트로 확인한 free space: 17% → 79%
  (`0x32b390 bytes free`). 실기기(COM12) 재플래싱 후 부팅 로그로 파티션 테이블
  (`factory ... 00400000`), BLE 어드버타이징, TWAI 드라이버, LVGL/GT911 터치 초기화까지
  전부 정상 확인.
- 2026-07-31: sdkconfig.defaults 최신화 완료 (idf.py save-defconfig 기준, 날짜: 2026-07-31).
  재생성 결과와 저장소 버전을 대조한 결과 새로 추가된 CONFIG_ 항목은 없었음 — 재생성본에서 빠진
  `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME`/`CONFIG_PARTITION_TABLE_FILENAME`/
  `CONFIG_BT_BLUEDROID_ENABLED=n`은 값이 ESP-IDF Kconfig 기본값과 동일해서 save-defconfig가
  생략한 것뿐, 충돌 아님(kconfgen 특성). 저장소 버전(주석 포함) 그대로 유지.

- 2026-08-04: RGB LCD 더블버퍼링 도입 — `esp_lcd_rgb_panel_config_t.num_fbs=2` +
  `esp_lcd_rgb_panel_get_frame_buffer()`로 얻은 패널 자체 프레임버퍼 2장을 LVGL 드로우
  버퍼로 직접 사용(zero-copy, `main/lvgl.c`). 화면 전환 끊김 개선 확인, 실기기 커밋됨
  (`8216d30`).
- 2026-08-04: 터치 반응성 개선을 위해 `lv_disp_drv_t.direct_mode`(부분 재드로우)를
  시도했으나, 3페이지(Sys Temp 카드/DTC 배너) · 4페이지(BLE·Vehicle Status 점)에서
  원인 불명의 노이즈성 화면 지직거림이 실기기에서 재현됨. 아래 4가지 가설을 각각 실기기에서
  검증했으나 전부 재현 조건을 없애지 못함 — **원인 미확정 상태로 남음, `확인필요`**:
  1. "드물게 갱신되는 반투명/둥근모서리 위젯이 버퍼 간 동기화가 안 맞는다" — 매 프레임
     강제 `lv_obj_invalidate()`로도 안 없어짐
  2. "반투명 블렌딩이 두 버퍼에서 다르게 누적된다" — 해당 위젯들을 전부 불투명으로
     바꿔도 안 없어짐
  3. "LVGL 리프레시 주기(20ms)가 RGB 패널 물리 프레임 주기(pclk_hz=16MHz 기준 ~25.6ms)보다
     짧아서 CPU 쓰기와 DMA 읽기가 겹친다" — `CONFIG_LV_DISP_DEF_REFR_PERIOD`를 30ms로
     올려도 안 없어짐
  4. "CAN 데이터(sys_temp_c/dtc_code/ble_connected)가 흔들려서 그래 보인다" — 실기기
     로그로 확인한 결과 데이터는 완전히 정적(CAN 시뮬레이터 미구동 상태)이었음, 데이터
     문제 아님
  결국 `direct_mode` 시도 이전 상태인 `s_disp_drv.full_refresh=1`로 되돌림(더블버퍼링
  자체는 유지) — 지직거림 사라짐, 화면 전환 끊김도 없음(사용자 실기기 확인).
  **다음에 `direct_mode`를 다시 시도한다면**: 위 4개 가설은 이미 기각됐으니 반복하지 말고,
  esp_lcd RGB panel 드라이버의 zero-copy 경로(`rgb_panel_draw_bitmap()`)와 LVGL
  `direct_mode`의 `refr_sync_areas()` 상호작용 자체를 실측(로직 애널라이저 등)으로
  검증하는 쪽을 우선 고려할 것.
  **2026-08-04(2차 시도) 업데이트**: 실제로 하드웨어 vsync 동기화(가설 6)까지 시도했으나
  실패함 — `experiment/direct-mode-vsync-sync` 브랜치(커밋 `e667979`)에 시도 코드와
  가설 5·6 상세 기록 보존. **제품화 단계 전 로직 분석기 실측 필요, `main`은 손대지 않고
  더블버퍼링+`full_refresh`로 유지 중.**

## API 사용 전 확인 규칙
- `esp_lcd_*` 관련 구조체를 쓸 때는 위 ESP-IDF 버전에 해당하는 헤더를
  (로컬 `managed_components/` 또는 IDF 설치 경로, 또는 해당 버전 태그의 공개 저장소에서) 직접 확인 후 사용한다.
- 새 컴포넌트를 추가했다면 위 표에 버전과 확인 날짜를 갱신한다.
