
#ifndef PT_BOARD_H
#define PT_BOARD_H

// LCD panel parameter
#define PT_LCD_H_RES 800
#define PT_LCD_V_RES 480
#define PT_LCD_HSYNC_PULSE_WIDTH 4
#define PT_LCD_VSYNC_PULSE_WIDTH 4
#define PT_LCD_HSYNC_BACK_PORCH 16
#define PT_LCD_VSYNC_BACK_PORCH 32
#define PT_LCD_HSYNC_FRONT_PORCH 16
#define PT_LCD_VSYNC_FRONT_PORCH 32
#define PT_LCD_PCLK_HZ 14800000

// LCD pinout

#define PT_LCD_DISP_PIN -1
#define PT_LCD_PCLK_PIN 5
#define PT_LCD_HSYNC_PIN -1
#define PT_LCD_VSYNC_PIN -1
#define PT_LCD_DE_PIN 38
#define PT_LCD_B3_PIN 17
#define PT_LCD_B4_PIN 18
#define PT_LCD_B5_PIN 48
#define PT_LCD_B6_PIN 47
#define PT_LCD_B7_PIN 39
#define PT_LCD_G2_PIN 11
#define PT_LCD_G3_PIN 12
#define PT_LCD_G4_PIN 13
#define PT_LCD_G5_PIN 14
#define PT_LCD_G6_PIN 15
#define PT_LCD_G7_PIN 16
#define PT_LCD_R3_PIN 6
#define PT_LCD_R4_PIN 7
#define PT_LCD_R5_PIN 8
#define PT_LCD_R6_PIN 9
#define PT_LCD_R7_PIN 10

#define PT_LCD_RESET_PIN 46
#define PT_LCD_BL_PIN 21
#define PT_LCD_BL_FREQUENCY_HZ 30000

// GT911 Touch
#define PT_I2C0_SPEED 100000
#define PT_I2C0_SCL_PIN 1
#define PT_I2C0_SDA_PIN 2

#define PT_GT911_I2C I2C_NUM_0
#define PT_GT911_IRQ_PIN 40
#define PT_GT911_RST_PIN 41

#endif
