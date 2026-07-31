# docs/design/can-signals.md

> 근거 파일: `docs/hardware/vehicle.dbc` (2026-07-30, Vector CANdb++ 편집용으로 작성).
> "확정"이라도 그 근거가 된 실차 매뉴얼/DBC 원본은 이 저장소/개발 PC에 없어 재검증 불가 상태다
> (2026-07-30 확인). placeholder 항목은 전부 스케일/바이트오더/ID 자체가 미확정 가정이다.

| CAN ID | 필드 | 스케일 | 바이트오더 | 상태 |
|---|---|---|---|---|
| 0x100 | speed | 1(정수, km/h) | - | 확정 (실차 매뉴얼 대조 완료 — 근거 문서 위치 불명) |
| 0x200 | soc | 1(정수, %) | - | 확정 (근거 문서 위치 불명) |
| 0x300 | pack_volt | 0.1V | LE 가정 | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조** |
| 0x301 | dtc_code | 1 | LE 가정 | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조** |
| 0x302 | drive_mode | 0=P,1=R,2=N,3=D | - | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조** |
| 0x303 | odo_km | 1km, 24bit | LE 가정 | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조** |
| 0x304 | range_km | 1km, 16bit | LE 가정 | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조** |
| 0x305 | power_kw | 0.1kW, unsigned | LE 가정 | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조 (회생제동과 별개 신호)** |
| 0x306 | regen_kw | 0.1kW, unsigned | LE 가정 | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조 (출력과 별개 신호)** |
| 0x307 | sys_temp_c | 1°C, signed 8bit | - | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조** |

`twai.c`가 이 ID들을 처리한다. 0x300 이후는 전부 placeholder이므로,
관련 코드를 수정/리뷰할 때마다 이 표의 상태 열을 먼저 확인한다. 실차 DBC나
KVASER를 실차 버스에 물린 역추적 로그로 값이 확정되면 이 표, `docs/hardware/vehicle.dbc`,
`main/twai.c`, `main/include/vehicle_data.h`를 함께 갱신할 것.
