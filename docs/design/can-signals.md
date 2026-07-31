# docs/design/can-signals.md

> 근거 파일: `docs/hardware/vehicle.dbc` (Desktop `cluster.dbc`의 사본).
> **2026-07-31 전면 재작성**: 차량단(인버터) 설계가 아직 끝나지 않아, 이 프로젝트가 CAN
> 스펙을 먼저 정하고 인버터 쪽이 여기 맞추기로 함(사용자 확인). 즉 아래 표는 더 이상
> "실차값 추측"이 아니라 **저희가 직접 정의한 스펙**이다. 다만 인버터 쪽 실물 구현/실기
> 검증 전이므로, "스펙을 정했다" ≠ "실물로 확인됐다"는 여전히 유효하다 — 상태 열 참고.
>
> 이전 리비전(신호당 메시지 1개, CAN ID 10개: 0x100/0x200/0x300~0x307)은 폐기했다.
> 지금은 2개 메시지에 10개 신호를 패킹한 구조다.

## InvMsg1 (CAN ID 0x100 / 256, DLC 8, 주기 100ms, INVERTER → CLUSTER)

| Byte | 신호 | 스케일(factor,offset) | 범위 | 상태 |
|---|---|---|---|---|
| 0 | speed | (1, -10) | [-10\|245] km/h, 음수=후진 | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 1 | drive_mode | (1, 0) | [0\|3], VAL_: 0=P 1=R 2=N 3=D | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 2 | dtc_code | (1, 0) | [0\|255], 단일 열거값(비트마스크 아님) | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 3~4 | (예약) | - | - | 미사용, 향후 확장용으로 비워둠 |
| 5 | pack_volt (DClinkVoltage) | (1, 0) | [0\|80] V, 정수 해상도 | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 6 | soc | (1, 0) | [0\|100] % | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 7 | sys_temp_c (Temp) | (1, -20) | [-20\|235] degC | **자체 확정 — 인버터측 구현/실기 검증 대기** |

## InvMsg2 (CAN ID 0x200 / 512, DLC 8, 주기 200ms, INVERTER → CLUSTER)

| Byte | 신호 | 스케일(factor,offset) | 범위 | 상태 |
|---|---|---|---|---|
| 0 | range_km (DriveRange) | (1, 0) | [0\|255] km | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 1 | regen_kw (RegenPower) | (1, 0) | [0\|255] kW, 정수 해상도 | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 2~4 | odo_km (Odometer) | (1, 0), 24bit LE | [0\|16777215] km | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 5 | power_kw (Power) | (1, 0) | [0\|255] kW, 정수 해상도 | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 6~7 | (예약) | - | - | 미사용, 향후 확장용으로 비워둠 |

바이트오더는 전부 LE(Intel, `@1`), 부호는 전부 unsigned(`+`) — 음수/확장 범위가 필요한
`speed`/`sys_temp_c`는 2의 보수가 아니라 **offset을 산술로 뺀 값**으로 표현한다
(예: `speed` raw=5 → 물리값 5-10=-5km/h).

`main/twai.c`가 이 두 메시지를 언패킹해서 `main/include/vehicle_data.h`의 `VehicleData_t`에
채운다. TRIP/시계/외기온도는 여전히 이 DBC에도, `VehicleData_t`에도 소스가 없어
`main/ui/ui.c`에서 정적 `--` placeholder(`HARNESS-TODO`)로 남아있다.

CAN ID/스케일/노드명이 바뀌면 이 표, `docs/hardware/vehicle.dbc`(및 원본 Desktop
`cluster.dbc`), `main/twai.c`, `main/include/vehicle_data.h`를 함께 갱신할 것.
