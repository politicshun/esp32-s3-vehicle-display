# UI Layout Spec — 디지털 클러스터 (LVGL 8.3.11, 800×480)

> 근거: 사용자 제공 레퍼런스 디자인 이미지, "핵심 표시 항목" 표
> 화면 구성: `lv_tileview` 기반 4페이지 가로 스와이프 (인디케이터 dot 4개, 하단 중앙)
> 구현: `main/ui/ui.c`, `main/ui/ui_style.c`

## ⚠️ 데이터 소스 제약 (2026-07-31 갱신 — 자체 CAN 스펙 확정 이후)

2026-07-31: 차량단(인버터) 설계가 아직 안 끝나서, 이 프로젝트가 CAN 스펙을 먼저 정하고
인버터 쪽이 거기 맞추기로 함(사용자 확인). `docs/hardware/vehicle.dbc`(Desktop `cluster.dbc`
사본)가 그 스펙이고, `main/include/vehicle_data.h`의 `VehicleData_t`에 아래 11개 필드가
존재한다:

```c
typedef struct {
    int16_t  speed;        // InvMsg1(0x100) byte0, 음수=후진
    uint8_t  soc;           // InvMsg1 byte6
    float    pack_volt;     // InvMsg1 byte5 (DClinkVoltage)
    uint8_t  dtc_code;      // InvMsg1 byte2
    bool     ble_connected; // CAN 무관 (main/ble.c)
    uint8_t  drive_mode;    // InvMsg1 byte1, VAL_: 0=P 1=R 2=N 3=D
    uint32_t odo_km;        // InvMsg2(0x200) byte2~4 (24bit)
    uint16_t range_km;      // InvMsg2 byte0
    float    power_kw;      // InvMsg2 byte5
    float    regen_kw;      // InvMsg2 byte1
    int16_t  sys_temp_c;    // InvMsg1 byte7
} VehicleData_t;
```

drive_mode/odo_km/range_km/power_kw/regen_kw/sys_temp_c/speed/soc/pack_volt/dtc_code
9개는 `docs/hardware/vehicle.dbc`에 정리된 **자체 확정 스펙**으로 화면까지 바인딩은
되어 있지만, 인버터 쪽 실물 구현·실기 검증 전이라 "스펙을 정했다" ≠ "실물로 확인됐다"다.
인버터 실물이 이 스펙대로 동작하는 게 확인되면 `docs/design/can-signals.md`의 상태 열만
갱신하면 되고, UI 바인딩 코드 자체는 이미 완료된 상태다.

TRIP/시계/외기온도 3개는 여전히 `VehicleData_t`에 소스가 없다. 레이아웃은 잡아두되
`ui.c` 안에서 `HARNESS-TODO` 주석이 달린 정적 placeholder(`--`)로만 표시했다.

## 공통 스타일 (`ui_style.c`)

| 토큰 | 값 |
|---|---|
| BG | `#0A0E14` → `#141C2E`(수직 그라데이션, `ui_style_bg`) |
| ACCENT_CYAN | `#2FD8E8` (스피드/SOC 아크) |
| ACCENT_CYAN_DEEP | `#1560A0` (스피드 그라데이션 아크의 저값 쪽 색) |
| ACCENT_GREEN | `#3CE87A` (정상/연결됨) |
| ACCENT_YELLOW | `#E8C93C` (예약 — 현재 미사용) |
| ACCENT_RED | `#E84C3C` (DTC/저잔량/미연결) |
| TEXT_PRIMARY / SECONDARY | `#FFFFFF` / `#8A97A8` |
| FONT | `lv_font_montserrat_48`(큰 숫자) / `_24`(중) / `_14`(라벨) |

### 게이지 고급화 (2026-07-30, 코드 전용 — 이미지 에셋 없이)

레퍼런스 사진("UI 가이드.png")의 배경 웨이브/쐐기 그래픽은 실제 이미지 에셋이라 재현 불가하지만,
아래는 순수 LVGL 스타일만으로 구현:
- 배경: `bg_grad_color`/`bg_grad_dir(VER)`로 은은한 수직 그라데이션
- 그라데이션 값 아크(속도 게이지만): LVGL 8.x의 arc는 단색만 지원해서, `ACCENT_CYAN_DEEP`→
  `ACCENT_CYAN`으로 색이 보간된 8개 얇은 아크 세그먼트(`UI_SPEED_GRAD_SEGMENTS`)를 이어붙여
  그라데이션처럼 보이게 함(`lv_color_mix`). SOC는 저잔량 RED 전환 로직이 있어 세그먼트화하지 않고
  기존 단색 동적 아크 유지.

### ⚠️ 실기기 사고: shadow 글로우 → task watchdog + 발열 (2026-07-30)

`lv_meter`의 `LV_PART_MAIN`에 `shadow_width=26`(시안 글로우)을 넣었다가 실기기에서
**Core1 task watchdog 타임아웃 + 화면 멈춤 + 보드 발열**이 발생함. LVGL 소프트웨어 그림자
렌더링이 매우 무거운데, 애니메이션되는 원형 게이지 2개가 매 프레임 그 그림자를 다시 계산하면서
CPU1이 `draw_shadow`에 계속 묶여 idle을 못 돌았음 (백트레이스로 확인). **애니메이션되는
위젯에는 shadow_width 스타일을 쓰지 말 것.**

