# docs/design/ble-gatt.md

- 기기명: "ESP32S3-Cluster"
- Service UUID / Characteristic UUID: `ble.c`의 placeholder 128bit UUID — **HARNESS-TODO: 양산 전 재발급 필요**
- Characteristic 권한: Read + Notify. **암호화/페어링 미적용** (개발 단계 임시, `BLE_GAP_CONN_MODE_UND`로 전체 개방)
- Payload (2026-07-31 변경): **ASCII 텍스트** — `"SPD:%dkm/h SOC:%u%% VOLT:%.1fV DTC:%u"`
  (`format_vehicle_text()`, `main/ble.c`). nRF Connect 등 범용 BLE 스캐너 앱으로 테스트하기로
  해서(사용자 확인), packed 바이너리(`VehicleBlePacket_t`, 이전 리비전) 대신 사람이 바로 읽는
  텍스트로 바꿈 — 스캐너 앱에서 hex 대신 "UTF-8 String" 보기로 확인. speed는 `int16_t`라 `%d`로
  부호(후진 음수) 보존.
  - 아직 drive_mode/odo_km/range_km/power_kw/regen_kw/sys_temp_c는 텍스트에 안 실림 —
    CAN 스펙 확장(`docs/design/can-signals.md`) 이후 BLE 쪽 반영은 다음 단계(HARNESS-TODO)
  - 커스텀 스마트폰 앱을 만드는 단계가 되면 다시 packed 바이너리(또는 JSON)로 바꾸는 걸
    검토할 것 — 지금 텍스트 포맷은 사람이 눈으로 확인하는 테스트 단계 전용
- 연결 상태는 `vehicle_data.ble_connected`에 반영, notify 주기 500ms
