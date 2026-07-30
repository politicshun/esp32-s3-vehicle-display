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

## API 사용 전 확인 규칙
- `esp_lcd_*` 관련 구조체를 쓸 때는 위 ESP-IDF 버전에 해당하는 헤더를
  (로컬 `managed_components/` 또는 IDF 설치 경로, 또는 해당 버전 태그의 공개 저장소에서) 직접 확인 후 사용한다.
- 새 컴포넌트를 추가했다면 위 표에 버전과 확인 날짜를 갱신한다.
