# ARCHITECTURE.md — ESP32-S3 차량 클러스터

## 레이어 구조

```
main/main.c          app_main() — 초기화 순서 + 태스크 생성만. 로직 없음.
main/init.c          HW 초기화 (I2C 버스, CH422G). bsp_hardware_init() 단일 진입점.
main/twai.c          CAN 수신 → vehicle_data 갱신. 그 외 책임 없음.
main/ble.c           BLE GATT 서버 → vehicle_data 읽어서 notify. 그 외 책임 없음.
main/lvgl.c          LCD/터치 → vehicle_data 읽어서 화면 갱신. 그 외 책임 없음.
main/vehicle_data.c  뮤텍스로 보호된 공유 상태. 유일한 진실의 원천(런타임 값 기준).
include/pin_config.h 모든 GPIO/I2C 주소/CH422G 비트 정의. **이 파일 밖에서 GPIO 번호를 하드코딩하지 않는다.**
                     (실제 경로: main/include/pin_config.h — CMakeLists.txt의 INCLUDE_DIRS 설정에 맞춰 확인됨, 2026-07-28)
include/bsp.h        init.c가 만든 I2C 버스 핸들 등 크로스 모듈 공유 심볼.
```

## 의존성 규칙

- `twai.c`, `ble.c`, `lvgl.c`는 서로를 직접 참조하지 않는다 — **오직 `vehicle_data.c`를 통해서만 통신**한다.
- `pin_config.h`에 없는 GPIO 번호를 다른 `.c` 파일에 새로 적지 않는다. 필요하면 `pin_config.h`에
  먼저 추가하고 PR/커밋에 이유를 남긴다.
- `init.c`가 만드는 `i2c_master_bus_handle_t`는 `bsp.h`를 통해서만 다른 파일에 노출한다
  (I2C 포트 번호를 다른 파일에서 직접 캐스팅해서 쓰지 않는다 — 과거 이 방식이 ESP-IDF 버전업으로 깨진 적 있음).

## 새 기능 추가 시 규칙

1. 새 CAN ID를 다루려면 `docs/design/can-signals.md`에 ID·스케일·바이트오더를 먼저 기록
2. 새 BLE characteristic을 추가하려면 `docs/design/ble-gatt.md`에 UUID·권한(암호화 필요 여부)을 먼저 기록
3. 문서 없이 코드부터 쓰지 않는다 (AGENTS.md 0번 원칙과 동일)
