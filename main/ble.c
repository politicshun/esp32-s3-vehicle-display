#include "ble.h"
#include "vehicle_data.h"
#include "esp_log.h"
#include <stdio.h>  /* snprintf — 텍스트 characteristic (build_vehicle_text) */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "BLE";

/* 2026-08-03: 실기기 hex 확인 중 17바이트가 아니라 18바이트로 보인다는 제보가 있어 컴파일
 * 타임에 검증 — 통과함(펌웨어 struct는 정확히 17바이트, __attribute__((packed))가 의도대로
 * 동작). 즉 그 여분 바이트는 펌웨어가 아니라 폰/스캐너 앱 쪽 표시 문제. 앞으로 필드를
 * 추가/변경할 때 docs/design/ble-gatt.md의 크기 표기를 깜빡하고 안 고치는 걸 막기 위해
 * 영구 가드로 남겨둔다 — 값이 바뀌면 이 줄도 같이 고치고 문서도 갱신할 것. */
_Static_assert(sizeof(VehicleBlePacket_t) == 17, "VehicleBlePacket_t 크기가 17바이트가 아님 - docs/design/ble-gatt.md도 같이 갱신할 것");

/* ---- 외부(ble.h)에 노출되는 전역 상태 ---- */
uint16_t g_ble_conn_handle = BLE_HS_CONN_HANDLE_NONE;
uint16_t g_vehicle_chr_val_handle = 0;
uint16_t g_vehicle_text_chr_val_handle = 0;

/* ---- 이 프로젝트 전용 커스텀 UUID ----
 * 2026-08-03 양산용으로 재발급(OS 난수 생성, `[guid]::NewGuid()`) —
 * 이전 placeholder(0x2f,0x1a,0x9e,...)는 폐기.
 * Service UUID: d8be14dc-6df4-4aae-9750-65c274746c87
 * Char    UUID: 95896bdd-d3b1-4f4b-b7b7-9dd2876e5c7c
 * (BLE_UUID128_INIT의 바이트 순서는 문자열 표기의 역순 — 참고: ble_uuid_to_str()
 *  구현이 value[15..0] 순으로 출력함, esp-idf/.../host/ble_uuid.c)
 * 앱 팀에는 위 하이픈 표기 UUID 문자열 그대로 전달하면 됨(docs/design/ble-gatt.md 참고). */
static const ble_uuid128_t vehicle_svc_uuid =
    BLE_UUID128_INIT(0x87, 0x6c, 0x74, 0x74, 0xc2, 0x65, 0x50, 0x97,
                      0xae, 0x4a, 0xf4, 0x6d, 0xdc, 0x14, 0xbe, 0xd8);

static const ble_uuid128_t vehicle_chr_uuid =
    BLE_UUID128_INIT(0x7c, 0x5c, 0x6e, 0x87, 0xd2, 0x9d, 0xb7, 0xb7,
                      0x4b, 0x4f, 0xb1, 0xd3, 0xdd, 0x6b, 0x89, 0x95);

/* 2026-08-06: 보고/데모용 텍스트 characteristic UUID (OS 난수 생성, `[guid]::NewGuid()`,
 * 위 vehicle_chr_uuid와 동일한 방식). 같은 서비스(vehicle_svc_uuid) 아래에 추가되는
 * 두 번째 characteristic이라 앱 팀의 기존 packed 바이너리 characteristic
 * (vehicle_chr_uuid)에는 영향 없음.
 * Char UUID: ef143c25-4b04-4596-942d-1f10b5d3a1cb (docs/design/ble-gatt.md 참고) */
static const ble_uuid128_t vehicle_text_chr_uuid =
    BLE_UUID128_INIT(0xcb, 0xa1, 0xd3, 0xb5, 0x10, 0x1f, 0x2d, 0x94,
                      0x96, 0x45, 0x04, 0x4b, 0x25, 0x3c, 0x14, 0xef);

/* 전방 선언: ble_app_advertise()에서 정의보다 먼저 참조되므로 필요 */
int ble_gap_event(struct ble_gap_event *event, void *arg);

/* ---- VehicleData_t -> BLE 전송용 packed 바이너리(VehicleBlePacket_t, ble.h 참고) ---- */
static void build_vehicle_packet(VehicleBlePacket_t *pkt, const VehicleData_t *d) {
    pkt->proto_version = BLE_PROTOCOL_VERSION;
    pkt->speed = d->speed;
    pkt->soc = d->soc;
    pkt->pack_volt = (uint8_t)d->pack_volt;
    pkt->dtc_code = d->dtc_code;
    pkt->drive_mode = d->drive_mode;
    pkt->odo_km = d->odo_km;
    pkt->range_km = d->range_km;
    pkt->power_kw = (uint8_t)d->power_kw;
    pkt->regen_kw = (uint8_t)d->regen_kw;
    pkt->sys_temp_c = d->sys_temp_c;
}

