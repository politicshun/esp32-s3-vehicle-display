# scripts/gen_gauge_face.py
# 게이지 다이얼 페이스(눈금/숫자/트랙 홈/경고구역)를 고품질 정적 이미지로 미리 그려서
# LVGL true-color(RGB565) C 배열로 저장한다. 예전엔 lv_meter가 런타임에 눈금/숫자를
# 그렸는데(main/ui/ui.c make_gauge_meter), 얇고 밋밋해서 "조잡해 보인다"는 피드백(2026-08-04)
# — PIL로 안티에일리어싱된 눈금/숫자를 미리 그려서 훨씬 정교하게 만들고, LVGL은 그 위에
# 움직이는 값 아크(현재 속도/파워/SOC를 나타내는 색 채움)만 실시간으로 그린다.
#
# 각도/반지름 공식은 managed_components/lvgl__lvgl/src/extra/widgets/meter/lv_meter.c를
# 직접 읽고 그대로 가져온 것 — 어긋나면 라이브 아크와 이 이미지의 트랙이 안 맞아 보인다:
#   scale_area = 위젯 content area (테두리+패딩 뺀 영역, lv_obj_get_content_coords),
#     r_out = scale_area 폭/2
#   각도(도) = rotation + (value-min)/(max-min) * angle_range   (0도=3시 방향, 시계방향 증가)
#   점 좌표: x = cx + r*cos(rad), y = cy + r*sin(rad)  (PIL도 y+ 아래쪽이라 부호 그대로 씀)
#   lv_meter_add_arc(meter, scale, width, color, r_mod)의 실제 반지름 = r_out + r_mod,
#   그 반지름을 중심으로 위아래 width/2씩 걸쳐서 그려짐(lv_draw_arc 관례)
#
# 2026-08-05: r_out = meter_size/2 그대로 써야 한다 — 예전엔 "테두리 1px"를 빼고 있었는데,
# 실제로는 border_width=0이라 뺄 게 없다(그래서 그냥 -1*SS를 지움). 대신 진짜 원인은 따로
# 있었다: sdkconfig CONFIG_LV_USE_THEME_DEFAULT=y가 lv_meter에 기본 "card" 스타일의
# pad_all(DPI_DEF=130 기준 약 19px)을 입혀서, get_content_coords가 그 padding만큼 깎인
# scale_area를 돌려주고 있었다 — main/ui/ui.c make_gauge_meter()에서
# lv_obj_set_style_pad_all(meter, 0, LV_PART_MAIN)으로 지워야 이 파일이 가정하는 r_out과
# 실제 라이브 아크의 r_out이 일치한다(안 지우면 baked 트랙과 라이브 아크가 서로 다른
# 반지름에 그려져 "이중 링"처럼 보임 — 실제로 겪은 버그).
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFont

BG = (0x0A, 0x0E, 0x14)
GLOW = (0x2F, 0xD8, 0xE8)  # ui_glow_speed/soc와 동일 톤 — 이제 별도 레이어 대신 이 이미지 배경에 직접 구움
TEXT_SEC = (0x8A, 0x97, 0xA8)
TEXT_PRI = (0xFF, 0xFF, 0xFF)
RED = (0xE8, 0x4C, 0x3C)
TRACK_DIM = (0x1A, 0x22, 0x30)  # 값 아크가 채워질 빈 홈 — BG보다 살짝 밝은 정도

FONT_PATH = "C:/Users/Moty_SJ/AppData/Local/Temp/claude/C--esp-workspace-esp32-s3-vehicle-display/c014e684-0a2c-4355-8338-6c449ae6b38f/scratchpad/font/Rajdhani-Bold.ttf"

ROTATION = 135
ANGLE_RANGE = 270
SS = 4  # 슈퍼샘플링 배수 — 4배 크기로 그린 뒤 축소해서 안티에일리어싱 품질을 높임


def polar(cx, cy, r, angle_deg):
    rad = math.radians(angle_deg)
    return (cx + r * math.cos(rad), cy + r * math.sin(rad))


