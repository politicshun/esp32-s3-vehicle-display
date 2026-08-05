#include "ui.h"
#include "ui_style.h"
#include "vehicle_data.h"
#include <stdio.h>
#include <stdbool.h>

/* 게이지 다이얼 페이스(눈금/숫자/트랙 홈/경고구역/글로우 배경) 정적 이미지
 * (scripts/gen_gauge_face.py로 생성, main/ui/ui_gauge_*.c). 2026-08-04: 예전엔 lv_meter가
 * 런타임에 눈금/숫자를 그렸는데 얇고 밋밋해서 "조잡해 보인다"는 피드백 — PIL로
 * 안티에일리어싱된 다이얼을 미리 구워서 훨씬 정교하게 만들고, LVGL은 그 위에 움직이는
 * 값 아크(현재 속도/파워/SOC 채움)만 실시간으로 그린다(make_gauge_meter 참고). */
extern const lv_img_dsc_t ui_gauge_speed_face;
extern const lv_img_dsc_t ui_gauge_power_face;
extern const lv_img_dsc_t ui_gauge_soc_face;
extern const lv_img_dsc_t ui_gauge_volt_face;
extern const lv_img_dsc_t ui_gauge_temp_face;

/*
 * main/include/vehicle_data.h 의 VehicleData_t 필드:
 *   speed(int16_t, 음수=후진), soc(uint8_t), pack_volt(float), dtc_code(uint8_t),
 *   ble_connected(bool), drive_mode(uint8_t), odo_km(uint32_t), range_km(uint16_t),
 *   power_kw(float), regen_kw(float), sys_temp_c(int16_t)
 *
 * 2026-07-31: 차량단(인버터) 설계가 아직 안 끝나서, 이 프로젝트가 CAN 스펙을 먼저 정하고
 * 인버터 쪽이 거기 맞추기로 함 — docs/hardware/cluster.dbc(InvMsg1 0x100 / InvMsg2 0x200)에
 * 우리가 직접 정의한 스펙이다. 화면 바인딩은 완료했지만 인버터 실물 구현/실기 검증 전이다.
 *
 * TRIP/시계/외기온도는 여전히 VehicleData_t에 소스가 없어 "HARNESS-TODO" 정적
 * placeholder로 남아있다. 필드가 추가되면 아래 ui_update()의 해당 블록만 채우면 된다.
 */

/* 속도 게이지 표시 범위 — 실차 최고속도 스펙이 아니라 UI 표시 스케일 값.
 * 클러스터 UI에서 흔히 쓰는 라운드 값(200km/h)으로 설정, 실차 스펙 확정되면 조정 가능. */
#define SPEED_GAUGE_MAX_KMH 200
/* 레드존 시작값도 실차 스펙이 아니라 계기판 장식용 UI 스케일 값 (최대치의 80% 지점). */
#define SPEED_GAUGE_REDLINE_START 160

#define UI_CARD_W 160
#define UI_CARD_H 78

static lv_obj_t *tileview;
static lv_obj_t *pages[4];
static lv_obj_t *page_dots[4];

/* Page 1 — 주행 필수 정보 */
#define UI_SPEED_GRAD_SEGMENTS 8
static lv_obj_t *meter_speed;
/* 값 아크를 진한 블루->시안으로 이어지는 세그먼트 여러 개로 근사해서 그라데이션처럼 보이게 함
 * (LVGL 8.x lv_arc/lv_meter 아크는 단색만 지원, 진짜 그라데이션 아크가 없음) */
static lv_meter_indicator_t *speed_grad_segs[UI_SPEED_GRAD_SEGMENTS];
static int32_t speed_seg_bounds[UI_SPEED_GRAD_SEGMENTS + 1];
static lv_obj_t *lbl_speed;
static lv_obj_t *lbl_gear;        /* drive_mode 바인딩 (placeholder, CAN 0x302) */
static lv_obj_t *lbl_odo;         /* odo_km 바인딩 (placeholder, CAN 0x303) */
static lv_obj_t *lbl_trip_todo;   /* HARNESS-TODO: trip 필드 없음 (범위 밖) */

/* Page 2 — 전력 및 배터리
 * 2026-07-31: Power/Regen을 텍스트 2줄 카드로 넣었더니 UI_CARD_H(78px)에 못 들어가고
 * 잘려서(사용자 확인), 카드 대신 SOC와 대칭인 Power 게이지를 왼쪽에 추가하는 구조로 교체.
 * 2026-07-31(2차): 게이지 2개가 좌우 구석에 몰려 답답해 보인다는 피드백 + Regen을
 * 텍스트 대신 "배터리 차듯이" 채워지는 막대로 보여달라는 요청 반영 — 여백을 넓히고
 * (24px->36px), Regen을 lv_bar(배터리st 충전 표시처럼 초록 막대가 채워지는 방식)로 교체. */
#define UI_POWER_GAUGE_MAX_KW 100  /* 실차 스펙 아님 — UI 표시 스케일 값 (레드존 없음, 과열 아니라 정상 출력 영역) */
#define UI_REGEN_BAR_MAX_KW 50     /* 실차 스펙 아님 — UI 표시 스케일 값, 감속 시 회생 피크가 대략 이 정도라고 가정(can_sim_kvaser.py 기준) */
static lv_obj_t *meter_power;
static lv_meter_indicator_t *power_value_indic;
static lv_obj_t *lbl_power_kw;
static lv_obj_t *lbl_power_unit;      /* "kW" 고정 텍스트 — Regen 위젯들 정렬 기준점으로 ui_update()에서도 참조 */
static lv_obj_t *lbl_regen_caption;   /* "REGEN" 고정 캡션 — lbl_regen_kw 재정렬 기준점 */
static lv_obj_t *lbl_regen_kw;
static lv_obj_t *bar_regen;           /* regen_kw를 배터리 충전 막대처럼 표시 */
static lv_obj_t *meter_soc;
static lv_meter_indicator_t *soc_value_indic;
static lv_obj_t *lbl_soc_pct;
static lv_obj_t *lbl_soc_unit;    /* "%" — 2026-08-05: 숫자와 한 줄이라 SOC 아크 확대 후에도
                                   * 겹쳐 보인다는 피드백으로 Power의 lbl_power_unit과 동일하게
                                   * 줄바꿈 + 축소 */
static lv_obj_t *lbl_pack_volt;
static lv_obj_t *lbl_range;        /* range_km 바인딩 (placeholder, CAN 0x304) */

