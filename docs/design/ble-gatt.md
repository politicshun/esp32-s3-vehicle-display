# docs/design/ble-gatt.md

앱 팀(BLE 컴패니언 앱) 핸드오프 스펙. 진행 상황에 따라 계속 갱신함 — 하단 "핸드오프 진행 현황" 참고.

## GATT 프로파일

- 기기명(advertising name): `"ESP32S3-Cluster"`
- Service UUID / Characteristic UUID (2026-08-03 재발급 완료, `main/ble.c`):
  - Service: `d8be14dc-6df4-4aae-9750-65c274746c87`
  - Characteristic (vehicle data, Read+Notify): `95896bdd-d3b1-4f4b-b7b7-9dd2876e5c7c`
  - 앱 팀에는 위 문자열 그대로(하이픈 포함, 대소문자 무관) 전달. 이전 placeholder UUID(`0x2f,0x1a,0x9e,...`)는 폐기.
- Characteristic 권한: Read + Notify (Write 없음 — 앱→차량 방향 커맨드는 아직 요구사항 없음)
- Notify 주기: 500ms 고정 (`main/ble.c` `ble_sync_task`)
- 연결 상태는 `vehicle_data.ble_connected`에 반영
- 선호 MTU: 247 (`ble_att_set_preferred_mtu`) — 협상 실패 시 기본 23바이트(페이로드 20바이트)로 폴백되지만
  현재 payload(18바이트)는 기본 MTU로도 들어감. 향후 필드 추가 여지를 위해 큰 MTU를 유지.

## Payload 포맷 (2026-08-03 확정 — packed 바이너리)

`VehicleBlePacket_t` (`main/include/ble.h`), `__attribute__((packed))`, **little-endian**
(ESP32 네이티브 바이트오더 — 앱 파서에서 명시적으로 little-endian으로 디코드할 것).
총 18바이트.

| Offset | 필드 | 타입 | 단위/범위 | 비고 |
|---|---|---|---|---|
| 0 | proto_version | uint8 | `BLE_PROTOCOL_VERSION`(현재 1) | payload 필드 추가/변경 시 증가. 앱은 이 값으로 구버전 펌웨어 구분 |
| 1-2 | speed | int16 | km/h, [-10\|245] | 음수 = 후진 |
| 3 | soc | uint8 | %, [0\|100] | 배터리 잔량 |
| 4 | pack_volt | uint8 | V, [0\|80] | DC 링크 전압, 정수 해상도 |
| 5 | dtc_code | uint8 | [0\|255] | 고장 코드(단일 열거값, 비트마스크 아님). **0=정상, 1~255=고장(포괄) — 세부 코드별 의미 분류는 없음(2026-08-03 사용자 확인).** `docs/hardware/vehicle.dbc`에 VAL_ 테이블이 없고, `main/ui/ui.c`도 raw 값을 그대로 hex로만 보여줄 뿐 해석하지 않는 것과 일치. 앱도 "0=정상 / nonzero=고장(코드값은 raw hex로만 표시)" 이상으로 해석하지 말 것 — 세부 분류가 생기면 인버터팀 스펙 확정 후 이 표를 갱신 |
| 6 | drive_mode | uint8 | [0\|3] | 0=P 1=R 2=N 3=D |
| 7-10 | odo_km | uint32 | km | 누적 주행거리(이미 실제값, CAN 압축 해제된 값) |
| 11-12 | range_km | uint16 | km | 예상 주행가능거리 |
| 13 | power_kw | uint8 | kW, [0\|255] | 실시간 출력, 정수 해상도 |
| 14 | regen_kw | uint8 | kW, [0\|255] | 회생제동 출력, 정수 해상도 |
| 15-16 | sys_temp_c | int16 | degC | 시스템 온도 |

이전 리비전(2026-07-31, ASCII 텍스트 `"SPD:%dkm/h SOC:%u%% VOLT:%.1fV DTC:%u"`)은 nRF Connect 등
범용 스캐너 앱으로 테스트하던 단계 전용이었고 폐기됨. 그 텍스트에는 drive_mode/odo_km/range_km/
power_kw/regen_kw/sys_temp_c가 빠져있었는데, 이번 바이너리 전환에서 전부 포함시킴.

## 페어링/보안 (2026-08-03 확정 — Just Works bonding)

- 페어링 방식: **Just Works** — PIN 입력 없이 자동 암호화+본딩(`sm_io_cap=BLE_HS_IO_NO_INPUT_OUTPUT`).
  MITM(중간자 공격) 보호는 없음 — Just Works 자체의 한계이며, 별도 화면/키패드가 없는 이 기기
  구성상 Passkey/Numeric Comparison 방식은 애초에 쓸 수 없음.
- 보안 레벨: Unauthenticated pairing with encryption (`sm_sec_lvl=2`), LE Secure Connections
  우선 사용(`sm_sc=1`, 미지원 상대는 legacy pairing으로 폴백).
- vehicle 데이터 characteristic은 Read/Notify 모두 암호화된 링크에서만 허용(`_ENC` 플래그) —
  앱은 최초 연결 시 OS의 BLE 페어링 다이얼로그(iOS "페어링 요청" / Android 알림)가 뜨는 걸
  기본 흐름으로 구현하면 됨. 별도 PIN 입력 UI는 필요 없음.
- 페어링 키는 NVS에 저장되어(`CONFIG_BT_NIMBLE_NVS_PERSIST=y`) 기기 재부팅 후에도 재페어링
  불필요. 앱이 로컬 페어링 정보를 잃어버린 경우(재설치 등) 스택이 자동으로 재페어링을
  처리함(`CONFIG_BT_NIMBLE_HANDLE_REPEAT_PAIRING_DELETION=y`).
- 최대 3개 기기까지 본딩 정보 유지(`CONFIG_BT_NIMBLE_MAX_BONDS=3`, 기본값).

## 핸드오프 진행 현황 (2026-08-03 기준)

| 항목 | 상태 |
|---|---|
| Service/Characteristic UUID 재발급 | ✅ 완료 |
| Payload 포맷 확정(바이너리) | ✅ 완료 |
| dtc_code 코드값→의미 매핑 | ✅ 확정 — 0=정상/nonzero=고장(포괄), 세부 코드 분류 없음(사용자 확인) |
| 페어링/보안 정책 | ✅ 확정 — Just Works bonding |

4개 항목 모두 완료. 앱 팀은 이 문서 기준으로 컴패니언 앱 개발을 시작할 수 있음.
