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

## API 사용 전 확인 규칙
- `esp_lcd_*` 관련 구조체를 쓸 때는 위 ESP-IDF 버전에 해당하는 헤더를
  (로컬 `managed_components/` 또는 IDF 설치 경로, 또는 해당 버전 태그의 공개 저장소에서) 직접 확인 후 사용한다.
- 새 컴포넌트를 추가했다면 위 표에 버전과 확인 날짜를 갱신한다.