/* ---- GATT characteristic access 콜백 (스마트폰이 Read 요청 시) ---- */
static int vehicle_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    VehicleData_t data;
    vehicle_data_get(&data);

    VehicleBlePacket_t pkt;
    build_vehicle_packet(&pkt, &data);

    int rc = os_mbuf_append(ctxt->om, &pkt, sizeof(pkt));
    return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

/* 2026-08-06: 보고/데모용 — nRF Connect 등 범용 스캐너 앱이 별도 파서 없이 바로
 * 읽을 수 있는 UTF-8 텍스트로 VehicleData_t 전체(10개 필드)를 사람이 읽기 쉬운
 * 형태로 직렬화한다. 2026-07-31에 썼다가 폐기했던 버전(SPD/SOC/VOLT/DTC 4개뿐)과
 * 달리 drive_mode/odo_km/range_km/power_kw/regen_kw/sys_temp_c까지 전부 포함.
 * drive_mode는 앱 팀 바이너리 스펙처럼 raw 숫자(0~3)가 아니라 P/R/N/D 글자로 —
 * "실제 값으로 구분되어 보이게" 하는 게 목적이라 숫자보다 글자가 사람이 읽기 낫다.
 * pack_volt/power_kw/regen_kw는 VehicleData_t가 float지만 실제로는 정수 해상도라
 * (docs/hardware/cluster.dbc factor1, main/include/ble.h 기존 주석 참고) 소수점 없이 출력. */
static const char *drive_mode_letter(uint8_t mode) {
    switch (mode) {
        case 0: return "P";
        case 1: return "R";
        case 2: return "N";
        case 3: return "D";
        default: return "?";
    }
}

static int build_vehicle_text(char *buf, size_t buf_size, const VehicleData_t *d) {
    return snprintf(buf, buf_size,
        "SPD:%dkm/h SOC:%u%% VOLT:%.0fV DTC:%u MODE:%s "
        "ODO:%lukm RANGE:%ukm PWR:%.0fkW REGEN:%.0fkW TEMP:%dC",
        d->speed, d->soc, d->pack_volt, d->dtc_code, drive_mode_letter(d->drive_mode),
        (unsigned long)d->odo_km, d->range_km, d->power_kw, d->regen_kw, d->sys_temp_c);
}

/* ---- GATT characteristic access 콜백 (텍스트 characteristic, 스마트폰이 Read 요청 시) ---- */
static int vehicle_text_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                       struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    VehicleData_t data;
    vehicle_data_get(&data);

    char text[128];
    int len = build_vehicle_text(text, sizeof(text), &data);
    if (len < 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    int rc = os_mbuf_append(ctxt->om, text, (uint16_t)len);
    return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

/* ---- GATT 서비스 테이블 ---- */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &vehicle_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = &vehicle_chr_uuid.u,
                .access_cb = vehicle_chr_access_cb,
                .val_handle = &g_vehicle_chr_val_handle,
                /* 2026-08-03: Just Works bonding 적용(사용자 확인) — Read/Notify 모두 암호화된
                 * 링크에서만 허용(_ENC 플래그). MITM 인증까지는 요구하지 않음(Just Works 특성). */
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY |
                         BLE_GATT_CHR_F_READ_ENC | BLE_GATT_CHR_F_NOTIFY_INDICATE_ENC,
            },
            {
                /* 2026-08-06: 보고/데모용 텍스트 characteristic — 같은 서비스 아래
                 * 두 번째 characteristic으로 추가, 위 바이너리 characteristic과
                 * 보안 정책(Just Works, 암호화 필수)은 동일하게 맞춤. */
                .uuid = &vehicle_text_chr_uuid.u,
                .access_cb = vehicle_text_chr_access_cb,
                .val_handle = &g_vehicle_text_chr_val_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY |
                         BLE_GATT_CHR_F_READ_ENC | BLE_GATT_CHR_F_NOTIFY_INDICATE_ENC,
            },
            {0} /* 배열 종료 마커 */
        },
    },
    {0} /* 배열 종료 마커 */
};

/* ---- 연결 상태를 vehicle_data.ble_connected에 반영 ---- */
static void set_ble_connected_flag(bool connected) {
    VehicleData_t data;
    vehicle_data_get(&data);
    data.ble_connected = connected;
    vehicle_data_set(&data);
}

/* ---- Advertising 시작 ---- */
static void ble_app_advertise(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name = ble_svc_gap_device_name();

    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_set_fields 실패: rc=%d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; /* 누구나 연결 가능 */
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                            &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_start 실패: rc=%d", rc);
    } else {
        ESP_LOGI(TAG, "Advertising 시작됨 (기기명: %s)", name);
    }
}

