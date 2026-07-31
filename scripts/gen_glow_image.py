# scripts/gen_glow_image.py
# 게이지 뒤에 깔 정적 "글로우" 배경을 생성해서 LVGL true-color(RGB565) C 배열로 저장한다.
# main/ui/ui_style.c의 UI_COLOR_BG(#0A0E14)/UI_COLOR_CYAN(#2FD8E8)과 맞춰서,
# 중심은 은은한 시안 글로우, 가장자리는 배경색으로 자연스럽게 페이드아웃되게 그린다.
# (런타임에 매 프레임 재계산하는 lv shadow와 달리, 이건 한 번만 그려서 정적 비트맵으로 blit함)
import math
from PIL import Image

BG = (0x0A, 0x0E, 0x14)
GLOW = (0x2F, 0xD8, 0xE8)


def make_glow(size, intensity, out_c_path, var_name):
    img = Image.new("RGB", (size, size))
    cx = cy = size / 2
    max_r = size / 2
    px = img.load()
    for y in range(size):
        for x in range(size):
            d = math.hypot(x - cx, y - cy) / max_r
            d = min(d, 1.0)
            # 중심에서 intensity, 가장자리에서 0으로 부드럽게 (제곱으로 빠르게 감쇠)
            t = max(0.0, 1.0 - d) ** 2 * intensity
            r = int(BG[0] + (GLOW[0] - BG[0]) * t)
            g = int(BG[1] + (GLOW[1] - BG[1]) * t)
            b = int(BG[2] + (GLOW[2] - BG[2]) * t)
            px[x, y] = (r, g, b)

    rgb565_bytes = bytearray()
    for y in range(size):
        for x in range(size):
            r, g, b = img.getpixel((x, y))
            v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            rgb565_bytes.append(v & 0xFF)
            rgb565_bytes.append((v >> 8) & 0xFF)

    with open(out_c_path, "w", encoding="utf-8") as f:
        f.write("#include \"lvgl.h\"\n\n")
        f.write(f"static const uint8_t {var_name}_map[] = {{\n")
        for i in range(0, len(rgb565_bytes), 16):
            chunk = rgb565_bytes[i:i + 16]
            f.write("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",\n")
        f.write("};\n\n")
        f.write(f"const lv_img_dsc_t {var_name} = {{\n")
        f.write("    .header.always_zero = 0,\n")
        f.write(f"    .header.w = {size},\n")
        f.write(f"    .header.h = {size},\n")
        f.write(f"    .data_size = {size * size} * LV_COLOR_SIZE / 8,\n")
        f.write("    .header.cf = LV_IMG_CF_TRUE_COLOR,\n")
        f.write(f"    .data = {var_name}_map,\n")
        f.write("};\n")

    print(f"wrote {out_c_path} ({size}x{size}, {len(rgb565_bytes)} bytes pixel data)")


if __name__ == "__main__":
    # 실제 표시 크기(280/240)보다 훨씬 작게 만들고 LVGL zoom으로 확대해서 사용한다.
    # 글로우는 고주파 디테일이 없는 부드러운 그라데이션이라 저해상도로 만들어도
    # 확대 시 티가 안 나고, 대신 플래시 사용량이 원본 해상도 대비 1/19 수준으로 줄어든다
    # (280x280 원본 156800바이트 -> 64x64 8192바이트).
    make_glow(64, 0.55, "main/ui/ui_glow_speed.c", "ui_glow_speed")
    make_glow(64, 0.55, "main/ui/ui_glow_soc.c", "ui_glow_soc")