### 정적 글로우 이미지로 대체 (2026-07-30)

같은 "빛나는 다이얼" 느낌을 매 프레임 재계산 없이 내기 위해, `scripts/gen_glow_image.py`로
64×64 저해상도 방사형 글로우(배경색→시안, `PIL`로 생성)를 LVGL true-color(RGB565) C 배열로
구워서(`main/ui/ui_glow_speed.c`, `ui_glow_soc.c`) `lv_img_create()` + `lv_img_set_zoom()`으로
게이지 크기(280/240px)까지 확대해 깐다. 글로우는 고주파 디테일이 없어 저해상도 확대에도 티가
안 남.

⚠️ **처음에 원본 해상도(280×280/240×240) 그대로 무압축 배열로 넣었더니 앱 파티션(1MB)이
4%만 남을 정도로 커졌음** — RGB565 원본 그대로면 페이지당 최대 156KB. 64×64로 줄이고 zoom으로
키우는 방식으로 바꿔서 17% 여유로 회복. 앞으로 이미지 에셋을 추가할 때는 항상
`idf.py build` 로그의 "app partition ... free" 경고를 확인할 것.

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

## 계기판형 게이지로 전면 개편 (2026-07-30)

빈 공간이 많고 아크가 단순해서 "차량 미터기 느낌이 안 난다"는 피드백에 따라, `lv_arc` 대신
`lv_meter`(`CONFIG_LV_USE_METER=y`, 이미 활성화돼 있었음)로 교체:
- 10~20단위 눈금 + 숫자 스케일(`lv_meter_set_scale_ticks`/`_major_ticks`가 자동으로 숫자 라벨도 그림)
- 정적 경고구역 아크(speed 160~200km/h, soc 0~20% — 둘 다 실차 스펙이 아니라 UI 표시 스케일/
  기존 `UI_SOC_LOW_PCT` 기준 그대로임)
- 동적 값 아크(속도/SOC 표시, SOC는 저잔량 시 RED로 색 전환 — `lv_meter_indicator_t`의
  공개(비-opaque) 구조체 필드를 직접 수정 + `lv_obj_invalidate`로 반영, 별도 공개 setter가 없어서)
- 아이콘(`LV_SYMBOL_OK`/`WARNING`/`BLUETOOTH` 등)은 검토했으나 실측 결과 `lv_font_montserrat_24`에
  해당 글리프가 없어서(한글과 같은 문제) 사용하지 않음 — 텍스트만 사용.

빈 공간 채우기용 `make_info_card()` 헬퍼(제목+큰 값, 테두리 있는 박스, `main/ui/ui.c`) 추가 —
기존 구석의 작은 텍스트 placeholder를 전부 이 카드로 교체.

## Page 1 — Drive (`build_page_drive`)

- Speed(live, `%d km/h`): 280px `lv_meter` 게이지(0~200km/h 표시 스케일, 20단위 눈금 숫자,
  160~200 레드존) 중앙에 48px 숫자. `speed`는 후진 시 음수(InvMsg1 byte0, offset -10)가
  될 수 있는데, 라벨 숫자는 정확히 음수로 찍지만(2026-07-31 %u→%d 수정) 게이지 아크는 음수
  구간을 시각적으로 표현하지 않고 0으로 clamp됨 — **HARNESS-TODO: 후진 표시 UI 미구현**
- MODE(Drive Mode, live): `drive_mode` 값을 P/R/N/D로 배지에 표시 (InvMsg1 byte1,
  `docs/hardware/vehicle.dbc`의 VAL_ 테이블과 1:1 대응, 자체 확정 스펙)
- ODO(live): `odo_km` 값 표시 (InvMsg2 byte2~4, 자체 확정 스펙) — `make_info_card` 카드(좌하단)
- TRIP: **HARNESS-TODO** — 필드 자체가 없음, `make_info_card` 카드(우하단)

## Page 2 — Battery (`build_page_battery`)

- SOC(live): 240px `lv_meter` 게이지(0~100%, 20단위 눈금 숫자, 0~20% 레드존), 20% 이하 시
  값 아크 RED 전환
- Pack Voltage(live, `%.1f V`) — 카드로 표시, 게이지 바로 아래 중앙 (InvMsg1 byte5, 정수 V 해상도)
- Range(live): `range_km` 값 표시 (InvMsg2 byte0, [0\|255]km) — 카드(좌하단)
- Power/Regen(live): `power_kw`/`regen_kw` 두 값을 카드 한 개에 2줄로 표시
  (InvMsg2 byte5/byte1, 정수 kW 해상도) — 카드(우하단)

## Page 3 — Diagnostics (`build_page_diag`)

- DTC(live): 0="DTC: OK"(회색 배너), 0이 아니면 RED 배너 + raw hex 값(`0x%02X`, 1바이트) 그대로
  표시 (InvMsg1 byte2 — 단일 열거값으로 가정, 여러 고장 동시 표현이 필요해지면 비트마스크로
  재설계 필요)