/* ---- GAP 이벤트 핸들러: 연결/해제/구독 처리 ---- */
int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            g_ble_conn_handle = event->connect.conn_handle;
            set_ble_connected_flag(true);
            ESP_LOGI(TAG, "스마트폰 연결됨 (handle=%d)", g_ble_conn_handle);
        } else {
            ESP_LOGW(TAG, "연결 실패: status=%d", event->connect.status);
            ble_app_advertise(); /* 실패 시 재광고 */
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "연결 해제됨 (reason=%d)", event->disconnect.reason);
        g_ble_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        set_ble_connected_flag(false);
        ble_app_advertise(); /* 다시 광고 시작 */
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        /* 스마트폰이 notify를 구독(CCCD 활성화)했을 때 */
        ESP_LOGI(TAG, "Notify 구독 상태 변경: cur_notify=%d",
                 event->subscribe.cur_notify);
        return 0;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU 협상됨: %d bytes", event->mtu.value);
        return 0;

    default:
        return 0;
    }
}

/* ---- 호스트 sync/reset 콜백 ---- */
static void on_sync(void) {
    uint8_t own_addr_type;
    int rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_hs_id_infer_auto 실패: rc=%d", rc);
        return;
    }
    ble_app_advertise();
}

static void on_reset(int reason) {
    ESP_LOGE(TAG, "BLE 호스트 리셋됨, reason=%d", reason);
}

/* ---- NimBLE host task (별도 스레드에서 이벤트 루프 실행) ---- */
static void ble_host_task(void *param) {
    nimble_port_run(); /* 이 함수는 스택이 종료될 때까지 반환하지 않음 */
    nimble_port_freertos_deinit();
}

void ble_init(void) {
    esp_err_t err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init 실패: %d", err);
        return;
    }

    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;

    /* 2026-08-03: Just Works bonding 적용(사용자 확인) — PIN 입력 없이 자동 암호화+본딩,
     * 재연결 시 재페어링 불필요. MITM 보호는 없음(Just Works 특성상 중간자 공격에는 원천적으로
     * 취약 — sm_io_cap이 입출력 수단 없음이라 다른 방식(Passkey/Numeric Comparison) 자체가
     * 불가능함). 차량 클러스터-폰 컴패니언 앱 유스케이스에서 흔히 쓰이는 수준의 보안. */
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT; /* Just Works 강제 */
    ble_hs_cfg.sm_bonding = 1;   /* 페어링 키 저장 -> 재연결 시 재페어링 불필요 */
    ble_hs_cfg.sm_mitm = 0;      /* Just Works: MITM 보호 없음 */
    ble_hs_cfg.sm_sc = 1;        /* LE Secure Connections 우선(레거시보다 안전), 미지원 상대는 legacy 폴백 */
    ble_hs_cfg.sm_sc_only = 0;
    ble_hs_cfg.sm_sec_lvl = 2;   /* Unauthenticated pairing with encryption (Just Works에 해당하는 레벨) */
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

    /* 2026-07-31: Notify는 협상된 ATT MTU를 넘는 바이트를 자르고 마는데(Read처럼 blob으로
     * 이어받는 재조립이 없음) — 텍스트 payload 시절 기본 MTU(23바이트=페이로드 20바이트)에서
     * 잘림이 실기기에서 확인된 적이 있음. 2026-08-03 바이너리 전환 후 VehicleBlePacket_t가
     * 17바이트라 기본 MTU로도 들어가지만, 향후 필드 추가 여지를 위해 선호 MTU는 그대로 둔다
     * (연결 전에 호출 필요). */
    ble_att_set_preferred_mtu(247);

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set("ESP32S3-Cluster");

    int rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "gatts_count_cfg 실패: rc=%d", rc);
        return;
    }
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "gatts_add_svcs 실패: rc=%d", rc);
        return;
    }

    nimble_port_freertos_init(ble_host_task);
    ESP_LOGI(TAG, "BLE 스택 초기화 완료");
}

/* ---- 기존 ble_sync_task: 실제 notify 전송 구현 ---- */
void ble_sync_task(void *pvParameters) {
    ESP_LOGI(TAG, "BLE Sync Task Started");

    VehicleData_t data;

    while (1) {
        vehicle_data_get(&data);

        if (data.ble_connected && g_ble_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            VehicleBlePacket_t pkt;
            build_vehicle_packet(&pkt, &data);

            struct os_mbuf *om = ble_hs_mbuf_from_flat(&pkt, sizeof(pkt));
            if (om != NULL) {
                int rc = ble_gatts_notify_custom(g_ble_conn_handle,
                                                  g_vehicle_chr_val_handle, om);
                if (rc != 0) {
                    ESP_LOGW(TAG, "notify 실패: rc=%d", rc);
                }
            }

            /* 2026-08-06: 보고/데모용 텍스트 characteristic도 같은 주기로 notify —
             * nRF Connect 등에서 구독하면 화면에서 값이 실시간으로 갱신되는 걸 바로 볼 수 있음. */
            char text[128];
            int text_len = build_vehicle_text(text, sizeof(text), &data);
            if (text_len > 0) {
                struct os_mbuf *text_om = ble_hs_mbuf_from_flat(text, (uint16_t)text_len);
                if (text_om != NULL) {
                    int rc = ble_gatts_notify_custom(g_ble_conn_handle,
                                                      g_vehicle_text_chr_val_handle, text_om);
                    if (rc != 0) {
                        ESP_LOGW(TAG, "텍스트 notify 실패: rc=%d", rc);
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}