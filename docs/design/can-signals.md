# docs/design/can-signals.md

> 근거 파일: `docs/hardware/vehicle.dbc` (2026-07-30, Vector CANdb++ 편집용으로 작성).
> **2026-07-31 정정: 이 표의 모든 CAN ID(0x100/0x200 포함)를 실차 미대조 임시 선언으로 재분류함.**
> 0x100/0x200을 "확정"으로 표기했던 이전 버전은 근거 문서가 저장소/개발 PC 어디에도 없어
> 재검증이 불가능했고, 사용자 확인 결과 초기 테스트용으로 임의 할당한 ID였다(2026-07-31).
> 즉 지금 시점에 실차 DBC와 대조된 CAN ID/스케일/바이트오더는 **하나도 없다.**

| CAN ID | 필드 | 스케일 | 바이트오더 | 상태 |
|---|---|---|---|---|
| 0x100 | speed | 1(정수, km/h) | - | **HARNESS-TODO: 확인필요 — 초기 테스트용 임의 할당, 실차 미대조 (2026-07-31 재분류)** |
| 0x200 | soc | 1(정수, %) | - | **HARNESS-TODO: 확인필요 — 초기 테스트용 임의 할당, 실차 미대조 (2026-07-31 재분류)** |
| 0x300 | pack_volt | 0.1V | LE 가정 | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조** |
| 0x301 | dtc_code | 1 | LE 가정 | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조** |
| 0x302 | drive_mode | 0=P,1=R,2=N,3=D | - | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조** |
| 0x303 | odo_km | 1km, 24bit | LE 가정 | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조** |
| 0x304 | range_km | 1km, 16bit | LE 가정 | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조** |
| 0x305 | power_kw | 0.1kW, unsigned | LE 가정 | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조 (회생제동과 별개 신호)** |
| 0x306 | regen_kw | 0.1kW, unsigned | LE 가정 | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조 (출력과 별개 신호)** |
| 0x307 | sys_temp_c | 1°C, signed 8bit | - | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조** |

`twai.c`가 이 ID들을 처리한다. **이 표의 CAN ID는 전부(0x100/0x200 포함) placeholder이므로**,
관련 코드를 수정/리뷰할 때마다 이 표의 상태 열을 먼저 확인한다. 실차 DBC나
KVASER를 실차 버스에 물린 역추적 로그로 값이 확정되면 이 표, `docs/hardware/vehicle.dbc`,
`main/twai.c`, `main/include/vehicle_data.h`를 함께 갱신할 것.
