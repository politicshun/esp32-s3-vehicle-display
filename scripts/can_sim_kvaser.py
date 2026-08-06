# scripts/can_sim_kvaser.py
# KVASER 실물 어댑터로 InvMsg1(0x100, 100ms)/InvMsg2(0x200, 200ms)를 계속 쏴주는
# 벤치 테스트용 시뮬레이터.
# CANKing의 Generator는 "고정값 주기 반복"만 되고 값이 시간에 따라 변하는 건 못 해서,
# docs/hardware/cluster.dbc를 cantools로 읽어 실측 스펙(factor/offset/범위) 그대로
# 인코딩하고, python-can의 kvaser 백엔드(canlib32.dll)로 실제 버스에 낸다.
#
# 2026-08-05: 원래는 정차->가속->순항->감속->후진 사이클로 "그럴듯한 주행"을 흉내냈는데,
# 실주행처럼 보일 필요는 없고 게이지/숫자가 값 변화에 반응성 좋게+매끄럽게 따라오는지만
# 확인하면 된다는 피드백 — 구간별 상태 머신을 버리고, 모든 신호를 각자 다른 주기의
# sin파로 "항상 동시에" 움직이게 바꿈(한 신호가 멈춰있는 구간이 없음). 저마다 주기/위상을
# 다르게 잡아서 전부 같이 오르내리지 않고 서로 엇갈리게 움직이도록 함 — 그래야 여러
# 게이지가 동시에 다른 방향으로 바뀌는 "역동적인" 그림이 나옴. SOC/Temp는 진폭을
# UI_SOC_LOW_PCT(20%)/UI_TEMP_WARN_C(60도) 경고 임계값을 주기적으로 넘나들도록 잡아서
# 경고색 전환(main/ui/ui.c anim_soc_exec_cb/anim_temp_exec_cb) 로직도 같이 흔들어본다.
#
# 사용 전 확인:
#   python -m pip install cantools python-can
#   python can_sim_kvaser.py --list-channels   # 실물 어댑터가 몇 번 채널인지 확인
#   python can_sim_kvaser.py --channel 0       # 실행 (Ctrl+C로 종료)
import argparse
import math
import time
from pathlib import Path

import can
import cantools

DBC_PATH = Path(__file__).resolve().parent.parent / "docs" / "hardware" / "cluster.dbc"

MSG1_PERIOD_S = 0.100  # InvMsg1 (docs/hardware/cluster.dbc GenMsgCycleTime와 동일)
MSG2_PERIOD_S = 0.200  # InvMsg2
TICK_S = 0.02          # 갱신 주기는 20ms로 설정했음. (LVGL 갱신과 일치시킴)

DRIVE_MODE = 3  # 항상 D — 기어 배지는 애니메이션이 없어서(즉시 전환) 반응성 테스트와 무관, 흔들면 오히려 헷갈림


def _osc(t: float, center: float, amplitude: float, period_s: float, phase_rad: float) -> float:
    """center를 기준으로 +-amplitude만큼 sin파로 진동하는 값. 신호마다 period_s/phase_rad를
    다르게 줘서 서로 다른 타이밍에 오르내리게 만든다(전부 같이 움직이면 "역동적"이라기보단
    단조로워 보임)."""
    return center + amplitude * math.sin(2.0 * math.pi * t / period_s + phase_rad)


class DriveState:
    """실주행 흉내를 버리고, 모든 신호를 각자 다른 주기의 sin파로 항상 동시에 움직이게
    만드는 상태. 목적은 '그럴듯한 값'이 아니라 '게이지/숫자 애니메이션이 끊김 없이 잘
    따라오는지'를 최대한 많이 노출시키는 것 — 그래서 값이 멈춰있는 구간이 아예 없다."""

    def __init__(self, start_odo_km: float = 12345.0):
        self.t = 0.0
        self.odo_km = start_odo_km

    def step(self, dt: float):
        self.t += dt
        t = self.t

        # 각 신호: (중심값, 진폭, 주기(s), 위상) — 전부 다르게 잡아서 서로 엇갈리게 움직임.
        # 2026-08-05(2차): 주기가 1~4s로 너무 짧아서 애니메이션(150ms)이 따라잡기도 전에
        # 목표값이 또 바뀌어 "부드럽게 스윕"이 아니라 "불연속적으로 튄다"는 피드백 —
        # 진폭은 그대로 두고 주기만 3~4배 늘려서(값이 전체 범위를 쓸고 지나가는 속도를 늦춤)
        # 매 애니메이션 구간 동안 목표가 조금씩만 움직이게 함.
        speed = _osc(t, 95.0, 105.0, 8.0, 0.0)          # -10~200km/h (가끔 후진 구간도 스쳐감)
        power = _osc(t, 50.0, 50.0, 6.0, 0.8)           # 0~100kW (UI_POWER_GAUGE_MAX_KW과 동일)
        regen = _osc(t, 25.0, 25.0, 7.0, 1.6)           # 0~50kW (UI_REGEN_BAR_MAX_KW과 동일)
        soc = _osc(t, 55.0, 45.0, 11.0, 2.4)            # 10~100% (가끔 20% 밑으로 -> SOC 경고색)
        voltage = _osc(t, 40.0, 40.0, 5.5, 3.2)         # 0~80V (UI_BAR_GAUGE 전압 범위와 동일)
        temp = _osc(t, 45.0, 55.0, 9.0, 4.0)            # -10~100도 (가끔 60도 넘음 -> Temp 경고색)
        range_km = _osc(t, 130.0, 120.0, 12.0, 0.4)     # 10~250km, 애니메이션 없는 텍스트지만 같이 흔듦

        speed = max(-10.0, min(245.0, speed))
        power = max(0.0, min(255.0, power))
        regen = max(0.0, min(255.0, regen))
        soc = max(0.0, min(100.0, soc))
        voltage = max(0.0, min(80.0, voltage))
        temp = max(-20.0, min(235.0, temp))
        range_km = max(0.0, min(255.0, range_km))

        self.odo_km += abs(speed) / 3600.0 * dt

        return {
            "Speed": round(speed),
            "DriveMode": DRIVE_MODE,
            "DTC": 0,
            "Power": round(power),
            "RegenPower": round(regen),
            "SOC": round(soc),
            "DClinkVoltage": round(voltage),
            "Temp": round(temp),
            "DriveRange": round(range_km),
            "Odometer": round(min(327675.0, self.odo_km)),
        }


