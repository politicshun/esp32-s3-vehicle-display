// ---- TEMPORARY: CAN 버스 없이 UI 반응성만 확인하기 위한 더미 vehicle_data 생성기 ----
// main/twai.c의 CAN 자체 루프백 시도가 종단저항 없는 버스에서 실패(twai_transmit
// 타임아웃 + arb_lost/bus_error 폭증)해서, CAN 계층을 아예 우회하고 vehicle_data_set()을
// 직접 호출하는 방식으로 전환함.
// 실차 CAN 버스에 연결할 때는 main.c에서 dummy_data_sim_task 생성 줄을 지울 것.
#include "vehicle_data.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void dummy_data_sim_task(void *pvParameters) {
    int speed = 0, speed_dir = 1;
    int soc = 100, soc_dir = -1;
    float pack_volt = 300.0f, volt_dir = 1.0f;
    int dtc_tick = 0;

    while (1) {
        VehicleData_t d;
        vehicle_data_get(&d);

        d.speed = (uint16_t)speed;
        speed += speed_dir * 3;
        if (speed >= 180 || speed <= 0) speed_dir = -speed_dir;

        d.soc = (uint8_t)soc;
        soc += soc_dir;
        if (soc >= 100 || soc <= 0) soc_dir = -soc_dir;

        d.pack_volt = pack_volt;
        pack_volt += volt_dir;
        if (pack_volt >= 420.0f || pack_volt <= 300.0f) volt_dir = -volt_dir;

        dtc_tick++;
        d.dtc_code = (dtc_tick / 25) % 2 == 0 ? 0x0000 : 0x1234; // 5초(25*200ms)마다 토글

        vehicle_data_set(&d);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
