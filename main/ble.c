#include "ble.h"
#include "vehicle_data.h"
#include "esp_log.h"
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

/* ---- 외부(ble.h)에 노출되는 전역 상태 ---- */
uint16_t g_ble_conn_handle = BLE_HS_CONN_HANDLE_NONE;
uint16_t g_vehicle_chr_val_handle = 0;

/* ---- 이 프로젝트 전용 커스텀 UUID ----
 * 임의로 생성한 128bit UUID입니다. 실제 배포 전에는
 * uuidgenerator.net 등에서 새로 생성해서 교체하세요. */
static const ble_uuid128_t vehicle_svc_uuid =
    BLE_UUID128_INIT(0x2f, 0x1a, 0x9e, 0x40, 0x8b, 0x2c, 0x4d, 0x91,
                      0xa5, 0x3e, 0x00, 0x11, 0x00, 0xff, 0x00, 0x01);

static const ble_uuid128_t vehicle_chr_uuid =
    BLE_UUID128_INIT(0x2f, 0x1a, 0x9e, 0x40, 0x8b, 0x2c, 0x4d, 0x91,
                      0xa5, 0x3e, 0x00, 0x11, 0x00, 0xff, 0x00, 0x02);

/* 전방 선언: ble_app_advertise()에서 정의보다 먼저 참조되므로 필요 */
int ble_gap_event(struct ble_gap_event *event, void *arg);

/* ---- GATT characteristic access 콜백 (스마트폰이 Read 요청 시) ---- */
static int vehicle_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    VehicleData_t data;
    vehicle_data_get(&data);

    VehicleBlePacket_t pkt = {
        .speed = data.speed,
        .soc = data.soc,
        .pack_volt = data.pack_volt,
        .dtc_code = data.dtc_code,
    };

    int rc = os_mbuf_append(ctxt->om, &pkt, sizeof(pkt));
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
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
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
            VehicleBlePacket_t pkt = {
                .speed = data.speed,
                .soc = data.soc,
                .pack_volt = data.pack_volt,
                .dtc_code = data.dtc_code,
            };

            struct os_mbuf *om = ble_hs_mbuf_from_flat(&pkt, sizeof(pkt));
            if (om != NULL) {
                int rc = ble_gatts_notify_custom(g_ble_conn_handle,
                                                  g_vehicle_chr_val_handle, om);
                if (rc != 0) {
                    ESP_LOGW(TAG, "notify 실패: rc=%d", rc);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}