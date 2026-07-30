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
| FONT | `lv_font_montserrat_48`(큰 숫자) / `_24`(중) / `_14`(라벨) — sdkconfig에 3개 다 활성화 필요(적용 완료) |

## Page 1 — 주행 필수 정보 (`build_page_drive`)

- `speed`(live, `%u km/h`): 화면 중앙, 48px
- GEAR: **HARNESS-TODO** — placeholder 라벨만
- ODO/TRIP: **HARNESS-TODO** — placeholder 라벨만

## Page 2 — 전력 및 배터리 (`build_page_battery`)

- `soc`(live): 270° 아크 게이지, 20% 이하 시 RED 전환
- `pack_volt`(live, `%.1f V`)
- Range / 실시간 출력(kW): **HARNESS-TODO**

## Page 3 — 시스템 상태 및 진단 (`build_page_diag`)

- `dtc_code`(live): 0=정상(회색 배너), 0이 아니면 RED 배너 + raw hex 값 그대로 표시
  (0x300/0x301 CAN ID가 `docs/design/can-signals.md`상 실차 DBC 미대조 placeholder이므로,
  UI는 값을 해석하지 않고 그대로 노출)
- `pack_volt`(live, page 2와 별도 위젯으로 재표시)
- Pack Temp: **HARNESS-TODO**

## Page 4 — 기기 조작 및 연결성 (`build_page_connect`)

- `ble_connected`(live): dot 색상 + 텍스트
- 차량 상태 dot: `dtc_code == 0` 여부로 파생 (live)
- 시각 / 외기온도: **HARNESS-TODO** (RTC/온도센서 소스 미확정)

## 알려진 미구현 (다음 세션 후보)

- 페이지 인디케이터 dot의 현재 페이지 하이라이트 (tileview `LV_EVENT_VALUE_CHANGED`)
- speed 숫자를 레퍼런스만큼 더 키우려면 48px 이상 커스텀 비트맵 폰트 필요
- HARNESS-TODO 항목들은 `vehicle_data.h`에 필드가 추가되는 시점에 `ui.c`의 해당 블록만
  실데이터 바인딩으로 교체하면 됨 (레이아웃은 이미 확보돼 있음)
