# docs/hardware/schematic-netlist.md
> 출처: ESP32-S3-Touch-LCD-7-Sch.pdf 넷리스트 직접 대조. PPT나 인수인계 문서보다 항상 우선한다.

## CAN (J6 커넥터 ↔ TJA1051)
- 넷 `NLCANL` = { J6 pin1, TJA1051(U7) pin6 }
- 넷 `NLCANH` = { J6 pin2, TJA1051(U7) pin7, R75 pin1 }
- **주의**: 과거 PPT 슬라이드 7의 "J6 pin1=CAN_H, pin2=CAN_L" 표기는 스키매틱과 반대로 되어 있었음(정정됨).
- MCU ↔ TJA1051(TXD/RXD): `pin_config.h`의 `CAN_TX_GPIO_NUM`(GPIO20) / `CAN_RX_GPIO_NUM`(GPIO19)
- **CAN 종단저항 (R75, 120Ω)**: SMD 고정저항이 아니라 **3핀 헤더 + 점퍼 캡 선택식**.
  보드 실크스크린 `CAN 120R NC` 라벨 (J5 스크류터미널 옆, RS-485용 동일 패턴 헤더와 나란히 위치).
  점퍼를 `120R` 쪽에 꽂으면 종단 활성화, `NC` 쪽이면 개방. 2026-08-18 실측: 점퍼가 헐겁게
  걸쳐있으면 완전 개방이 아니라 수십 kΩ대 고저항 접촉으로 측정될 수 있음(실제로 28.8kΩ →
  재장착 후 116Ω으로 정상화, 사용자 확인). 출처: `ESP32-S3-Touch-LCD-7-Sch.pdf` 넷리스트 대조.

## CAN/USB 스위치 (U9, FSUSB42UMX)
- SEL 핀 = 넷 `NLEXIO5` = `NLUSB_SEL` = CH422G EXIO5
- 스위치 출력 HSD1 = USBH_N/P (보드 위 "USB" 라벨 USB-C 포트, Type_C2) ↔ ESP32-S3 GPIO19/20 직결
- 스위치 출력 HSD2 = CANTX/CANRX ↔ TJA1051
- **즉 GPIO19/20은 CAN과 그 USB-C 포트가 물리적으로 배타 공유** — 동시 사용 불가

## I2C 버스 (CH422G + GT911 공유)
- GPIO8 = SDA, GPIO9 = SCL (`pin_config.h`의 `I2C_MASTER_SDA_IO`/`I2C_MASTER_SCL_IO`와 일치 확인됨)

## CH422G EXIO 매핑 (Appendix 2 EXIO pins 표 기준, `pin_config.h`와 교차 확인됨)
| EXIO | 기능 | pin_config.h 매크로 |
|---|---|---|
| EXIO1 | CTP_RST (터치 리셋) | `CH422G_EXIO_CTP_RST` (1<<1) |
| EXIO2 | DISP (백라이트 Enable) | `CH422G_EXIO_LCD_DISP` (1<<2) |
| EXIO3 | LCD_RST | `CH422G_EXIO_LCD_RST` (1<<3) |
| EXIO4 | SDCS (TF카드 CS) | `CH422G_EXIO_SD_CS` (1<<4) |
| EXIO5 | USB_SEL/CAN_SEL (Pull high = CAN 모드, Waveshare 공식 문서로 확인) | `CH422G_EXIO_USB_SEL` (1<<5) |
| EXIO6 | LCD_VDD_EN | `CH422G_EXIO_LCD_VDD_EN` (1<<6) |

## LCD RGB565 데이터 핀 (Waveshare 공식 문서 "LCD Interface" 표 기준)
데이터 순서(LSB→MSB): B3,B4,B5,B6,B7, G2,G3,G4,G5,G6,G7, R3,R4,R5,R6,R7
→ 상세 GPIO 번호는 `include/pin_config.h`의 `LCD_*_GPIO` 매크로 참조 (이 파일에 중복 기재하지 않음 — 어긋나면 여기가 아니라 거기가 진실).
