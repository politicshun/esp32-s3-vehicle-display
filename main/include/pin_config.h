#pragma once

#include "driver/gpio.h"

// TWAI (CAN) Pin
#define CAN_TX_GPIO_NUM         GPIO_NUM_20
#define CAN_RX_GPIO_NUM         GPIO_NUM_19

// Touch / I2C Bus Pin
#define I2C_MASTER_SDA_IO       GPIO_NUM_8
#define I2C_MASTER_SCL_IO       GPIO_NUM_9
#ifndef I2C_MASTER_FREQ_HZ
#define I2C_MASTER_FREQ_HZ      400000
#endif

// Touch / IRQ Pin
#define CTP_IRQ_GPIO_NUM        GPIO_NUM_4

// CH422G IO Expander Register
#define CH422G_EXIO_CTP_RST     (1 << 1)
#define CH422G_EXIO_LCD_DISP    (1 << 2)
#define CH422G_EXIO_LCD_RST     (1 << 3)
#define CH422G_EXIO_SD_CS       (1 << 4)
#define CH422G_EXIO_USB_SEL     (1 << 5)
#define CH422G_EXIO_LCD_VDD_EN  (1 << 6)

#define LCD_HSYNC_GPIO GPIO_NUM_46
#define LCD_VSYNC_GPIO GPIO_NUM_3
#define LCD_DE_GPIO     GPIO_NUM_5
#define LCD_PCLK_GPIO   GPIO_NUM_7
 
#define LCD_B3_GPIO GPIO_NUM_14
#define LCD_B4_GPIO GPIO_NUM_38
#define LCD_B5_GPIO GPIO_NUM_18
#define LCD_B6_GPIO GPIO_NUM_17
#define LCD_B7_GPIO GPIO_NUM_10
 
#define LCD_G2_GPIO GPIO_NUM_39
#define LCD_G3_GPIO GPIO_NUM_0
#define LCD_G4_GPIO GPIO_NUM_45
#define LCD_G5_GPIO GPIO_NUM_48
#define LCD_G6_GPIO GPIO_NUM_47
#define LCD_G7_GPIO GPIO_NUM_21
 
#define LCD_R3_GPIO GPIO_NUM_1
#define LCD_R4_GPIO GPIO_NUM_2
#define LCD_R5_GPIO GPIO_NUM_42
#define LCD_R6_GPIO GPIO_NUM_41
#define LCD_R7_GPIO GPIO_NUM_40