def list_channels():
    configs = can.detect_available_configs(interfaces=["kvaser"])
    if not configs:
        print("KVASER 채널을 하나도 못 찾았습니다 — 어댑터가 이 PC에 꽂혀있는지, "
              "드라이버가 정상인지 확인해주세요.")
        return
    for c in configs:
        print(f"channel={c['channel']}  device={c.get('device_name')}  serial={c.get('serial')}")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--channel", type=int, default=0, help="KVASER 채널 번호 (--list-channels로 확인)")
    ap.add_argument("--bitrate", type=int, default=500000, help="CAN 버스 속도 (기본 500kbps)")
    ap.add_argument("--list-channels", action="store_true", help="사용 가능한 KVASER 채널만 출력하고 종료")
    args = ap.parse_args()

    if args.list_channels:
        list_channels()
        return

    db = cantools.database.load_file(str(DBC_PATH)) # dbc 파일 불러오기 -> CAN 메시지, 데이터 맵핑
    msg1 = db.get_message_by_name("InvMsg1")
    msg2 = db.get_message_by_name("InvMsg2")

    bus = can.interface.Bus(interface="kvaser", channel=args.channel, bitrate=args.bitrate)
    print(f"KVASER channel {args.channel} 열림 ({args.bitrate}bps). "
          f"InvMsg1={hex(msg1.frame_id)}({MSG1_PERIOD_S*1000:.0f}ms) "
          f"InvMsg2={hex(msg2.frame_id)}({MSG2_PERIOD_S*1000:.0f}ms) 송신 시작. Ctrl+C로 종료.")

    state = DriveState()
    next_msg1 = next_msg2 = time.monotonic()
    last_tick = time.monotonic()
    last_print = 0.0

    try:
        while True:
            now = time.monotonic()
            dt = now - last_tick
            if dt < TICK_S:
                time.sleep(TICK_S - dt)
                now = time.monotonic()
                dt = now - last_tick
            last_tick = now

            values = state.step(dt)

            if now >= next_msg1:
                data1 = msg1.encode({
                    "Speed": values["Speed"], "DriveMode": values["DriveMode"],
                    "DTC": values["DTC"], "Power": values["Power"], "RegenPower": values["RegenPower"],
                })
                bus.send(can.Message(arbitration_id=msg1.frame_id, data=data1, is_extended_id=False))
                next_msg1 += MSG1_PERIOD_S

            if now >= next_msg2:
                data2 = msg2.encode({
                    "SOC": values["SOC"], "DClinkVoltage": values["DClinkVoltage"],
                    "Temp": values["Temp"], "DriveRange": values["DriveRange"],
                    "Odometer": values["Odometer"],
                })
                bus.send(can.Message(arbitration_id=msg2.frame_id, data=data2, is_extended_id=False))
                next_msg2 += MSG2_PERIOD_S

            if now - last_print >= 1.0:
                last_print = now
                print(f"speed={values['Speed']:4d}km/h mode={values['DriveMode']} "
                      f"power={values['Power']:3d}kW regen={values['RegenPower']:3d}kW "
                      f"soc={values['SOC']:3d}% volt={values['DClinkVoltage']:2d}V "
                      f"temp={values['Temp']:3d}C range={values['DriveRange']:3d}km "
                      f"odo={values['Odometer']}km")
    except KeyboardInterrupt:
        print("\n종료합니다.")
    finally:
        bus.shutdown()


if __name__ == "__main__":
    main()