/* Page 3 — 시스템 상태 및 진단
 * 2026-08-05(1차): Pack Voltage/Sys Temp가 그냥 사각형 카드에 텍스트만 들어있어 "너무
 * 단순하다"는 피드백 — page2의 Power/SOC처럼 원형 아크 게이지로 교체했었음.
 * 2026-08-05(2차): "전압/온도는 급격히 변하는 값이 아닌데 원형 아크는 과하다, 막대+위험구간
 * 표시가 낫겠다"는 피드백으로 재교체 — scripts/gen_gauge_face.py의 draw_bar_face()가 구운
 * 가로 막대 트랙 홈 이미지 위에, lv_bar 라이브 인디케이터(main part 투명, indicator만 채움
 * 색)를 정확히 겹쳐서 "채워지는 막대" 느낌을 낸다(원형의 make_gauge_meter와 동일 아이디어).
 * UI_BAR_GAUGE_* 상수는 gen_gauge_face.py의 BAR_GROOVE_* 값과 반드시 일치해야 함(어긋나면
 * 라이브 막대와 baked 트랙 테두리가 안 맞아 보임).
 * Pack Voltage는 docs/design/can-signals.md의 실제 CAN 스펙 범위([0|80]V) 그대로 쓰고,
 * Sys Temp는 실제 범위(-20~235)가 너무 넓어서 UI_TEMP_GAUGE_* 표시 스케일로 축소(레이블 숫자는
 * 원본 값 그대로 찍고 막대만 clamp — anim_power_exec_cb와 동일 패턴). */
#define UI_TEMP_GAUGE_MIN (-20)
#define UI_TEMP_GAUGE_MAX 120
#define UI_BAR_GAUGE_CANVAS_W 340
#define UI_BAR_GAUGE_CANVAS_H 130
#define UI_BAR_GAUGE_GROOVE_W 300
#define UI_BAR_GAUGE_GROOVE_H 28
#define UI_BAR_GAUGE_GROOVE_CY 34  /* baked 이미지 top 기준 트랙 홈 세로 중심 */
static lv_obj_t *banner_dtc;
static lv_obj_t *lbl_dtc_code;
static lv_obj_t *bar_volt;        /* 트랙 홈 이미지 위에 겹치는 라이브 채움 막대 */
static lv_obj_t *lbl_pack_volt2;  /* page3 전용 전압 라벨 (page2의 lbl_pack_volt와 별개 위젯) */
static lv_obj_t *bar_temp;
static lv_obj_t *lbl_sys_temp;    /* sys_temp_c 바인딩 (placeholder, CAN 0x307) */

/* Page 4 — 기기 조작 및 연결성 */
static lv_obj_t *dot_ble;
static lv_obj_t *lbl_ble_status;
static lv_obj_t *dot_vehicle;

/* ---------------------------------------------------------------------
 * 공통 헬퍼
 * ------------------------------------------------------------------- */
