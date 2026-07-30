# docs/design/can-signals.md

| CAN ID | 필드 | 스케일 | 바이트오더 | 상태 |
|---|---|---|---|---|
| 0x100 | speed | 1(정수, km/h) | - | 확정 (실차 매뉴얼 대조 완료) |
| 0x200 | soc | 1(정수, %) | - | 확정 |
| 0x300 | pack_volt | 0.1V | LE 가정 | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조** |
| 0x301 | dtc_code | 1 | LE 가정 | **HARNESS-TODO: 확인필요 — placeholder, 실차 DBC 미대조** |

`twai.c`가 이 ID들을 처리한다. 0x300/0x301은 실제 값이 아니므로,
관련 코드를 수정/리뷰할 때마다 이 표의 상태 열을 먼저 확인한다.
