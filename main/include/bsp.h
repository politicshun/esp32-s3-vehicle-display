#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdint.h>

// bsp_hardware_init()에서 생성되는 I2C 마스터 버스 핸들.
// CH422G와 GT911 터치가 이 버스를 공유하므로 lvgl.c의 touch_init()에서도 사용.
extern i2c_master_bus_handle_t g_i2c_bus_handle;

esp_err_t ch422g_write_exio(uint8_t bit_mask);
esp_err_t ch422g_clear_exio(uint8_t bit_mask);
void bsp_hardware_init(void);