- Pack Voltage(live) / Sys Temp(live, `sys_temp_c`, InvMsg1 byte7): 220×140 큰 카드 2개,
  배너 아래 가운데 정렬. Sys Temp는 `UI_TEMP_WARN_C`(60°C, 실차 BMS 경고 기준과 무관하게
  UI 자체 임계값) 이상이면 카드 테두리와 텍스트가 RED로 전환

## Page 4 — Connectivity (`build_page_connect`)

- BLE(live): 22px dot + "BLE Connected"/"BLE Disconnected" 텍스트 (dot/간격 확대)
- Vehicle Status: `dtc_code == 0` 여부로 파생 (live), "Vehicle Status: OK" 텍스트
- DEVICE: BLE 기기명을 그대로 표시하는 카드 — `main/ble.c`의
  `ble_svc_gap_device_name_set("ESP32S3-Cluster")`와 동일한 실제 고정값(지어낸 데이터 아님),
  빈 공간을 의미 있게 채우기 위해 추가
- Time/Out Temp: **HARNESS-TODO** (RTC/온도센서 소스 미확정) — 카드(좌하단/우하단)

## "핵심 표시 항목" 표 커버리지 (2026-07-31 갱신 — 자체 CAN 스펙 확정 이후)

> **2026-07-31**: 차량단(인버터) 설계가 아직 안 끝나서 이 프로젝트가 CAN 스펙을 먼저 정하고
> 인버터 쪽이 맞추기로 함. 신호당 메시지 1개(10개 ID)였던 이전 구조를 InvMsg1(0x100)/
> InvMsg2(0x200) 2개 메시지로 패킹 재설계함(`docs/hardware/vehicle.dbc`,
> `docs/design/can-signals.md`). 아래 10개 신호는 "자체 확정 스펙"이지 "실물 검증 완료"는
> 아니다 — 인버터 실물이 없어서 검증 자체가 아직 불가능한 상태.

사용자 제공 가이드 이미지의 항목 전부가 (자체 확정-live / HARNESS-TODO placeholder 중
하나로) 화면에 노출되도록 반영 완료:
- **자체 확정-live 10개**: Speed/DriveMode/DTC/DClinkVoltage(Pack Voltage)/SOC/Temp(Sys Temp)는
  InvMsg1(0x100)에, DriveRange(Range)/RegenPower(Regen)/Odometer(ODO)/Power는 InvMsg2(0x200)에
  패킹 — `docs/hardware/vehicle.dbc` 기준, 화면 바인딩 완료. 인버터측 실물 구현/실기 검증 전
  이라는 점만 유의(스펙 확정 ≠ 실물 확인)
- **HARNESS-TODO 3개**: TRIP/Time/Out Temp — `VehicleData_t`에 필드 자체가 없어 여전히 정적
  `--` (Sys Temp와 혼동 주의: Sys Temp는 Page 3의 `sys_temp_c`, InvMsg1 byte7로 이미
  바인딩되어 있고, Out Temp는 Page 4의 별개 항목으로 CAN 신호 자체가 없음)

인버터 실물이 이 스펙대로 구현되고 실기로 검증되면, `docs/design/can-signals.md`의 상태
열만 "자체 확정 — 인버터측 구현/실기 검증 대기" → "실기 검증 완료"로 갱신하면 되고,
UI 바인딩/디코딩 코드는 이미 끝난 상태.

## 알려진 미구현 (다음 세션 후보)

- ~~드래그 스와이프 페이지 전환~~ — 2026-07-31, **탭 방식으로 교체**: 실기기에서 드래그가
  상용화하기엔 프레임이 너무 끊겨서(사용자 확인), 화면을 좌/우 절반으로 나눈 투명 클릭존
  (`build_tap_zones`, `main/ui/ui.c`)을 tileview 위에 얹어 탭으로만 페이지 전환. tileview
  자체의 SCROLLABLE 플래그는 그대로 둬서 `lv_obj_set_tile_id()`의 내부 애니메이션 스크롤과
  dot 갱신(`LV_EVENT_VALUE_CHANGED`)은 그대로 작동 — 사용자 입력만 클릭존이 가로채서
  tileview까지 안 내려가게 함. 첫/마지막 페이지에서 더 밀면 그냥 안 움직임(래핑 없음).
- ~~페이지 인디케이터 dot의 현재 페이지 하이라이트~~ — 구현 완료 (2026-07-30,
  `tileview_event_cb` + `LV_EVENT_VALUE_CHANGED`)
- ~~speed를 아크 게이지로 표시~~ — 구현 완료 (2026-07-30, `arc_speed`)
- speed 숫자를 레퍼런스만큼 더 키우려면 48px 이상 커스텀 비트맵 폰트 필요 (여전히 미구현 —
  LVGL 기본 montserrat 폰트가 48px까지만 지원)
- HARNESS-TODO 항목들은 `vehicle_data.h`에 필드가 추가되는 시점에 `ui.c`의 해당 블록만
  실데이터 바인딩으로 교체하면 됨 (레이아웃은 이미 확보돼 있음)
