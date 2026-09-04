# scripts/gen_icons.py
# Voltline 텔테일/상태 아이콘 9종을 LVGL A8(알파 전용) C 배열로 굽는다.
#
# 원본은 docs/Voltline 전기오토바이 클러스터 UXUI/icons/*.svg (24x24, 단색
# path/rect)지만, 이 Windows 환경엔 cairosvg의 네이티브 cairo DLL이 없고
# rsvg-convert/inkscape도 PATH에 없어(2026-09-02 확인) SVG를 그대로 래스터화할
# 도구가 없다. 모양이 단순한 아이콘들이라 SVG를 그대로 파싱하는 대신 PIL
# ImageDraw 프리미티브로 같은 의미의 글리프를 직접 그린다(디자인 시스템
# readme 자체가 "Lucide 대체품, ISO 인증 아님, 실제 양산 전 교체 필요"라고
# 명시하고 있어 지금 단계에서는 픽셀 단위로 SVG와 100% 일치할 필요가 없다).
#
# 4배 슈퍼샘플링 후 LANCZOS 다운샘플로 안티에일리어싱, 흰색 불투명 알파로
# 한 번만 굽고 런타임에 lv_obj_set_style_img_recolor로 신호색을 입힌다
# (main/ui/ui_icons.c의 ui_icon_set() 참고).
import math
from PIL import Image, ImageDraw

SS = 4  # 슈퍼샘플 배율

ICON_SIZES = [22, 26]  # 헤더/푸터(22px)와 텔테일 레일(26px)


def _canvas(size):
    s = size * SS
    img = Image.new("L", (s, s), 0)
    return img, ImageDraw.Draw(img)


def _finish(img, size):
    return img.resize((size, size), Image.LANCZOS)


def draw_battery(size):
    img, d = _canvas(size)
    s = size * SS
    pad = s * 0.12
    body = (pad, s * 0.28, s - pad - s * 0.10, s * 0.72)
    d.rounded_rectangle(body, radius=s * 0.06, outline=255, width=int(s * 0.09))
    nub = (body[2], s * 0.42, s - pad, s * 0.58)
    d.rectangle(nub, fill=255)
    return _finish(img, size)


def draw_bluetooth(size):
    img, d = _canvas(size)
    s = size * SS
    cx = s / 2
    top, bot, mid = s * 0.12, s * 0.88, s / 2
    left = s * 0.30
    right = s * 0.70
    w = int(s * 0.11)
    # 세로 중심선 + 위/아래 두 개의 X자 꺾임(고전 블루투스 룬 문자 모양)
    d.line([(cx, top), (cx, bot)], fill=255, width=w)
    d.line([(cx, top), (right, mid * 0.55)], fill=255, width=w)
    d.line([(right, mid * 0.55), (left, mid)], fill=255, width=w)
    d.line([(left, mid), (right, mid * 1.45)], fill=255, width=w)
    d.line([(right, mid * 1.45), (cx, bot)], fill=255, width=w)
    return _finish(img, size)


def draw_brake(size):
    img, d = _canvas(size)
    s = size * SS
    cx = cy = s / 2
    r = s * 0.36
    w = int(s * 0.09)
    d.ellipse((cx - r, cy - r, cx + r, cy + r), outline=255, width=w)
    d.line([(cx, cy - r * 0.55), (cx, cy + r * 0.15)], fill=255, width=w)
    dot_r = s * 0.045
    d.ellipse((cx - dot_r, cy + r * 0.35 - dot_r, cx + dot_r, cy + r * 0.35 + dot_r), fill=255)
    return _finish(img, size)


def draw_controller(size):
    img, d = _canvas(size)
    s = size * SS
    pad = s * 0.16
    w = int(s * 0.09)
    d.rectangle((pad, pad, s - pad, s - pad), outline=255, width=w)
    cs = s * 0.16
    cx = cy = s / 2
    d.rectangle((cx - cs / 2, cy - cs / 2, cx + cs / 2, cy + cs / 2), fill=255)
    return _finish(img, size)


def draw_triangle_bang(size, filled):
    img, d = _canvas(size)
    s = size * SS
    top = (s / 2, s * 0.10)
    bl = (s * 0.08, s * 0.90)
    br = (s * 0.92, s * 0.90)
    if filled:
        d.polygon([top, bl, br], fill=255)
        bar_col = 0
        dot_col = 0
    else:
        d.polygon([top, bl, br], outline=255, width=int(s * 0.08))
        bar_col = 255
        dot_col = 255
    cx = s / 2
    d.rectangle((cx - s * 0.045, s * 0.38, cx + s * 0.045, s * 0.66), fill=bar_col)
    dot_r = s * 0.05
    d.ellipse((cx - dot_r, s * 0.72 - dot_r, cx + dot_r, s * 0.72 + dot_r), fill=dot_col)
    return _finish(img, size)