def draw_gauge_face(meter_size, canvas_size, min_v, max_v, tick_step, major_step, label_fmt,
                     redline_start, redline_end, out_c_path, var_name,
                     font_size=15, png_preview_path=None,
                     track_r_mod=34, track_w=14):
    S = canvas_size * SS
    M = meter_size * SS  # r_out 등 lv_meter 실측 기하는 위젯의 실제 크기(meter_size) 기준
    cx = cy = S / 2

    # 배경에 은은한 시안 글로우를 직접 구워 넣음 (ui_glow_speed/soc를 대체 — 별도 레이어로
    # 얹으면 이 이미지가 불투명이라 아래 글로우를 가려버리므로, 아예 이 배경에 합성해둠).
    # gen_glow_image.py와 동일한 감쇠 공식(중심에서 intensity, 가장자리 0으로 제곱 감쇠).
    # numpy로 벡터화(캔버스가 커서 순수 파이썬 픽셀 루프는 너무 느림).
    glow_r = M / 2
    yy, xx = np.mgrid[0:S, 0:S]
    dist = np.hypot(xx - cx, yy - cy) / glow_r
    t = (np.clip(1.0 - np.clip(dist, 0.0, 1.0), 0.0, None) ** 2) * 0.55
    bg_arr = np.array(BG, dtype=np.float32)
    glow_arr = np.array(GLOW, dtype=np.float32)
    arr = bg_arr + (glow_arr - bg_arr) * t[..., None]
    img = Image.fromarray(arr.astype(np.uint8), mode="RGB")

    d = ImageDraw.Draw(img)
    # lv_meter의 scale_area 반지름: border_width=0, pad_all=0(main/ui/ui.c make_gauge_meter())
    # 이므로 content area가 위젯 전체 크기와 같다 — 뺄 것 없이 그대로 절반.
    r_out = M / 2

    font = ImageFont.truetype(FONT_PATH, font_size * SS)

    def angle_of(v):
        return ROTATION + (v - min_v) / (max_v - min_v) * ANGLE_RANGE

    # 1) 값 아크가 채워질 빈 트랙 홈 (radius = r_out-track_r_mod, width=track_w,
    # main/ui/ui.c의 lv_meter_add_arc(...) 호출과 반드시 같은 값이어야 함 — 기본값(34/14)은
    # speed용, Power/SOC는 2026-08-05 "숫자랑 겹친다"는 피드백으로 더 크고 눈금 쪽에 가깝게
    # (track_r_mod=22, track_w=16) 오버라이드해서 호출한다.
    track_r = r_out - track_r_mod * SS
    track_w_px = track_w * SS
    bbox = [cx - track_r - track_w_px / 2, cy - track_r - track_w_px / 2,
            cx + track_r + track_w_px / 2, cy + track_r + track_w_px / 2]
    d.arc(bbox, angle_of(min_v), angle_of(max_v), fill=TRACK_DIM, width=int(track_w_px))

    # 2) 경고구역 정적 아크 (radius = r_out-8, width=6, main/ui/ui.c와 동일)
    if redline_end > redline_start:
        warn_r = r_out - 8 * SS
        warn_w = 6 * SS
        bbox = [cx - warn_r - warn_w / 2, cy - warn_r - warn_w / 2,
                cx + warn_r + warn_w / 2, cy + warn_r + warn_w / 2]
        d.arc(bbox, angle_of(redline_start), angle_of(redline_end), fill=RED, width=int(warn_w))

    # 3) 눈금 — 트랙/경고구역 바깥쪽에 그림 (minor는 얇고 어둡게, major는 굵고 밝게 + 숫자).
    # 숫자는 major tick 끝(major_r2)에서 충분히 더 떨어뜨려서(label_gap) 눈금선과 안 겹치게 함.
    minor_r1, minor_r2 = r_out + 4 * SS, r_out + 13 * SS
    major_r1, major_r2 = r_out + 2 * SS, r_out + 20 * SS
    label_gap = 20 * SS
    label_r = major_r2 + label_gap

    n_steps = round((max_v - min_v) / tick_step)
    for i in range(n_steps + 1):
        v = min_v + i * tick_step  # 정수 i로 계산해서 float 누적오차로 major 판정이 틀어지는 것 방지
        is_major = round(v - min_v) % major_step == 0
        ang = angle_of(v)
        if is_major:
            p1 = polar(cx, cy, major_r1, ang)
            p2 = polar(cx, cy, major_r2, ang)
            d.line([p1, p2], fill=TEXT_PRI, width=int(3 * SS))
            lx, ly = polar(cx, cy, label_r, ang)
            label = label_fmt(v)
            tb = d.textbbox((0, 0), label, font=font)
            tw, th = tb[2] - tb[0], tb[3] - tb[1]
            d.text((lx - tw / 2 - tb[0], ly - th / 2 - tb[1]), label, fill=TEXT_SEC, font=font)
        else:
            p1 = polar(cx, cy, minor_r1, ang)
            p2 = polar(cx, cy, minor_r2, ang)
            d.line([p1, p2], fill=TEXT_SEC, width=int(SS))
        v += tick_step

    img = img.resize((canvas_size, canvas_size), Image.LANCZOS)
    _write_lv_img_c(img, canvas_size, canvas_size, out_c_path, var_name, png_preview_path)


