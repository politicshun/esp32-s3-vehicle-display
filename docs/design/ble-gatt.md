# docs/design/ble-gatt.md

- 기기명: "ESP32S3-Cluster"
- Service UUID / Characteristic UUID: `ble.c`의 placeholder 128bit UUID — **HARNESS-TODO: 양산 전 재발급 필요**
- Characteristic 권한: Read + Notify. **암호화/페어링 미적용** (개발 단계 임시, `BLE_GAP_CONN_MODE_UND`로 전체 개방)
- Payload: `VehicleBlePacket_t` (packed, LE) — speed(u16), soc(u8), pack_volt(float), dtc_code(u16)
- 연결 상태는 `vehicle_data.ble_connected`에 반영, notify 주기 500ms
