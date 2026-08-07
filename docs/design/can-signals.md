# docs/design/can-signals.md

> 근거 파일: `docs/hardware/cluster.dbc` (Desktop `cluster.dbc`의 사본).
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

## ClusterAlive (CAN ID 0x300 / 768, DLC 8, 주기 500ms, CLUSTER → INVERTER)

2026-08-07 추가. **이 저장소가 송신하는 유일한 메시지**로, 인버터 쪽이 클러스터의 생존과
상태를 판정할 수 있게 한다. ID/주기/페이로드는 사용자 결정으로 확정(추측 아님) —
인버터측 구현/실기 검증은 대기 중.

| Byte | 신호 | 스케일(factor,offset) | 범위 | 상태 |
|---|---|---|---|---|
| 0 | AliveCounter | (1, 0) | [0\|255], 전송 성공 시마다 +1 순환 | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 1 | ClusterStatus | (1, 0) | 비트마스크 (아래 표) | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 2~3 | ClusterUptime | (1, 0), 16bit LE | [0\|65535] s, 부팅 후 경과 초 | **자체 확정 — 인버터측 구현/실기 검증 대기** |
| 4~7 | (예약) | - | - | 미사용, 향후 확장용으로 비워둠 |

`ClusterStatus`는 **비트마스크**다 (`dtc_code`처럼 단일 열거값이 아님):

| 비트 | 의미 | 판정 근거 |
|---|---|---|
| 0 | InvMsg1 수신 정상 | 마지막 InvMsg1 수신이 300ms(=주기 100ms × 3) 이내 |
| 1 | InvMsg2 수신 정상 | 마지막 InvMsg2 수신이 600ms(=주기 200ms × 3) 이내 |
| 2 | BLE 앱 연결됨 | `VehicleData_t.ble_connected` |
| 3 | DTC 활성 | `VehicleData_t.dtc_code != 0` |
| 4~7 | (예약) | - |

`ClusterUptime`은 65535초(약 18.2시간)에서 **saturate**한다 — 롤오버시키면 수신측이
클러스터 재부팅과 구분할 수 없기 때문이다.

`AliveCounter`는 `twai_transmit()`이 `ESP_OK`를 반환했을 때 증가한다.

## 수신 필터 (2026-08-07)

`main/twai.c`는 SJA1000 계열 **하드웨어 어셉턴스 필터를 dual filter mode**로 걸어
표준 데이터 프레임 **0x100과 0x200만** 통과시킨다 (그 외 ID, 확장 프레임, RTR은
하드웨어에서 차단). 계산 근거는 `twai_start_with_retry()`의 주석 참고.

**수신 대상 ID가 3개 이상으로 늘어나면 dual filter로는 정확 매칭이 불가능하다** —
마스크를 넓혀 통과시키고 `handle_rx_message()`의 `switch` `default`에서 거르는 절충이
필요하며, 그때 이 문단과 코드 주석을 같이 갱신할 것.

송신하는 `ClusterAlive`(0x300)는 self-reception을 요청하지 않으므로 이 필터와 무관하다.

---

바이트오더는 전부 LE(Intel, `@1`), 부호는 전부 unsigned(`+`) — 음수/확장 범위가 필요한
`speed`/`sys_temp_c`는 2의 보수가 아니라 **offset을 산술로 뺀 값**으로 표현한다
(예: `speed` raw=5 → 물리값 5-10=-5km/h). `odo_km`는 `raw * 5`로 복원한다.

`main/twai.c`가 이 두 메시지를 언패킹해서 `main/include/vehicle_data.h`의 `VehicleData_t`에
채운다. TRIP/시계/외기온도는 여전히 이 DBC에도, `VehicleData_t`에도 소스가 없어
`main/ui/ui.c`에서 정적 `--` placeholder(`HARNESS-TODO`)로 남아있다 — InvMsg1의
byte5~7, InvMsg2의 byte6~7이 비어있으니 필요해지면 거기 채우면 된다.

CAN ID/스케일/노드명/우선도 배치가 바뀌면 이 표, `docs/hardware/cluster.dbc`
(및 원본 Desktop `cluster.dbc`), `main/twai.c`, `main/include/vehicle_data.h`,
그리고 로컬 작업본 `docs/hardware/CLUSTER_CAN_Protocol_*.xlsx`(깃에 안 올라감)를
함께 갱신할 것.