def _write_lv_img_c(img, canvas_w, canvas_h, out_c_path, var_name, png_preview_path=None):
    """PIL Image -> LVGL true-color(RGB565) C 배열로 저장. draw_gauge_face(원형)와
    draw_bar_face(막대형)가 공유하는 인코딩/출력 로직 — 캔버스가 정사각형이 아니어도 된다."""
    if png_preview_path:
        img.save(png_preview_path)
        print(f"wrote preview {png_preview_path}")

    arr = np.asarray(img, dtype=np.uint16)  # (H, W, 3)
    r = arr[:, :, 0] >> 3
    g = arr[:, :, 1] >> 2
    b = arr[:, :, 2] >> 3
    val = (r << 11) | (g << 5) | b
    out = np.empty((canvas_h, canvas_w, 2), dtype=np.uint8)
    out[:, :, 0] = val & 0xFF
    out[:, :, 1] = (val >> 8) & 0xFF
    rgb565_bytes = out.tobytes()

    with open(out_c_path, "w", encoding="utf-8") as f:
        f.write("#include \"lvgl.h\"\n\n")
        f.write(f"static const uint8_t {var_name}_map[] = {{\n")
        for i in range(0, len(rgb565_bytes), 16):
            chunk = rgb565_bytes[i:i + 16]
            f.write("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",\n")
        f.write("};\n\n")
        f.write(f"const lv_img_dsc_t {var_name} = {{\n")
        f.write("    .header.always_zero = 0,\n")
        f.write(f"    .header.w = {canvas_w},\n")
        f.write(f"    .header.h = {canvas_h},\n")
        f.write(f"    .data_size = {canvas_w * canvas_h} * LV_COLOR_SIZE / 8,\n")
        f.write("    .header.cf = LV_IMG_CF_TRUE_COLOR,\n")
        f.write(f"    .data = {var_name}_map,\n")
        f.write("};\n")

    print(f"wrote {out_c_path} ({canvas_w}x{canvas_h}, {len(rgb565_bytes)} bytes pixel data)")


# ---------------------------------------------------------------------------
# 막대형(선형) 게이지 페이스 — 2026-08-05, Pack Voltage/Sys Temp는 "급격히 변하는 값이
# 아닌데 원형 아크는 과하다"는 피드백으로 원형 대신 채택. 원형 게이지와 같은 아이디어
# (트랙 홈은 baked 이미지, 그 위에 움직이는 값 채움만 LVGL lv_bar로 실시간 렌더)를
# 가로 막대에 적용 — 위험구간은 원형의 "레드존 정적 아크"와 동일하게, 트랙 바로 아래
# 별도의 얇은 정적 스트립으로 표시해서 현재값 채움 색과 겹치지 않고 항상 보이게 한다.
#
# 아래 GROOVE_* 상수는 main/ui/ui.c의 UI_BAR_GAUGE_* 매크로와 반드시 같은 값이어야 한다 —
# lv_bar 라이브 인디케이터를 이 트랙 홈 위에 픽셀 단위로 겹쳐 그리기 때문(어긋나면 채움 막대와
# baked 트랙 테두리가 안 맞아 보임).
BAR_CANVAS_W = 340
BAR_CANVAS_H = 130
BAR_GROOVE_W = 300
BAR_GROOVE_H = 28
BAR_GROOVE_RADIUS = 14
BAR_GROOVE_CY = 34  # 캔버스 top 기준 트랙 홈 세로 중심


def draw_bar_face(min_v, max_v, tick_step, major_step, label_fmt,
                   warn_start, warn_end, out_c_path, var_name,
                   font_size=13, png_preview_path=None):
    S_W, S_H = BAR_CANVAS_W * SS, BAR_CANVAS_H * SS
    GROOVE_W, GROOVE_H, GROOVE_RADIUS = BAR_GROOVE_W * SS, BAR_GROOVE_H * SS, BAR_GROOVE_RADIUS * SS
    GROOVE_CY = BAR_GROOVE_CY * SS
    cx = S_W / 2
    groove_x0, groove_x1 = cx - GROOVE_W / 2, cx + GROOVE_W / 2
    groove_y0, groove_y1 = GROOVE_CY - GROOVE_H / 2, GROOVE_CY + GROOVE_H / 2

    # 트랙 홈 중심에 은은한 시안 글로우 (원형 게이지와 동일 감쇠 공식, 캔버스가 가로로 길어서
    # 반지름 기준점만 그루브 절반 폭으로 잡음).
    glow_r = GROOVE_W / 2
    yy, xx = np.mgrid[0:S_H, 0:S_W]
    dist = np.hypot(xx - cx, yy - GROOVE_CY) / glow_r
    t = (np.clip(1.0 - np.clip(dist, 0.0, 1.0), 0.0, None) ** 2) * 0.4
    bg_arr = np.array(BG, dtype=np.float32)
    glow_arr = np.array(GLOW, dtype=np.float32)
    arr = bg_arr + (glow_arr - bg_arr) * t[..., None]
    img = Image.fromarray(arr.astype(np.uint8), mode="RGB")

    d = ImageDraw.Draw(img)
    font = ImageFont.truetype(FONT_PATH, font_size * SS)

    def x_of(v):
        return groove_x0 + (v - min_v) / (max_v - min_v) * GROOVE_W

    # 1) 값 채움 막대(lv_bar)가 얹힐 빈 트랙 홈
    d.rounded_rectangle([groove_x0, groove_y0, groove_x1, groove_y1],
                         radius=GROOVE_RADIUS, fill=TRACK_DIM)

    # 2) 위험구간 — 트랙 홈 바로 아래 별도의 얇은 정적 스트립(원형 게이지의 레드존 아크와
    # 동일 의도: 값 채움 막대와 겹치지 않는 위치에 항상 보이게 그려서, 채움이 그 구간을
    # 덮어도 경고구간 자체는 가려지지 않는다).
    if warn_end > warn_start:
        warn_y0 = groove_y1 + 6 * SS
        warn_y1 = warn_y0 + 8 * SS
        wx0, wx1 = x_of(warn_start), x_of(warn_end)
        d.rounded_rectangle([wx0, warn_y0, wx1, warn_y1], radius=4 * SS, fill=RED)

    # 3) 눈금 — 위험구간 스트립이 있든 없든 항상 같은 y위치(두 게이지 간 시각적 정렬 유지).
    minor_y0, minor_y1 = groove_y1 + 20 * SS, groove_y1 + 28 * SS
    major_y0, major_y1 = groove_y1 + 18 * SS, groove_y1 + 32 * SS
    label_y = major_y1 + 10 * SS

    n_steps = round((max_v - min_v) / tick_step)
    for i in range(n_steps + 1):
        v = min_v + i * tick_step
        is_major = round(v - min_v) % major_step == 0
        x = x_of(v)
        if is_major:
            d.line([(x, major_y0), (x, major_y1)], fill=TEXT_PRI, width=int(3 * SS))
            label = label_fmt(v)
            tb = d.textbbox((0, 0), label, font=font)
            tw, th = tb[2] - tb[0], tb[3] - tb[1]
            d.text((x - tw / 2 - tb[0], label_y - tb[1]), label, fill=TEXT_SEC, font=font)
        else:
            d.line([(x, minor_y0), (x, minor_y1)], fill=TEXT_SEC, width=int(SS))

    img = img.resize((BAR_CANVAS_W, BAR_CANVAS_H), Image.LANCZOS)
    _write_lv_img_c(img, BAR_CANVAS_W, BAR_CANVAS_H, out_c_path, var_name, png_preview_path)


if __name__ == "__main__":
    # Speed: 0~200km/h, 10단위 눈금/20단위 숫자, 160~200 레드존 (main/ui/ui.c SPEED_GAUGE_* 상수와 동일)
    PREVIEW_DIR = "C:/Users/Moty_SJ/AppData/Local/Temp/claude/C--esp-workspace-esp32-s3-vehicle-display/c014e684-0a2c-4355-8338-6c449ae6b38f/scratchpad"

    # Speed: 0~200km/h, 10단위 눈금/20단위 숫자, 160~200 레드존 (main/ui/ui.c SPEED_GAUGE_* 상수와 동일)
    draw_gauge_face(280, 400, 0, 200, 10, 20, lambda v: str(int(v)),
                     160, 200, "main/ui/ui_gauge_speed.c", "ui_gauge_speed_face",
                     font_size=15,
                     png_preview_path=f"{PREVIEW_DIR}/gauge_speed_preview.png")

    # Power: 0~100kW, 10단위 눈금/20단위 숫자, 레드존 없음 (UI_POWER_GAUGE_MAX_KW과 동일)
    # 2026-08-05(1차): 기본 트랙 반지름(34/14, speed와 동일)이 너무 안쪽이라 중앙 숫자 라벨과
    # 겹친다는 피드백 — 아크를 더 굵고(14->16) 눈금 쪽으로 더 가깝게(34->22) 옮김.
    # 2026-08-05(2차): "그래도 좁은 느낌" 피드백으로 폭을 16->20으로 한 번 더 키움(반지름은
    # 20-24로 살짝만 안쪽으로 물려서 경고구역과의 여유는 그대로 유지).
    # main/ui/ui.c make_gauge_meter()의 lv_meter_add_arc(...,20,...,-24)와 반드시 같은 값.
    draw_gauge_face(200, 300, 0, 100, 10, 20, lambda v: str(int(v)),
                     0, 0, "main/ui/ui_gauge_power.c", "ui_gauge_power_face",
                     font_size=13, track_r_mod=24, track_w=20,
                     png_preview_path=f"{PREVIEW_DIR}/gauge_power_preview.png")

    # SOC: 0~100%, 10단위 눈금/20단위 숫자, 0~20 저잔량 경고구역 (UI_SOC_LOW_PCT와 동일)
    # Power와 동일한 이유로 트랙을 24/20으로 확대 — 경고구역(warn_r=r_out-8, width6, span
    # r_out-11~r_out-5)과는 여전히 3px 여유가 있어 겹치지 않음(트랙 바깥쪽 끝 r_out-14).
    draw_gauge_face(200, 300, 0, 100, 10, 20, lambda v: str(int(v)),
                     0, 20, "main/ui/ui_gauge_soc.c", "ui_gauge_soc_face",
                     font_size=13, track_r_mod=24, track_w=20,
                     png_preview_path=f"{PREVIEW_DIR}/gauge_soc_preview.png")

    # Pack Voltage: 0~80V — docs/design/can-signals.md InvMsg2 byte1 [0|80] V 그대로 사용
    # (실차 스펙 자체가 이 범위, UI 임의 스케일 아님). 위험구간 없음(저/과전압 경고 기준 미정,
    # main/ui/ui_style.h 참고 — 온도만 UI_TEMP_WARN_C로 확정돼 있음).
    # 2026-08-05: 처음엔 speed/power/soc와 같은 원형 아크로 만들었는데, "전압/온도는 급격히
    # 안 변하는 값인데 아크는 과하다, 막대+위험구간 표시가 낫겠다"는 피드백으로 막대형으로 교체.
    draw_bar_face(0, 80, 10, 20, lambda v: str(int(v)),
                  0, 0, "main/ui/ui_gauge_volt.c", "ui_gauge_volt_face",
                  font_size=13,
                  png_preview_path=f"{PREVIEW_DIR}/gauge_volt_preview.png")

    # Sys Temp: 실제 CAN 범위는 -20~235degC(docs/design/can-signals.md)지만 그대로 쓰면
    # 정상 동작 구간(상온 근처)이 눈금 한쪽 구석에 몰려 안 보임 — speed/power 게이지와 같은
    # 이유로 -20~120을 "UI 표시 스케일"로 잡음(실차 스펙 아님, 235까지 찍히면 라벨 숫자는
    # 그대로 그 값을 보여주되 채움 막대만 120에서 clamp, main/ui/ui.c anim_power_exec_cb와
    # 동일 패턴). 위험구간은 UI_TEMP_WARN_C(60, main/ui/ui_style.h)~120.
    draw_bar_face(-20, 120, 20, 40, lambda v: str(int(v)),
                  60, 120, "main/ui/ui_gauge_temp.c", "ui_gauge_temp_face",
                  font_size=13,
                  png_preview_path=f"{PREVIEW_DIR}/gauge_temp_preview.png")