static lv_obj_t *make_page(lv_obj_t *tv, uint8_t col_id)
{
    lv_obj_t *page = lv_tileview_add_tile(tv, col_id, 0, LV_DIR_HOR);
    lv_obj_add_style(page, &ui_style_bg, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    return page;
}

/* 빈 공간을 채우는 보조 정보 카드 — 제목(작게) 위, 값(크게) 아래, 테두리 있는 박스.
 * 데이터가 없는 항목은 value_lbl에 "--"만 채우고 ui_update()에서 갱신하지 않는다. */
static lv_obj_t *make_info_card(lv_obj_t *parent, const char *title, lv_obj_t **value_lbl_out)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, UI_CARD_W, UI_CARD_H);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_bg_color(card, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_10, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_border_opa(card, LV_OPA_40, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_title = lv_label_create(card);
    lv_obj_add_style(lbl_title, &ui_style_label_small, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_title, title);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *lbl_value = lv_label_create(card);
    lv_obj_add_style(lbl_value, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_value, "--");
    lv_obj_align(lbl_value, LV_ALIGN_BOTTOM_MID, 0, -10);

    if (value_lbl_out) *value_lbl_out = lbl_value;
    return card;
}

/* 다이얼 페이스 정적 이미지를 원본 해상도 그대로(zoom 없이) 배치 — gen_gauge_face.py가
 * 이미 실제 표시 크기 기준으로 캔버스를 구웠으므로 확대/축소가 필요 없다. */
static lv_obj_t *make_gauge_face_bg(lv_obj_t *parent, const lv_img_dsc_t *src)
{
    lv_obj_t *img = lv_img_create(parent);
    lv_img_set_src(img, src);
    return img;
}

/* 자동차 계기판 느낌의 원형 게이지 — 눈금/숫자/경고구역은 이제 make_gauge_face_bg()로 깐
 * 정적 이미지가 담당하고, 여기서는 그 위에 겹쳐 그릴 "움직이는 값 아크"만 위한 투명
 * lv_meter를 만든다(스케일의 각도/반지름 공식은 scripts/gen_gauge_face.py 상단 주석에
 * 그대로 옮겨적어 두 값이 서로 어긋나지 않게 함).
 * value_arc_enabled=false면 단색 값 아크를 만들지 않고 scale_out으로 스케일 포인터만 내보낸다
 * (호출자가 직접 그라데이션 세그먼트 등 커스텀 인디케이터를 붙일 수 있게). */
static lv_obj_t *make_gauge_meter(lv_obj_t *parent, int size, int32_t min, int32_t max,
                                   bool value_arc_enabled,
                                   lv_meter_indicator_t **value_indic_out,
                                   lv_meter_scale_t **scale_out)
{
    lv_obj_t *meter = lv_meter_create(parent);
    lv_obj_set_size(meter, size, size);
    lv_obj_set_style_border_width(meter, 0, LV_PART_MAIN);
    /* 2026-08-05: "눈금-빈 아크-실제 아크-숫자, 4겹으로 나뉘어 보인다"는 버그 리포트로
     * 발견 — sdkconfig의 CONFIG_LV_USE_THEME_DEFAULT=y가 lv_meter에 기본 "card" 스타일을
     * 입히는데(managed_components/lvgl__lvgl/src/extra/themes/default/lv_theme_default.c
     * lv_style_set_pad_all(&styles->card, PAD_DEF), DPI_DEF=130 기준 약 19px) 우리는 이
     * padding을 한 번도 0으로 안 지웠다. lv_meter.c draw_arcs()의 r_out은
     * lv_obj_get_content_coords()(테두리+패딩 뺀 영역)를 기준으로 계산되므로, 실제 라이브
     * 아크의 r_out이 gen_gauge_face.py가 가정한 값(size/2, padding 없음)보다 padding만큼
     * 작아서 — baked 트랙(바깥쪽)과 라이브 아크(그 padding만큼 안쪽)가 서로 다른 반지름에
     * 그려져 별개의 두 링처럼 보였다. padding을 0으로 지워야 baked 이미지와 좌표계가 맞는다. */
    lv_obj_set_style_pad_all(meter, 0, LV_PART_MAIN);
    lv_obj_clear_flag(meter, LV_OBJ_FLAG_CLICKABLE);

    lv_meter_scale_t *scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_range(meter, scale, min, max, 270, 135);

    if (value_arc_enabled) {
        /* lv_meter_set_indicator_value()는 start=end=v로 설정해서 길이 0인 점이 되어버리므로
         * (헤더 문서에 명시됨) 쓰지 않는다 — start는 min에 고정하고 end만 갱신해야
         * "0부터 현재값까지 채워지는" lv_arc 스타일의 게이지 느낌이 난다.
         * value_arc_enabled=true는 Power/SOC에서만 쓴다(speed는 false — 그라데이션 세그먼트를
         * 직접 붙임, build_page_drive() 참고). 2026-08-05(1차): 아크가 중앙 숫자 라벨과
         * 겹친다는 피드백으로 폭 14->16, 반지름 -34->-22로 조정. 2026-08-05(2차): "그래도
         * 좁은 느낌" 피드백으로 폭 16->20, 반지름 -22->-24로 한 번 더 키움 —
         * scripts/gen_gauge_face.py의 Power/SOC draw_gauge_face(track_r_mod=24, track_w=20)
         * 호출과 반드시 같은 값이어야 baked 트랙과 라이브 아크가 겹쳐 보인다. */
        lv_meter_indicator_t *value_indic = lv_meter_add_arc(meter, scale, 20, UI_COLOR_CYAN, -24);
        lv_meter_set_indicator_start_value(meter, value_indic, min);
        lv_meter_set_indicator_end_value(meter, value_indic, min);
        if (value_indic_out) *value_indic_out = value_indic;
    }

    if (scale_out) *scale_out = scale;
    return meter;
}

/* ---------------------------------------------------------------------
 * Page 1 — 주행 필수 정보
 * ------------------------------------------------------------------- */
static void build_page_drive(lv_obj_t *tv)
{
    lv_obj_t *page = make_page(tv, 0);
    pages[0] = page;

    /* 속도 게이지: 0~200km/h, 값 아크는 여기서 직접 그라데이션 세그먼트로 붙이므로
     * value_arc_enabled=false. */
    lv_meter_scale_t *speed_scale = NULL;
    meter_speed = make_gauge_meter(page, 280, 0, SPEED_GAUGE_MAX_KMH, false, NULL, &speed_scale);
    lv_obj_align(meter_speed, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_opa(meter_speed, LV_OPA_TRANSP, LV_PART_MAIN);

    /* 다이얼 페이스(눈금/숫자/트랙/레드존/글로우 전부 포함) 정적 이미지 — 400x400 캔버스가
     * 280px 게이지보다 넉넉해서(숫자 라벨 여백), 메타 위젯 중심에 맞춰 정렬한 뒤 배경으로
     * 내려서 라이브 값 아크가 그 위에 그려지게 함(gen_gauge_face.py 참고). */
    lv_obj_t *face_speed = make_gauge_face_bg(page, &ui_gauge_speed_face);
    lv_obj_align_to(face_speed, meter_speed, LV_ALIGN_CENTER, 0, 0);
    lv_obj_move_background(face_speed);

    /* 진한 블루(저값) -> 시안(고값)으로 이어지는 8구간 그라데이션 값 아크 */
    for (int i = 0; i < UI_SPEED_GRAD_SEGMENTS; i++) {
        speed_seg_bounds[i] = (SPEED_GAUGE_MAX_KMH * i) / UI_SPEED_GRAD_SEGMENTS;
        lv_color_t seg_color = lv_color_mix(UI_COLOR_CYAN, UI_COLOR_CYAN_DEEP,
                                             (i * 255) / (UI_SPEED_GRAD_SEGMENTS - 1));
        lv_meter_indicator_t *seg = lv_meter_add_arc(meter_speed, speed_scale, 14, seg_color, -34);
        lv_meter_set_indicator_start_value(meter_speed, seg, speed_seg_bounds[i]);
        lv_meter_set_indicator_end_value(meter_speed, seg, speed_seg_bounds[i]);
        speed_grad_segs[i] = seg;
    }
    speed_seg_bounds[UI_SPEED_GRAD_SEGMENTS] = SPEED_GAUGE_MAX_KMH;

    lbl_speed = lv_label_create(page);
    lv_obj_add_style(lbl_speed, &ui_style_label_hero, 0);
    lv_label_set_text(lbl_speed, "0");
    lv_obj_align_to(lbl_speed, meter_speed, LV_ALIGN_CENTER, 0, -12);

    lv_obj_t *unit = lv_label_create(page);
    lv_obj_add_style(unit, &ui_style_label_small, 0);
    lv_label_set_text(unit, "km/h");
    lv_obj_align_to(unit, lbl_speed, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    /* drive_mode(placeholder, CAN 0x302) -> P/R/N/D 배지. 값 자체는 실차 미대조 가정이라
     * 화면엔 뜨지만 신뢰할 수 있는 값은 아님(docs/hardware/cluster.dbc 참고). */
    /* 레퍼런스 클러스터들(Alibaba 벤치마킹, 2026-08-04)처럼 기어 배지를 크고 진하게 —
     * 56px 회색 아웃라인에서 100px 시안 강조 배지로, 글자도 히어로 폰트로 키움. */
    lv_obj_t *gear_badge = lv_obj_create(page);
    lv_obj_remove_style_all(gear_badge);
    lv_obj_set_size(gear_badge, 100, 100);
    lv_obj_set_style_radius(gear_badge, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(gear_badge, 3, 0);
    lv_obj_set_style_border_color(gear_badge, UI_COLOR_CYAN, 0);
    lv_obj_set_style_border_opa(gear_badge, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(gear_badge, UI_COLOR_CYAN, 0);
    lv_obj_set_style_bg_opa(gear_badge, LV_OPA_10, 0);
    lv_obj_clear_flag(gear_badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(gear_badge, LV_ALIGN_TOP_LEFT, 16, 16);

    lbl_gear = lv_label_create(gear_badge);
    lv_obj_add_style(lbl_gear, &ui_style_label_hero, 0);
    lv_obj_set_style_text_color(lbl_gear, UI_COLOR_CYAN, 0);
    lv_label_set_text(lbl_gear, "-");
    lv_obj_center(lbl_gear);

    /* "핵심 표시 항목" 표의 "주행 모드(Drive Mode)" — 배지 아래 캡션으로 항목명만 표시 */
    lv_obj_t *lbl_gear_caption = lv_label_create(page);
    lv_obj_add_style(lbl_gear_caption, &ui_style_label_small, 0);
    lv_label_set_text(lbl_gear_caption, "MODE");
    lv_obj_align_to(lbl_gear_caption, gear_badge, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    /* ODO/RANGE/TRIP 통합 하단바 (2026-08-04, Alibaba 벤치마킹) — 예전엔 ODO/TRIP은
     * page1 좌우 구석, RANGE는 page2에 따로 떨어져 있었는데, 레퍼런스 클러스터들처럼
     * 주행 정보를 한 줄로 묶어서 항상 같이 보이게 함. */
    lv_obj_t *bottom_row = lv_obj_create(page);
    lv_obj_remove_style_all(bottom_row);
    lv_obj_set_size(bottom_row, 800 - 32, UI_CARD_H);
    lv_obj_set_flex_flow(bottom_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(bottom_row, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_clear_flag(bottom_row, LV_OBJ_FLAG_SCROLLABLE);

    make_info_card(bottom_row, "ODO", &lbl_odo);
    make_info_card(bottom_row, "RANGE", &lbl_range);
    make_info_card(bottom_row, "TRIP", &lbl_trip_todo);
}

/* ---------------------------------------------------------------------
 * Page 2 — 전력 및 배터리
 * ------------------------------------------------------------------- */
static void build_page_battery(lv_obj_t *tv)
{
    lv_obj_t *page = make_page(tv, 1);
    pages[1] = page;

    /* 2026-07-31(3차): (1) Regen 캡션/막대가 게이지 중간 높이(lbl_power_unit) 기준으로
     * 붙어있어서 게이지 하단부(눈금/숫자)와 겹치는 문제 — meter_power "전체 박스" 하단
     * 기준으로 앵커를 바꿈. (2) 게이지 2개가 양끝+맨위에 몰려 답답해 보인다는 피드백 —
     * 크기를 줄이고(220->200) 여백을 키워서(가로 36->50, 위 8->24) 더 안정적으로 배치. */
#define UI_BATTERY_GAUGE_SIZE 200
/* 2026-08-04: 다이얼 페이스 이미지(300px)가 실제 메터(200px)보다 50px씩 더 커서(눈금
 * 숫자 여백), MARGIN_X/Y가 너무 작으면 이미지 위쪽/가장자리가 화면 밖으로 잘림 — 실기기
 * 확인 후 위로 최소 50 이상 확보 + 가운데 쪽으로 더 모으도록 여백을 키움
 * (가로 50->90, 위 24->56). */
#define UI_BATTERY_GAUGE_MARGIN_X 90
#define UI_BATTERY_GAUGE_MARGIN_Y 56

    /* Power 게이지: 0~100kW 표시 스케일(UI_POWER_GAUGE_MAX_KW, 실차 스펙 아님), 레드존 없음 */
    meter_power = make_gauge_meter(page, UI_BATTERY_GAUGE_SIZE, 0, UI_POWER_GAUGE_MAX_KW,
                                    true, &power_value_indic, NULL);
    lv_obj_align(meter_power, LV_ALIGN_TOP_LEFT, UI_BATTERY_GAUGE_MARGIN_X, UI_BATTERY_GAUGE_MARGIN_Y);
    lv_obj_set_style_bg_opa(meter_power, LV_OPA_TRANSP, LV_PART_MAIN);

    /* 다이얼 페이스(왼쪽 Power, 오른쪽 SOC) — 300x300 캔버스가 200px 게이지 기준으로
     * 구워져 있음(gen_gauge_face.py). Power는 레드존 없는 버전, SOC는 저잔량 경고구역
     * (0~UI_SOC_LOW_PCT) 포함 버전으로 서로 다른 이미지를 씀. 메터 위젯 중심에 맞춰
     * 정렬한 뒤 배경으로 내림. */
    lv_obj_t *face_power = make_gauge_face_bg(page, &ui_gauge_power_face);
    lv_obj_align_to(face_power, meter_power, LV_ALIGN_CENTER, 0, 0);
    lv_obj_move_background(face_power);

    lbl_power_kw = lv_label_create(page);
    lv_obj_add_style(lbl_power_kw, &ui_style_label_big, 0);
    lv_label_set_text(lbl_power_kw, "0");
    lv_obj_align_to(lbl_power_kw, meter_power, LV_ALIGN_CENTER, 0, -14);

    lbl_power_unit = lv_label_create(page);
    lv_obj_add_style(lbl_power_unit, &ui_style_label_small, 0);
    lv_label_set_text(lbl_power_unit, "kW");
    lv_obj_align_to(lbl_power_unit, lbl_power_kw, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    /* Regen: 텍스트 대신 배터리 충전 막대처럼 보이는 lv_bar로 표시(사용자 확인) — 캡션/막대
     * 둘 다 meter_power "게이지 전체 박스"의 아래쪽에 앵커해서, 게이지 안쪽 눈금/숫자와
     * 안 겹치게 확실히 떨어뜨린다(내부 라벨 기준으로 앵커하면 게이지 중간 높이라 겹쳤었음). */
    lbl_regen_caption = lv_label_create(page);
    lv_obj_add_style(lbl_regen_caption, &ui_style_label_small, 0);
    lv_obj_set_style_text_color(lbl_regen_caption, UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_regen_caption, "REGEN");
    lv_obj_align_to(lbl_regen_caption, meter_power, LV_ALIGN_OUT_BOTTOM_MID, -34, 16);

    lbl_regen_kw = lv_label_create(page);
    lv_obj_add_style(lbl_regen_kw, &ui_style_label_small, 0);
    lv_obj_set_style_text_color(lbl_regen_kw, UI_COLOR_GREEN, 0);
    lv_label_set_text(lbl_regen_kw, "0 kW");
    lv_obj_align_to(lbl_regen_kw, lbl_regen_caption, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    bar_regen = lv_bar_create(page);
    lv_obj_set_size(bar_regen, 160, 14);
    lv_bar_set_range(bar_regen, 0, UI_REGEN_BAR_MAX_KW);
    lv_bar_set_value(bar_regen, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(bar_regen, 7, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_regen, UI_COLOR_TEXT_SEC, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar_regen, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar_regen, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(bar_regen, UI_COLOR_TEXT_SEC, LV_PART_MAIN);
    lv_obj_set_style_border_opa(bar_regen, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_regen, 7, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar_regen, UI_COLOR_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar_regen, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_align_to(bar_regen, meter_power, LV_ALIGN_OUT_BOTTOM_MID, 0, 40);

    /* SOC 게이지: 0~100%, 0~20 레드존(저잔량 경고, UI_SOC_LOW_PCT와 동일 기준 — 다이얼 이미지에
     * 이미 구워져 있음, main/ui.c에서는 값 아크 색만 저잔량 시 RED로 전환) */
    meter_soc = make_gauge_meter(page, UI_BATTERY_GAUGE_SIZE, 0, 100, true, &soc_value_indic, NULL);
    lv_obj_align(meter_soc, LV_ALIGN_TOP_RIGHT, -UI_BATTERY_GAUGE_MARGIN_X, UI_BATTERY_GAUGE_MARGIN_Y);
    lv_obj_set_style_bg_opa(meter_soc, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_t *face_soc = make_gauge_face_bg(page, &ui_gauge_soc_face);
    lv_obj_align_to(face_soc, meter_soc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_move_background(face_soc);

    lbl_soc_pct = lv_label_create(page);
    lv_obj_add_style(lbl_soc_pct, &ui_style_label_big, 0);
    lv_label_set_text(lbl_soc_pct, "0");
    lv_obj_align_to(lbl_soc_pct, meter_soc, LV_ALIGN_CENTER, 0, -14);

    lbl_soc_unit = lv_label_create(page);
    lv_obj_add_style(lbl_soc_unit, &ui_style_label_small, 0);
    lv_label_set_text(lbl_soc_unit, "%");
    lv_obj_align_to(lbl_soc_unit, lbl_soc_pct, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

    /* Pack Voltage — Range는 2026-08-04부터 page1 ODO/RANGE/TRIP 통합 하단바로 이동해서
     * page2엔 이제 이 카드 하나만 남음, 가운데로 재배치. */
    lv_obj_t *volt_card = make_info_card(page, "Pack Voltage", &lbl_pack_volt);
    lv_label_set_text(lbl_pack_volt, "-- V");
    lv_obj_align(volt_card, LV_ALIGN_BOTTOM_MID, 0, -16);
}

/* ---------------------------------------------------------------------
 * Page 3 — 시스템 상태 및 진단
 * ------------------------------------------------------------------- */
static void build_page_diag(lv_obj_t *tv)
{
    lv_obj_t *page = make_page(tv, 2);
    pages[2] = page;

    banner_dtc = lv_obj_create(page);
    lv_obj_remove_style_all(banner_dtc);
    lv_obj_set_size(banner_dtc, 800 - 32, 64);
    lv_obj_set_style_radius(banner_dtc, 8, 0);
    lv_obj_set_style_bg_color(banner_dtc, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(banner_dtc, LV_OPA_20, 0);
    lv_obj_align(banner_dtc, LV_ALIGN_TOP_MID, 0, 16);

    lbl_dtc_code = lv_label_create(banner_dtc);
    lv_obj_add_style(lbl_dtc_code, &ui_style_label_mid, 0);
    /* dtc_code CAN ID(0x301)는 docs/design/can-signals.md 기준 실차 DBC 미대조
     * placeholder다 — UI는 raw 값만 그대로 보여준다(의미 해석 시도 안 함). */
    lv_label_set_text(lbl_dtc_code, "DTC: OK");
    lv_obj_center(lbl_dtc_code);

    /* Pack Voltage/Sys Temp 막대 게이지 2개를 세로로 쌓음 — 각 행은 [캡션/값 라벨 한 줄] +
     * [막대 트랙 홈 이미지(340x130)]. DTC 배너(top16,height64 -> bottom80) 아래부터 시작.
     * ROW_GAP은 캡션 줄(약 26px) + 여유를 감안해서 잡음. */
#define UI_DIAG_ROW1_FACE_Y 126
#define UI_DIAG_ROW_GAP 30

    lv_obj_t *face_volt = make_gauge_face_bg(page, &ui_gauge_volt_face);
    lv_obj_align(face_volt, LV_ALIGN_TOP_MID, 0, UI_DIAG_ROW1_FACE_Y);

    lv_obj_t *lbl_volt_caption = lv_label_create(page);
    lv_obj_add_style(lbl_volt_caption, &ui_style_label_small, 0);
    lv_obj_set_style_text_color(lbl_volt_caption, UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_volt_caption, "PACK VOLTAGE");
    lv_obj_align_to(lbl_volt_caption, face_volt, LV_ALIGN_OUT_TOP_LEFT, 20, -6);

    lbl_pack_volt2 = lv_label_create(page);
    lv_obj_add_style(lbl_pack_volt2, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_pack_volt2, "-- V");
    lv_obj_align_to(lbl_pack_volt2, face_volt, LV_ALIGN_OUT_TOP_RIGHT, -20, -10);

    /* 트랙 홈 이미지 위에 겹치는 라이브 채움 막대 — main part는 투명(baked 트랙이 그대로
     * 비쳐 보임), indicator만 채움 색. 오프셋(-31)은 gen_gauge_face.py의
     * BAR_GROOVE_CY(34) - BAR_CANVAS_H/2(65) 그대로(스크립트 상단 주석과 반드시 일치시킬 것). */
    bar_volt = lv_bar_create(page);
    lv_obj_set_size(bar_volt, UI_BAR_GAUGE_GROOVE_W, UI_BAR_GAUGE_GROOVE_H);
    lv_bar_set_range(bar_volt, 0, 80);
    lv_bar_set_value(bar_volt, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(bar_volt, UI_BAR_GAUGE_GROOVE_H / 2, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar_volt, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_volt, UI_BAR_GAUGE_GROOVE_H / 2, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar_volt, UI_COLOR_CYAN, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar_volt, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_align_to(bar_volt, face_volt, LV_ALIGN_CENTER, 0,
                     UI_BAR_GAUGE_GROOVE_CY - UI_BAR_GAUGE_CANVAS_H / 2);
    lv_obj_move_foreground(bar_volt);

    lv_obj_t *face_temp = make_gauge_face_bg(page, &ui_gauge_temp_face);
    lv_obj_align(face_temp, LV_ALIGN_TOP_MID, 0, UI_DIAG_ROW1_FACE_Y + UI_BAR_GAUGE_CANVAS_H + UI_DIAG_ROW_GAP);

    lv_obj_t *lbl_temp_caption = lv_label_create(page);
    lv_obj_add_style(lbl_temp_caption, &ui_style_label_small, 0);
    lv_obj_set_style_text_color(lbl_temp_caption, UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_temp_caption, "SYS TEMP");
    lv_obj_align_to(lbl_temp_caption, face_temp, LV_ALIGN_OUT_TOP_LEFT, 20, -6);

    lbl_sys_temp = lv_label_create(page);
    lv_obj_add_style(lbl_sys_temp, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_sys_temp, "--");
    lv_obj_align_to(lbl_sys_temp, face_temp, LV_ALIGN_OUT_TOP_RIGHT, -20, -10);

    /* sys_temp_c(placeholder, CAN 0x307) — 경고 시(UI_TEMP_WARN_C 이상) 채움 막대/숫자를
     * RED로 전환(예전 카드 테두리 색 전환과 같은 의도, anim_soc_exec_cb의 저잔량 색 전환과
     * 동일 패턴). 위험구간 자체는 baked 이미지의 정적 빨간 스트립으로 항상 표시된다. */
    bar_temp = lv_bar_create(page);
    lv_obj_set_size(bar_temp, UI_BAR_GAUGE_GROOVE_W, UI_BAR_GAUGE_GROOVE_H);
    lv_bar_set_range(bar_temp, UI_TEMP_GAUGE_MIN, UI_TEMP_GAUGE_MAX);
    lv_bar_set_value(bar_temp, UI_TEMP_GAUGE_MIN, LV_ANIM_OFF);
    lv_obj_set_style_radius(bar_temp, UI_BAR_GAUGE_GROOVE_H / 2, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar_temp, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_temp, UI_BAR_GAUGE_GROOVE_H / 2, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar_temp, UI_COLOR_CYAN, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar_temp, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_align_to(bar_temp, face_temp, LV_ALIGN_CENTER, 0,
                     UI_BAR_GAUGE_GROOVE_CY - UI_BAR_GAUGE_CANVAS_H / 2);
    lv_obj_move_foreground(bar_temp);
}

/* ---------------------------------------------------------------------
 * Page 4 — 기기 조작 및 연결성
 * ------------------------------------------------------------------- */
static lv_obj_t *make_status_dot(lv_obj_t *parent)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 22, 22);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    return dot;
}

static void build_page_connect(lv_obj_t *tv)
{
    lv_obj_t *page = make_page(tv, 3);
    pages[3] = page;

    dot_ble = make_status_dot(page);
    lv_obj_align(dot_ble, LV_ALIGN_TOP_LEFT, 32, 48);

    lbl_ble_status = lv_label_create(page);
    lv_obj_add_style(lbl_ble_status, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_ble_status, "BLE Disconnected");
    lv_obj_align_to(lbl_ble_status, dot_ble, LV_ALIGN_OUT_RIGHT_MID, 16, 0);

    dot_vehicle = make_status_dot(page);
    lv_obj_align_to(dot_vehicle, dot_ble, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 40);

    lv_obj_t *lbl_vehicle = lv_label_create(page);
    lv_obj_add_style(lbl_vehicle, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_vehicle, "Vehicle Status: OK");
    lv_obj_align_to(lbl_vehicle, dot_vehicle, LV_ALIGN_OUT_RIGHT_MID, 16, 0);

    /* 남는 공간에 실제 고정값(BLE 기기명, main/ble.c의 ble_svc_gap_device_name_set과 동일)을
     * 채워서 지어낸 데이터 없이도 화면을 더 채운다. */
    lv_obj_t *device_row = lv_obj_create(page);
    lv_obj_remove_style_all(device_row);
    lv_obj_set_size(device_row, 800 - 64, 100);
    lv_obj_set_flex_flow(device_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(device_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(device_row, LV_ALIGN_BOTTOM_MID, 0, -110);
    lv_obj_clear_flag(device_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *device_card = lv_obj_create(device_row);
    lv_obj_remove_style_all(device_card);
    lv_obj_set_size(device_card, 220, 90);
    lv_obj_set_style_radius(device_card, 10, 0);
    lv_obj_set_style_bg_color(device_card, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(device_card, LV_OPA_10, 0);
    lv_obj_set_style_border_width(device_card, 1, 0);
    lv_obj_set_style_border_color(device_card, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_border_opa(device_card, LV_OPA_40, 0);
    lv_obj_clear_flag(device_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_device_title = lv_label_create(device_card);
    lv_obj_add_style(lbl_device_title, &ui_style_label_small, 0);
    lv_obj_set_style_text_color(lbl_device_title, UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_device_title, "DEVICE");
    lv_obj_align(lbl_device_title, LV_ALIGN_TOP_MID, 0, 12);

    lv_obj_t *lbl_device_value = lv_label_create(device_card);
    lv_obj_add_style(lbl_device_value, &ui_style_label_small, 0);
    lv_obj_set_style_text_color(lbl_device_value, UI_COLOR_TEXT_PRI, 0);
    lv_label_set_text(lbl_device_value, "ESP32S3-Cluster");
    lv_obj_align(lbl_device_value, LV_ALIGN_BOTTOM_MID, 0, -12);

    /* HARNESS-TODO: 시계/외기온도 — RTC/온도센서 소스 미확정, 소스 확정 전 stub 유지 */
    lv_obj_t *lbl_clock;
    lv_obj_t *clock_card = make_info_card(page, "Time", &lbl_clock);
    lv_obj_align(clock_card, LV_ALIGN_BOTTOM_LEFT, 16, -16);

    lv_obj_t *lbl_outtemp;
    lv_obj_t *outtemp_card = make_info_card(page, "Out Temp", &lbl_outtemp);
    lv_obj_align(outtemp_card, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
}

/* ---------------------------------------------------------------------
 * 페이지 인디케이터 (하단 중앙 dot 4개)
 * ------------------------------------------------------------------- */
static void build_page_indicator(lv_obj_t *parent)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, 80, 12);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *dot = lv_obj_create(row);
        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, 8, 8);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, i == 0 ? UI_COLOR_CYAN : UI_COLOR_TEXT_SEC, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        page_dots[i] = dot;
    }
}

/* 페이지 전환 시 현재 타일에 대응하는 인디케이터 dot만 CYAN으로 하이라이트 */
static void tileview_event_cb(lv_event_t *e)
{
    lv_obj_t *tv = lv_event_get_target(e);
    lv_obj_t *act = lv_tileview_get_tile_act(tv);

    for (int i = 0; i < 4; i++) {
        lv_obj_set_style_bg_color(page_dots[i],
            pages[i] == act ? UI_COLOR_CYAN : UI_COLOR_TEXT_SEC, 0);
    }
}

/* ---------------------------------------------------------------------
 * 탭 기반 페이지 전환 (2026-07-31, 드래그 스와이프 대체)
 * 800x480 화면을 좌/우 반반으로 나눈 투명 클릭존을 tileview 위에 얹어서, 사용자
 * 입력이 tileview의 드래그 스크롤 로직까지 아예 안 내려가게 막는다(오브젝트 트리상
 * 나중에 추가된 쪽이 위에서 터치를 가로챔) — tileview 자체의 SCROLLABLE은 그대로
 * 둬서 lv_obj_set_tile_id()의 내부 스크롤(및 그걸 따라오는 dot 갱신용
 * LV_EVENT_VALUE_CHANGED)은 그대로 동작한다.
 * 2026-07-31(2차): 슬라이드 애니메이션도 필요 없다는 피드백으로 LV_ANIM_ON->OFF로
 * 변경 — 탭하면 바로 전환됨. LV_ANIM_OFF도 내부적으로 LV_EVENT_SCROLL_END를 동기적으로
 * 보내므로(lv_obj_scroll_by, lv_obj_scroll.c) dot 갱신 로직은 애니메이션 유무와 무관하게
 * 그대로 작동한다(직접 소스 확인함). */
static void go_to_page(int delta)
{
    lv_obj_t *act = lv_tileview_get_tile_act(tileview);
    int cur = 0;
    for (int i = 0; i < 4; i++) {
        if (pages[i] == act) {
            cur = i;
            break;
        }
    }

    int target = cur + delta;
    if (target < 0) target = 0;
    if (target > 3) target = 3;

    if (target != cur) {
        lv_obj_set_tile_id(tileview, target, 0, LV_ANIM_OFF);
    }
}

static void tap_zone_event_cb(lv_event_t *e)
{
    int delta = (int)(intptr_t)lv_event_get_user_data(e);
    go_to_page(delta);
}

static void build_tap_zones(lv_obj_t *parent)
{
    lv_obj_t *tap_left = lv_obj_create(parent);
    lv_obj_remove_style_all(tap_left);
    lv_obj_set_size(tap_left, lv_pct(50), lv_pct(100));
    lv_obj_align(tap_left, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(tap_left, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(tap_left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(tap_left, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tap_left, tap_zone_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)-1);

    lv_obj_t *tap_right = lv_obj_create(parent);
    lv_obj_remove_style_all(tap_right);
    lv_obj_set_size(tap_right, lv_pct(50), lv_pct(100));
    lv_obj_align(tap_right, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(tap_right, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(tap_right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(tap_right, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tap_right, tap_zone_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)1);
}

/* ---------------------------------------------------------------------
 * public API
 * ------------------------------------------------------------------- */
void ui_init(lv_obj_t *parent)
{
    ui_style_init();

    lv_obj_add_style(parent, &ui_style_bg, 0);

    tileview = lv_tileview_create(parent);
    lv_obj_set_size(tileview, lv_pct(100), lv_pct(100));
    lv_obj_add_style(tileview, &ui_style_bg, 0);

    build_page_drive(tileview);
    build_page_battery(tileview);
    build_page_diag(tileview);
    build_page_connect(tileview);

    build_page_indicator(parent);
    build_tap_zones(parent);

    lv_obj_add_event_cb(tileview, tileview_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/* speed/soc 게이지+숫자를 한 애니메이션으로 같이 움직여서(exec_cb 안에서 둘 다 갱신)
 * 실차 클러스터처럼 값이 순간 이동하지 않고 부드럽게 스윕되도록 한다.
 * 2026-08-05: "게이지/숫자 변화 반응성을 높여달라"는 피드백으로 두 가지 변경.
 * (1) 250ms->150ms — CAN 시뮬레이터가 20ms마다 값을 갱신하는데(scripts/can_sim_kvaser.py
 *     TICK_S) 250ms는 체감상 느리게 따라오는 느낌이 남.
 * (2) animate_X_to()가 매번 "current = target"으로 시작점을 앞당겨 잡던 버그를 고침 —
 *     lv_anim_start()는 같은 var+exec_cb 애니메이션을 자동으로 lv_anim_del()하고 새로
 *     시작하므로(managed_components/lvgl__lvgl/src/misc/lv_anim.c 확인), 20ms 틱마다 값이
 *     바뀌면 이전 애니메이션은 250ms를 다 못 채우고 계속 중간에 잘렸다. 그런데 "current"를
 *     실제 렌더된 값이 아니라 "마지막으로 명령한 target"으로 앞당겨 놨기 때문에, 다음
 *     애니메이션이 시작하는 순간 화면이 (중간에 있던 실제 값) -> (이전 target)으로 순간
 *     점프했다가 다시 새 target으로 움직이는 "끊김"이 반복해서 생겼음(20ms 주기로 계속
 *     재발하니 체감상 뚝뚝 끊기는 반응성으로 느껴짐). 각 exec_cb가 매 프레임 실제 렌더된
 *     값을 *_current_v에 기록해두고, 다음 animate_X_to()는 거기서부터 이어서 움직이게
 *     고쳐서 애니메이션이 중간에 끊겨도 시각적으로 연속되게 함. */
#define UI_GAUGE_ANIM_TIME_MS 150

static int32_t speed_current_v = 0;

static void anim_speed_exec_cb(void *var, int32_t v)
{
    (void)var;
    speed_current_v = v;
    /* 세그먼트별로 [start,end] 구간 중 v로 덮이는 만큼만 채운다 — v 이전 구간은 완전히
     * 채워지고, v가 속한 구간은 부분적으로, 이후 구간은 비어있는(start=end) 채로 남는다. */
    for (int i = 0; i < UI_SPEED_GRAD_SEGMENTS; i++) {
        int32_t seg_start = speed_seg_bounds[i];
        int32_t seg_end = speed_seg_bounds[i + 1];
        int32_t filled_end = v > seg_end ? seg_end : (v < seg_start ? seg_start : v);
        lv_meter_set_indicator_end_value(meter_speed, speed_grad_segs[i], filled_end);
    }
    char buf[16];
    /* speed는 후진 시 음수(cluster.dbc offset -10)라 %u로 찍으면 거대한 양수로 깨짐.
     * 게이지(위 세그먼트 루프)는 v<0이면 자동으로 전부 0으로 clamp되지만, 후진 방향을
     * 시각적으로 표현하는 건 아직 안 함(라벨 숫자만 정확히 음수로 표시) — HARNESS-TODO */
    snprintf(buf, sizeof(buf), "%d", (int)v);
    lv_label_set_text(lbl_speed, buf);
    /* 자릿수가 바뀌면 라벨 폭도 바뀌는데, align_to는 최초 호출 시점 폭 기준으로 한 번만
     * 위치를 잡아서 이후 폭이 늘어난 만큼 오른쪽으로 밀려 보인다 — 매번 다시 정렬해야 함. */
    lv_obj_align_to(lbl_speed, meter_speed, LV_ALIGN_CENTER, 0, -12);
}

static void animate_speed_to(int32_t target)
{
    static int32_t last_target = -1;
    if (target == last_target) return;
    last_target = target;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, meter_speed);
    lv_anim_set_exec_cb(&a, anim_speed_exec_cb);
    lv_anim_set_values(&a, speed_current_v, target);
    lv_anim_set_time(&a, UI_GAUGE_ANIM_TIME_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static int32_t soc_current_v = 0;

static void anim_soc_exec_cb(void *var, int32_t v)
{
    (void)var;
    soc_current_v = v;
    lv_meter_set_indicator_end_value(meter_soc, soc_value_indic, v);
    /* lv_meter 인디케이터는 색을 바꾸는 공개 setter가 없어, 노출된(비-opaque) 구조체
     * 필드를 직접 갱신하고 무효화해서 다시 그리게 한다(공식 헤더에 문서화된 필드). */
    soc_value_indic->type_data.arc.color = (v <= UI_SOC_LOW_PCT) ? UI_COLOR_RED : UI_COLOR_CYAN;
    lv_obj_invalidate(meter_soc);
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", (unsigned)v);
    lv_label_set_text(lbl_soc_pct, buf);
    /* speed와 동일한 이유로 매번 재정렬 필요 (자릿수 변화로 폭이 바뀜) */
    lv_obj_align_to(lbl_soc_pct, meter_soc, LV_ALIGN_CENTER, 0, -14);
}

static void animate_soc_to(int32_t target)
{
    static int32_t last_target = -1;
    if (target == last_target) return;
    last_target = target;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, meter_soc);
    lv_anim_set_exec_cb(&a, anim_soc_exec_cb);
    lv_anim_set_values(&a, soc_current_v, target);
    lv_anim_set_time(&a, UI_GAUGE_ANIM_TIME_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static int32_t power_current_v = 0;

static void anim_power_exec_cb(void *var, int32_t v)
{
    (void)var;
    power_current_v = v;
    /* power_kw 실제 범위는 [0|255]지만 게이지 표시 스케일은 UI_POWER_GAUGE_MAX_KW(100) —
     * speed 게이지와 같은 패턴으로, 라벨 숫자는 실제값 그대로 찍되 게이지 아크만 clamp */
    int32_t gauge_v = v > UI_POWER_GAUGE_MAX_KW ? UI_POWER_GAUGE_MAX_KW : (v < 0 ? 0 : v);
    lv_meter_set_indicator_end_value(meter_power, power_value_indic, gauge_v);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", (int)v);
    lv_label_set_text(lbl_power_kw, buf);
    lv_obj_align_to(lbl_power_kw, meter_power, LV_ALIGN_CENTER, 0, -14);
}

static void animate_power_to(int32_t target)
{
    static int32_t last_target = -1;
    if (target == last_target) return;
    last_target = target;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, meter_power);
    lv_anim_set_exec_cb(&a, anim_power_exec_cb);
    lv_anim_set_values(&a, power_current_v, target);
    lv_anim_set_time(&a, UI_GAUGE_ANIM_TIME_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static int32_t volt_current_v = 0;

static void anim_volt_exec_cb(void *var, int32_t v)
{
    (void)var;
    volt_current_v = v;
    lv_bar_set_value(bar_volt, v, LV_ANIM_OFF);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d V", (int)v);
    lv_label_set_text(lbl_pack_volt2, buf);
}

static void animate_volt_to(int32_t target)
{
    static int32_t last_target = -1;
    if (target == last_target) return;
    last_target = target;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, bar_volt);
    lv_anim_set_exec_cb(&a, anim_volt_exec_cb);
    lv_anim_set_values(&a, volt_current_v, target);
    lv_anim_set_time(&a, UI_GAUGE_ANIM_TIME_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static int32_t temp_current_v = 0;

static void anim_temp_exec_cb(void *var, int32_t v)
{
    (void)var;
    temp_current_v = v;
    /* sys_temp_c 실제 범위(-20~235)가 UI_TEMP_GAUGE_MAX(120)를 넘을 수 있어 power 게이지와
     * 동일하게 라벨은 원본 값 그대로, 막대만 clamp. */
    int32_t bar_v = v > UI_TEMP_GAUGE_MAX ? UI_TEMP_GAUGE_MAX : (v < UI_TEMP_GAUGE_MIN ? UI_TEMP_GAUGE_MIN : v);
    lv_bar_set_value(bar_temp, bar_v, LV_ANIM_OFF);
    bool temp_warn = v >= UI_TEMP_WARN_C;
    lv_obj_set_style_bg_color(bar_temp, temp_warn ? UI_COLOR_RED : UI_COLOR_CYAN, LV_PART_INDICATOR);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d C", (int)v);
    lv_label_set_text(lbl_sys_temp, buf);
    lv_obj_set_style_text_color(lbl_sys_temp, temp_warn ? UI_COLOR_RED : UI_COLOR_TEXT_PRI, 0);
}

static void animate_temp_to(int32_t target)
{
    static int32_t last_target = -1;
    if (target == last_target) return;
    last_target = target;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, bar_temp);
    lv_anim_set_exec_cb(&a, anim_temp_exec_cb);
    lv_anim_set_values(&a, temp_current_v, target);
    lv_anim_set_time(&a, UI_GAUGE_ANIM_TIME_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

void ui_update(void)
{
    VehicleData_t d;
    vehicle_data_get(&d);

    char buf[32];

    /* Page 1 */
    animate_speed_to(d.speed);

    {
        static const char *gear_text[4] = { "P", "R", "N", "D" };
        lv_label_set_text(lbl_gear, d.drive_mode < 4 ? gear_text[d.drive_mode] : "--");
    }
    snprintf(buf, sizeof(buf), "%lu km", (unsigned long)d.odo_km);
    lv_label_set_text(lbl_odo, buf);

    /* Page 2 */
    animate_soc_to(d.soc);

    snprintf(buf, sizeof(buf), "%.1f V", d.pack_volt);
    lv_label_set_text(lbl_pack_volt, buf);

    snprintf(buf, sizeof(buf), "%u km", (unsigned)d.range_km);
    lv_label_set_text(lbl_range, buf);

    animate_power_to((int32_t)d.power_kw);
    lv_bar_set_value(bar_regen, (int32_t)d.regen_kw, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%d kW", (int)d.regen_kw);
    lv_label_set_text(lbl_regen_kw, buf);
    /* speed/soc와 같은 이유로 자릿수 변화 시 재정렬 필요 (align_to는 최초 호출 시점 폭 기준) —
     * build_page_battery()에서 잡은 것과 동일한 기준(lbl_regen_caption 오른쪽)으로 다시 정렬 */
    lv_obj_align_to(lbl_regen_kw, lbl_regen_caption, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    /* Page 3 */
    if (d.dtc_code == 0) {
        lv_label_set_text(lbl_dtc_code, "DTC: OK");
        lv_obj_set_style_bg_color(banner_dtc, UI_COLOR_TEXT_SEC, 0);
        lv_obj_set_style_bg_opa(banner_dtc, LV_OPA_20, 0);
    } else {
        snprintf(buf, sizeof(buf), "DTC: 0x%02X", (unsigned)d.dtc_code);
        lv_label_set_text(lbl_dtc_code, buf);
        lv_obj_set_style_bg_color(banner_dtc, UI_COLOR_RED, 0);
        lv_obj_set_style_bg_opa(banner_dtc, LV_OPA_50, 0);
    }

    animate_volt_to((int32_t)(d.pack_volt + 0.5f));
    animate_temp_to((int32_t)d.sys_temp_c);

    /* Page 4 */
    lv_obj_set_style_bg_color(dot_ble, d.ble_connected ? UI_COLOR_GREEN : UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_ble_status, d.ble_connected ? "BLE Connected" : "BLE Disconnected");

    lv_obj_set_style_bg_color(dot_vehicle, d.dtc_code == 0 ? UI_COLOR_GREEN : UI_COLOR_RED, 0);
}
