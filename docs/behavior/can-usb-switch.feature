Feature: CAN/USB 스위치 초기화
  보드의 GPIO19/20은 CAN과 온보드 USB-C 포트가 물리적으로 공유한다 (docs/hardware/schematic-netlist.md 참고).
  부팅 시 반드시 CAN 모드로 고정되어야 클러스터가 정상 동작한다.

  Scenario: 정상 부팅 시 CAN 모드로 전환
    Given CH422G I2C 초기화가 성공했다
    When bsp_hardware_init()이 실행된다
    Then CH422G_EXIO_USB_SEL 비트가 HIGH로 설정된다
    And 이후 TWAI 드라이버가 GPIO19(RX)/GPIO20(TX)에서 정상적으로 프레임을 수신한다

  Scenario: CH422G I2C 응답 없음
    Given CH422G가 I2C 버스에 응답하지 않는다
    When bsp_hardware_init()이 실행된다
    Then 에러 로그가 남고
    And 이후 CAN/LCD/터치 동작이 비정상일 수 있다는 경고가 남는다
    But 시스템은 재부팅 없이 계속 동작을 시도한다 (안전한 실패)