def draw_ev_warning(size):
    return draw_triangle_bang(size, filled=True)


def draw_warning_tri(size):
    return draw_triangle_bang(size, filled=False)


def draw_highbeam(size):
    img, d = _canvas(size)
    s = size * SS
    bulb = (s * 0.10, s * 0.22, s * 0.52, s * 0.78)
    d.rounded_rectangle(bulb, radius=s * 0.12, outline=255, width=int(s * 0.08))
    ray_x0, ray_x1 = s * 0.62, s * 0.92
    for t in (0.30, 0.50, 0.70):
        y = s * t
        d.line([(ray_x0, y), (ray_x1, y)], fill=255, width=int(s * 0.08))
    return _finish(img, size)


def draw_temp(size):
    img, d = _canvas(size)
    s = size * SS
    stem_x0, stem_x1 = s * 0.42, s * 0.58
    d.rounded_rectangle((stem_x0, s * 0.10, stem_x1, s * 0.68), radius=(stem_x1 - stem_x0) / 2, outline=255,
                         width=int(s * 0.07))
    bulb_r = s * 0.18
    cx = s / 2
    cy = s * 0.78
    d.ellipse((cx - bulb_r, cy - bulb_r, cx + bulb_r, cy + bulb_r), fill=255)
    return _finish(img, size)


def draw_turn(size, mirrored):
    img, d = _canvas(size)
    s = size * SS
    tail = (s * 0.12, s * 0.38, s * 0.55, s * 0.62)
    d.rectangle(tail, fill=255)
    head = [(s * 0.50, s * 0.15), (s * 0.92, s * 0.50), (s * 0.50, s * 0.85)]
    d.polygon(head, fill=255)
    if mirrored:
        img = img.transpose(Image.FLIP_LEFT_RIGHT)
    return _finish(img, size)


ICONS = {
    "battery": draw_battery,
    "bluetooth": draw_bluetooth,
    "brake": draw_brake,
    "controller": draw_controller,
    "ev_warning": draw_ev_warning,
    "warning_tri": draw_warning_tri,
    "highbeam": draw_highbeam,
    "temp": draw_temp,
    "turn_r": lambda size: draw_turn(size, mirrored=False),
    "turn_l": lambda size: draw_turn(size, mirrored=True),
}


def write_c(all_bitmaps, out_c_path, out_h_path):
    with open(out_c_path, "w", encoding="utf-8") as f:
        f.write("#include \"ui_icons.h\"\n\n")
        for name, size, img in all_bitmaps:
            var = f"ui_icon_{name}_{size}"
            data = list(img.tobytes())  # "L" 모드 = 1바이트/픽셀 알파값 그대로
            f.write(f"static const uint8_t {var}_map[] = {{\n")
            for i in range(0, len(data), 16):
                chunk = data[i:i + 16]
                f.write("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",\n")
            f.write("};\n\n")
            f.write(f"const lv_img_dsc_t {var} = {{\n")
            f.write("    .header.always_zero = 0,\n")
            f.write(f"    .header.w = {size},\n")
            f.write(f"    .header.h = {size},\n")
            f.write(f"    .data_size = {size * size},\n")
            f.write("    .header.cf = LV_IMG_CF_ALPHA_8BIT,\n")
            f.write(f"    .data = {var}_map,\n")
            f.write("};\n\n")
        total = sum(size * size for _, size, _ in all_bitmaps)
        f.write(f"/* 총 알파 데이터: {total} bytes ({len(all_bitmaps)}개 비트맵) */\n")

    with open(out_h_path, "w", encoding="utf-8") as f:
        f.write("#pragma once\n")
        f.write("/* scripts/gen_icons.py 로 생성 — 직접 수정하지 말 것 */\n")
        f.write("#include \"lvgl.h\"\n\n")
        f.write("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")
        for name, size, _ in all_bitmaps:
            f.write(f"extern const lv_img_dsc_t ui_icon_{name}_{size};\n")
        f.write("\n/* tint: 신호색 등 런타임 틴트. off_opa: 꺼짐 상태 opa(0-255), 항상 hidden 아님 */\n")
        f.write("void ui_icon_set(lv_obj_t *img_obj, const lv_img_dsc_t *src, lv_color_t tint, lv_opa_t opa);\n")
        f.write("\n#ifdef __cplusplus\n}\n#endif\n")

    print(f"wrote {out_c_path} / {out_h_path} - {len(all_bitmaps)} bitmaps, {total} bytes total")


if __name__ == "__main__":
    all_bitmaps = []
    for name, fn in ICONS.items():
        for size in ICON_SIZES:
            all_bitmaps.append((name, size, fn(size)))
    write_c(all_bitmaps, "main/ui/ui_icons.c", "main/ui/ui_icons.h")
