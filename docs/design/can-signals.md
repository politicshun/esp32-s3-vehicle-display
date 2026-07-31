# docs/design/can-signals.md

> 근거 파일: `docs/hardware/vehicle.dbc` (Desktop `cluster.dbc`의 사본).
> 2026-07-31: 차량단(인버터) 설계가 아직 끝나지 않아, 이 프로젝트가 CAN 스펙을 먼저 정하고
> 인버터 쪽이 여기 맞추기로 함(사용자 확인). 즉 아래 표는 "실차값 추측"이 아니라
> **저희가 직접 정의한 스펙**이다. 다만 인버터 쪽 실물 구현/실기 검증 전이므로,
> "스펙을 정했다" ≠ "실물로 확인됐다"는 여전히 유효하다 — 상태 열 참고.
>
> **2026-07-31 CAN 최적화 리비전**: 신호를 서브시스템별(주행/배터리) 묶음이 아니라
> **갱신 우선도** 기준으로 InvMsg1/InvMsg2에 재배치했다. Odometer는 24bit(factor1)에서
> 16bit(factor5)로 압축했다(실사용 상한 약 30만km 기준).

## InvMsg1 (CAN ID 0x100 / 256, DLC 8, 주기 100ms, INVERTER → CLUSTER)

**우선도 높음** — 운전 중 계속 바뀌거나(Speed/Power/RegenPower), 지연되면 안전에
영향을 줄 수 있는(DriveMode/DTC) 신호를 묶었다.

| Byte | 신호 | 스케일(factor,offset) | 범위 | 상태 |
|---|---|---|---|---|
| 0 | speed | (1, -10) | [-10\|245] km/h, 음수=후진 | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 1 | drive_mode | (1, 0) | [0\|3], VAL_: 0=P 1=R 2=N 3=D | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 2 | dtc_code | (1, 0) | [0\|255], 단일 열거값(비트마스크 아님) | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 3 | power_kw (Power) | (1, 0) | [0\|255] kW, 정수 해상도 | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 4 | regen_kw (RegenPower) | (1, 0) | [0\|255] kW, 정수 해상도 | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 5~7 | (예약) | - | - | 미사용, 향후 확장용으로 비워둠 |

## InvMsg2 (CAN ID 0x200 / 512, DLC 8, 주기 200ms, INVERTER → CLUSTER)

**우선도 낮음** — 서서히 바뀌는 상태값이라 200ms 주기로 충분하다고 판단한 신호를 묶었다.

| Byte | 신호 | 스케일(factor,offset) | 범위 | 상태 |
|---|---|---|---|---|
| 0 | soc | (1, 0) | [0\|100] % | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 1 | pack_volt (DClinkVoltage) | (1, 0) | [0\|80] V, 정수 해상도 | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 2 | sys_temp_c (Temp) | (1, -20) | [-20\|235] degC | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 3 | range_km (DriveRange) | (1, 0) | [0\|255] km | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 4~5 | odo_km (Odometer) | (5, 0), 16bit LE | [0\|327675] km | **자체 확정 — 인버터측 구현/실기 검증 대기** (2026-07-31: 24bit/factor1 → 16bit/factor5 압축, 실사용 상한 약 30만km 기준 5km 해상도) |
| 6~7 | (예약) | - | - | 미사용, 향후 확장용으로 비워둠 |

바이트오더는 전부 LE(Intel, `@1`), 부호는 전부 unsigned(`+`) — 음수/확장 범위가 필요한
`speed`/`sys_temp_c`는 2의 보수가 아니라 **offset을 산술로 뺀 값**으로 표현한다
(예: `speed` raw=5 → 물리값 5-10=-5km/h). `odo_km`는 `raw * 5`로 복원한다.

`main/twai.c`가 이 두 메시지를 언패킹해서 `main/include/vehicle_data.h`의 `VehicleData_t`에
채운다. TRIP/시계/외기온도는 여전히 이 DBC에도, `VehicleData_t`에도 소스가 없어
`main/ui/ui.c`에서 정적 `--` placeholder(`HARNESS-TODO`)로 남아있다 — InvMsg1의
byte5~7, InvMsg2의 byte6~7이 비어있으니 필요해지면 거기 채우면 된다.

CAN ID/스케일/노드명/우선도 배치가 바뀌면 이 표, `docs/hardware/vehicle.dbc`
(및 원본 Desktop `cluster.dbc`), `main/twai.c`, `main/include/vehicle_data.h`를
함께 갱신할 것.
