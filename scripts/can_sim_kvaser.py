# scripts/can_sim_kvaser.py
# KVASER 실물 어댑터로 실제 구동 사이클(정차->가속->순항->감속->후진->정차, 반복)을
# 흉내내서 InvMsg1(0x100, 100ms)/InvMsg2(0x200, 200ms)를 계속 쏴주는 벤치 테스트용 시뮬레이터.
# CANKing의 Generator는 "고정값 주기 반복"만 되고 값이 시간에 따라 변하는 건 못 해서,
# docs/hardware/vehicle.dbc를 cantools로 읽어 실측 스펙(factor/offset/범위) 그대로
# 인코딩하고, python-can의 kvaser 백엔드(canlib32.dll)로 실제 버스에 낸다.
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

DBC_PATH = Path(__file__).resolve().parent.parent / "docs" / "hardware" / "vehicle.dbc"

MSG1_PERIOD_S = 0.100  # InvMsg1 (docs/hardware/vehicle.dbc GenMsgCycleTime와 동일)
MSG2_PERIOD_S = 0.200  # InvMsg2
TICK_S = 0.02          # 시뮬레이션 상태 갱신 주기 (LVGL 쪽 20ms 루프와 맞춤 — 필수는 아님)

# 주행 사이클: (phase 이름, 지속시간(s), DriveMode 0=P 1=R 2=N 3=D)
CYCLE = [
    ("PARK",         5.0, 0),
    ("ACCEL",       10.0, 3),
    ("CRUISE",      10.0, 3),
    ("DECEL",        8.0, 3),
    ("REVERSE",      5.0, 1),
]
CYCLE_TOTAL_S = sum(seg[1] for seg in CYCLE)

TARGET_CRUISE_KMH = 80.0
REVERSE_KMH = -5.0
FULL_RANGE_KM = 300.0  # SOC 100% 기준 예상 주행가능거리 (표시용 가정치)


class DriveState:
    """실제 구동 사이클을 시간(t)에 따라 흉내내는 상태 머신.
    물리적으로 정밀한 시뮬레이션이 아니라, 벤치 테스트에서 '그럴듯하게 변하는 값'을
    만드는 게 목적이다 (가속/순항/감속/후진에 맞춰 speed/power/regen이 같이 움직임)."""

    def __init__(self, start_odo_km: float = 12345.0, start_soc: float = 80.0):
        self.t = 0.0
        self.odo_km = start_odo_km
        self.soc = start_soc

    def _phase_at(self, t_in_cycle: float):
        acc = 0.0
        for name, dur, mode in CYCLE:
            if t_in_cycle < acc + dur:
                return name, mode, (t_in_cycle - acc) / dur  # (이름, 모드, 그 구간 내 진행률 0~1)
            acc += dur
        return CYCLE[-1][0], CYCLE[-1][2], 1.0

    def step(self, dt: float):
        self.t += dt
        t_in_cycle = self.t % CYCLE_TOTAL_S
        phase, mode, progress = self._phase_at(t_in_cycle)

        speed = 0.0
        power = 0.0
        regen = 0.0

        if phase == "PARK":
            speed = 0.0
        elif phase == "ACCEL":
            speed = TARGET_CRUISE_KMH * progress
            power = 60.0 * min(1.0, progress * 1.5)  # 초반에 힘 세게, 뒤로 갈수록 완화
        elif phase == "CRUISE":
            speed = TARGET_CRUISE_KMH + 2.0 * math.sin(self.t * 2.0)  # 미세한 흔들림
            power = 15.0
        elif phase == "DECEL":
            speed = TARGET_CRUISE_KMH * (1.0 - progress)
            regen = 40.0 * math.sin(progress * math.pi)  # 감속 중간에 회생 피크
        elif phase == "REVERSE":
            speed = REVERSE_KMH
            power = 2.0

        # 주행거리 누적 (km/h * s -> km), 후진도 절대값으로 누적
        self.odo_km += abs(speed) / 3600.0 * dt
        # SOC: 출력만큼 소모, 회생만큼 일부 회복 (그럴듯한 비율일 뿐 실측 아님)
        self.soc -= power * dt * 0.0006
        self.soc += regen * dt * 0.0002
        self.soc = max(0.0, min(100.0, self.soc))

        voltage = 72.0 - power * 0.05 + regen * 0.02
        voltage = max(0.0, min(80.0, voltage))
        temp = 25.0 + power * 0.15 - (0.0 if power > 0 else 0.5)
        temp = max(-20.0, min(235.0, temp))
        range_km = self.soc / 100.0 * FULL_RANGE_KM

        return {
            "Speed": round(speed),
            "DriveMode": mode,
            "DTC": 0,
            "Power": round(max(0.0, power)),
            "RegenPower": round(max(0.0, regen)),
            "SOC": round(self.soc),
            "DClinkVoltage": round(voltage),
            "Temp": round(temp),
            "DriveRange": round(min(255.0, range_km)),
            "Odometer": round(min(327675.0, self.odo_km)),
        }, phase


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

    db = cantools.database.load_file(str(DBC_PATH))
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

            values, phase = state.step(dt)

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
                print(f"[{phase:7s}] speed={values['Speed']:4d}km/h mode={values['DriveMode']} "
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
