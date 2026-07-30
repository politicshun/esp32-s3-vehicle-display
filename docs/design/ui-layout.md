# UI Layout Spec — 디지털 클러스터 (LVGL 8.3.11, 800×480)

> 근거: 사용자 제공 레퍼런스 디자인 이미지, "핵심 표시 항목" 표
> 화면 구성: `lv_tileview` 기반 4페이지 가로 스와이프 (인디케이터 dot 4개, 하단 중앙)
> 구현: `main/ui/ui.c`, `main/ui/ui_style.c`

## ⚠️ 데이터 소스 제약 (2번째 리비전에서 확정)

`main/include/vehicle_data.h`의 `VehicleData_t`에는 아래 5개 필드만 존재한다:

```c
typedef struct {
    uint16_t speed;
    uint8_t  soc;
    float    pack_volt;
    uint16_t dtc_code;
    bool     ble_connected;
} VehicleData_t;
```

레퍼런스 디자인의 Drive Mode/ODO/TRIP/Range/실시간 출력(kW)/팩 온도/시계/외기온도는
**현재 구조체에 소스가 없다.** CLAUDE.md 0번 원칙에 따라 지어내지 않고, 레이아웃은
잡아두되 `ui.c` 안에서 `HARNESS-TODO` 주석이 달린 정적 placeholder(`--`)로만 표시했다.
사용자가 "확장 기능은 보드 실구동 후 결정"이라고 확인함 — 이 문서/코드의 HARNESS-TODO
항목은 그 결정을 기다리는 상태다.

## 공통 스타일 (`ui_style.c`)

| 토큰 | 값 |
|---|---|
| BG | `#0A0E14` |
| ACCENT_CYAN | `#2FD8E8` (스피드/SOC 아크) |
| ACCENT_GREEN | `#3CE87A` (정상/연결됨) |
| ACCENT_YELLOW | `#E8C93C` (예약 — 현재 미사용) |
| ACCENT_RED | `#E84C3C` (DTC/저잔량/미연결) |
| TEXT_PRIMARY / SECONDARY | `#FFFFFF` / `#8A97A8` |
| FONT | `lv_font_montserrat_48`(큰 숫자) / `_24`(중) / `_14`(라벨) |

### ⚠️ UI 표시 언어: 한글 → 영문 전환 (2026-07-30)

처음엔 `main/ui/ui_font_kr_14.c`/`ui_font_kr_24.c`(맑은고딕 서브셋, `--lv-fallback`으로
몽세라 연결)를 만들어 한글을 시도했으나, **실기기에서 여전히 아무것도 안 보임** —
디버깅 중 "km/h" 같은 순수 라틴 텍스트까지 같이 사라지는 것으로 봐서 fallback 체인 자체가
의도대로 동작하지 않는 것으로 추정되나, 원인을 완전히 확정하지는 못했다(다음 세션에서
재조사 필요하면 `ui_font_kr_*.c`/`scripts/extract_korean_chars.py` 파일은 남겨뒀음 — 단,
현재 `main/CMakeLists.txt`의 `SRCS`에서는 빠져 있어 빌드에 안 들어감).

사용자 판단: "실제 차량 클러스터도 영어가 많다" — 폰트 문제 원인 규명보다 **UI 텍스트를
전부 영문으로 전환**하는 쪽으로 결정. 현재 `ui_style.c`는 `lv_font_montserrat_24/14`를
그대로 쓰고, `ui.c`의 모든 라벨/문자열이 영문이다.

## Page 1 — Drive (`build_page_drive`)

- Speed(live, `%u km/h`): 300px 원형 아크 게이지(0~200km/h, 표시 스케일값) 중앙에 48px 숫자
  (2026-07-30: 레퍼런스 디자인과 맞춰 페이지 2 SOC 아크와 동일한 스타일로 추가)
- MODE(Drive Mode): **HARNESS-TODO** — 값은 없지만 레퍼런스처럼 원형 배지 모양 + "MODE" 캡션
- ODO/TRIP: **HARNESS-TODO** — placeholder 라벨만

## Page 2 — Battery (`build_page_battery`)

- SOC(live): 270° 아크 게이지, 20% 이하 시 RED 전환
- Pack Voltage(live, `%.1f V`)
- Range: **HARNESS-TODO**
- Power/Regen: **HARNESS-TODO** ("Power/Regen" 라벨)

## Page 3 — Diagnostics (`build_page_diag`)

- DTC(live): 0="DTC: OK"(회색 배너), 0이 아니면 RED 배너 + raw hex 값 그대로 표시
  (0x300/0x301 CAN ID가 `docs/design/can-signals.md`상 실차 DBC 미대조 placeholder이므로,
  UI는 값을 해석하지 않고 그대로 노출)
- Pack Voltage(live, page 2와 별도 위젯으로 재표시)
- Sys Temp: **HARNESS-TODO** ("Sys Temp" 라벨)

## Page 4 — Connectivity (`build_page_connect`)

- BLE(live): dot 색상 + "BLE Connected"/"BLE Disconnected" 텍스트
- Vehicle Status: `dtc_code == 0` 여부로 파생 (live), "Vehicle Status: OK" 텍스트
- Time/Out Temp: **HARNESS-TODO** (RTC/온도센서 소스 미확정, "Time"/"Out Temp" 라벨)

## "핵심 표시 항목" 표 커버리지 (2026-07-30)

사용자 제공 가이드 이미지의 12개 항목 전부가 (live 데이터 또는 HARNESS-TODO placeholder로)
화면에 노출되도록 반영 완료. live 5개 vs placeholder 7개 비율은 `VehicleData_t` 필드 수
제약 그대로이며, 스타일/레이아웃 다듬기는 후속 세션 과제로 남김(사용자 확인).

## 알려진 미구현 (다음 세션 후보)

- ~~페이지 인디케이터 dot의 현재 페이지 하이라이트~~ — 구현 완료 (2026-07-30,
  `tileview_event_cb` + `LV_EVENT_VALUE_CHANGED`)
- ~~speed를 아크 게이지로 표시~~ — 구현 완료 (2026-07-30, `arc_speed`)
- speed 숫자를 레퍼런스만큼 더 키우려면 48px 이상 커스텀 비트맵 폰트 필요 (여전히 미구현 —
  LVGL 기본 montserrat 폰트가 48px까지만 지원)
- HARNESS-TODO 항목들은 `vehicle_data.h`에 필드가 추가되는 시점에 `ui.c`의 해당 블록만
  실데이터 바인딩으로 교체하면 됨 (레이아웃은 이미 확보돼 있음)
