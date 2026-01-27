#pragma once
#ifndef PT_DISPLAY_H
#define PT_DISPLAY_H

/* =========================
 *  Includes
 * ========================= */
#include <Arduino.h>
#include <algorithm>
#include <driver/ledc.h>
#include <esp_heap_caps.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include "TAMC_GT911.h"
#include "pt_board.h"

#ifndef PT_LVGL_RENDER_PARTIAL_LINES
#define PT_LVGL_RENDER_PARTIAL_LINES 80
#endif

#ifndef PT_LCD_RENDER_BOUNCE_LINES
#define PT_LCD_RENDER_BOUNCE_LINES 10
#endif

/* =========================
 *  Types / Render modes
 * ========================= */

// Function pointer type for scheduling work on the LVGL thread
typedef void (*pt_ui_fn_t)(void *arg);

// Render method enum (mirrors board Kconfig options)
typedef enum
{
  PT_LVGL_RENDER_FULL_1 = 0,
  PT_LVGL_RENDER_FULL_2,
  PT_LVGL_RENDER_PARTIAL_1,
  PT_LVGL_RENDER_PARTIAL_2,
  PT_LVGL_RENDER_PARTIAL_1_PSRAM,
  PT_LVGL_RENDER_PARTIAL_2_PSRAM,
} PT_LVGL_render_method_t;

// Default render method when not provided by build system
#ifndef PT_LVGL_RENDER_METHOD
#define PT_LVGL_RENDER_METHOD PT_LVGL_RENDER_PARTIAL_2
#endif

/* =========================
 *  Display / Touch Objects
 * ========================= */
TAMC_GT911 pt_touchpanel(
    PT_I2C0_SDA_PIN,
    PT_I2C0_SCL_PIN,
    PT_GT911_IRQ_PIN,
    PT_GT911_RST_PIN,
    std::max(PT_LCD_H_RES, 0),
    std::max(PT_LCD_V_RES, 0));

#if defined(ESP_ARDUINO_VERSION_MAJOR)
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
Arduino_ESP32RGBPanel pt_rgbpanel(
    PT_LCD_DE_PIN, PT_LCD_VSYNC_PIN, PT_LCD_HSYNC_PIN, PT_LCD_PCLK_PIN,
    // Data pins: R3-R7, G2-G7, B3-B7 (Correct RGB order)
    PT_LCD_R3_PIN, PT_LCD_R4_PIN, PT_LCD_R5_PIN, PT_LCD_R6_PIN, PT_LCD_R7_PIN,
    PT_LCD_G2_PIN, PT_LCD_G3_PIN, PT_LCD_G4_PIN, PT_LCD_G5_PIN, PT_LCD_G6_PIN, PT_LCD_G7_PIN,
    PT_LCD_B3_PIN, PT_LCD_B4_PIN, PT_LCD_B5_PIN, PT_LCD_B6_PIN, PT_LCD_B7_PIN,
    0 /* hsync_polarity */, PT_LCD_HSYNC_PULSE_WIDTH, PT_LCD_HSYNC_BACK_PORCH, PT_LCD_HSYNC_FRONT_PORCH,
    0 /* vsync_polarity */, PT_LCD_VSYNC_PULSE_WIDTH, PT_LCD_VSYNC_BACK_PORCH, PT_LCD_VSYNC_FRONT_PORCH,
    1 /* pclk_active_neg */,
    PT_LCD_PCLK_HZ, false /* useBigEndian */,
    0 /* de_idle_high */, 0 /* pclk_idle_high */, PT_LCD_RENDER_BOUNCE_LINES *PT_LCD_H_RES /* bounce_buffer_size_px */);
#else
Arduino_ESP32RGBPanel pt_rgbpanel(
    PT_LCD_DE_PIN, PT_LCD_VSYNC_PIN, PT_LCD_HSYNC_PIN, PT_LCD_PCLK_PIN,
    // Data pins: R3-R7, G2-G7, B3-B7 (Correct RGB order)
    PT_LCD_R3_PIN, PT_LCD_R4_PIN, PT_LCD_R5_PIN, PT_LCD_R6_PIN, PT_LCD_R7_PIN,
    PT_LCD_G2_PIN, PT_LCD_G3_PIN, PT_LCD_G4_PIN, PT_LCD_G5_PIN, PT_LCD_G6_PIN, PT_LCD_G7_PIN,
    PT_LCD_B3_PIN, PT_LCD_B4_PIN, PT_LCD_B5_PIN, PT_LCD_B6_PIN, PT_LCD_B7_PIN,
    0 /* hsync_polarity */, PT_LCD_HSYNC_PULSE_WIDTH, PT_LCD_HSYNC_BACK_PORCH, PT_LCD_HSYNC_FRONT_PORCH,
    0 /* vsync_polarity */, PT_LCD_VSYNC_PULSE_WIDTH, PT_LCD_VSYNC_BACK_PORCH, PT_LCD_VSYNC_FRONT_PORCH,
    1 /* pclk_active_neg */,
    PT_LCD_PCLK_HZ, false /* useBigEndian */);
#endif
#else
// Fallback if version macros not present
#pragma message "ESP_ARDUINO_VERSION_MAJOR not defined — assume old core"
#define USE_LCD_TEARING_FIX 0
#endif

Arduino_RGB_Display pt_gfx(PT_LCD_H_RES, PT_LCD_V_RES, &pt_rgbpanel, 0, true);

/* =========================
 *  State / LVGL Buffers
 * ========================= */

lv_color_t *pt_disp_draw_buf;
lv_color_t *pt_disp_draw_buf2 = NULL; // optional second buffer for double-buffering

/* =========================
 *  Backlight (LEDC) Config
 * ========================= */

static uint8_t pt_backlight_percent = 100;

/* =========================
 *  Helpers
 * ========================= */

/**
 * @brief Returns the number of milliseconds since the program started.
 *
 * This inline function calls the underlying `millis()` function to retrieve
 * the elapsed time in milliseconds. Useful for timing and scheduling tasks.
 *
 * @return uint32_t Number of milliseconds since the program started.
 */
inline uint32_t millis_cb()
{
  return millis();
}

/**
 * @brief Converts a brightness percentage to LEDC duty cycle value.
 *
 * This function takes a percentage value (0-100) and calculates the corresponding
 * duty cycle for the LEDC (LED Controller) based on a timer with 11-bit resolution.
 * The duty cycle determines the brightness of the backlight.
 *
 * @param percent Brightness percentage (0-100).
 * @return Calculated duty cycle value for the LEDC. Returns 0 if the computed duty is less than 1.
 */
static uint32_t pt_get_duty_from_percent(uint32_t percent)
{
  uint32_t duty = (uint32_t)(((float)percent / 100.0f) * ((1 << LEDC_TIMER_11_BIT) - 1));
  return (duty < 1) ? 0 : duty;
}

/**
 * @brief Sets the backlight brightness to a specified percentage.
 *
 * This inline function adjusts the backlight brightness by setting the LEDC duty cycle
 * based on the provided percentage. It ensures the percentage is within the valid range (0-100).
 * If `save` is true, it updates the global `pt_backlight_percent` variable to remember
 * the current brightness setting.
 *
 * @param percent Brightness percentage (0-100).
 * @param save If true, saves the current brightness setting.
 */
inline void pt_set_backlight(uint8_t percent, bool save)
{
  if (percent > 100)
    percent = 100;
  uint32_t target_duty = pt_get_duty_from_percent(percent);
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, target_duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
  if (save)
  {
    pt_backlight_percent = percent;
  }
}

/**
 * @brief Initializes the backlight control.
 *
 * This function sets up the LEDC timer and channel for controlling the backlight brightness.
 * It also sets the initial brightness level.
 *
 * @param set_percent Initial brightness percentage (0-100).
 */
static void pt_init_backlight(uint8_t set_percent)
{
  // Initialize backlight at 0% to avoid the lcd reset flash
  ledc_timer_config_t timer_config = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_TIMER_11_BIT,
      .timer_num = LEDC_TIMER_1,
      .freq_hz = PT_LCD_BL_FREQUENCY_HZ,
      .clk_cfg = LEDC_USE_APB_CLK};
  ledc_timer_config(&timer_config);

  ledc_channel_config_t channel_config = {
      .gpio_num = PT_LCD_BL_PIN,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_1,
      .duty = 0,
      .hpoint = 0};
  ledc_channel_config(&channel_config);

  ledc_fade_func_install(0);

  if (set_percent < 0)
  {
    set_percent = 100;
    pt_set_backlight(set_percent, true);
  }
  else
  {
    pt_backlight_percent = set_percent;
    if (set_percent != 0)
    {
      pt_set_backlight(0, false);
    }
    else
    {
      pt_set_backlight(0, true);
    }
  }
}

/* =========================
 *  LVGL Callbacks
 * ========================= */

/**
 * @brief Flushes the display buffer to the screen.
 *
 * This function is called by LVGL to update the display with the contents of the
 * provided pixel map.
 *
 * @param disp Pointer to the LVGL display object.
 * @param area Pointer to the area to be updated.
 * @param px_map Pointer to the pixel map data.
 */
inline void pt_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);

  // In 16-bit mode, we can directly draw the 16-bit bitmap.
  // The Red/Blue swap is handled by corrected pin assignment in the constructor.
  pt_gfx.draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);

  lv_disp_flush_ready(disp);
}

/**
 * @brief Reads touchpad input data.
 *
 * This function is called by LVGL to read touch input data from the touch panel.
 *
 * @param indev Pointer to the LVGL input device object.
 * @param data Pointer to the LVGL input device data structure.
 */
inline void pt_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  pt_touchpanel.read(); // Read touch data
  if (pt_touchpanel.isTouched)
  {
    for (int i = 0; i < pt_touchpanel.touches; i++)
    {
      if (i == 0)
      {
        // Validate coordinates to avoid 65535 or out of bounds on I2C failure
        if (pt_touchpanel.points[i].x < 800 && pt_touchpanel.points[i].y < 480) {
            data->state = LV_INDEV_STATE_PRESSED;
            data->point.x = pt_touchpanel.points[i].x;
            data->point.y = pt_touchpanel.points[i].y;
        } else {
            data->state = LV_INDEV_STATE_RELEASED;
        }
      }
    }
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

/* =========================
 *  Public API
 * ========================= */

/**
 * @brief Sets up the display.
 *
 * This function initializes the display settings and prepares it for use.
 */
inline void pt_setup_display(PT_LVGL_render_method_t mode = (PT_LVGL_render_method_t)PT_LVGL_RENDER_METHOD)
{
  uint32_t screenWidth;
  uint32_t screenHeight;
  uint32_t bufSize;
  lv_display_t *disp;

  pinMode(PT_LCD_RESET_PIN, OUTPUT);
  digitalWrite(PT_LCD_RESET_PIN, 0);
  delay(100);
  digitalWrite(PT_LCD_RESET_PIN, 1);
  delay(10);

  // Backlight setup
  pt_init_backlight(100); 
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (1 << LEDC_TIMER_11_BIT) - 1);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

  // Panel bring-up
  pt_gfx.begin();
  pt_gfx.fillScreen(0x000000);

  // Touch
  pt_touchpanel.begin();
  pt_touchpanel.setRotation(1);

  // LVGL core
  lv_init();
  lv_tick_set_cb(millis_cb);

  screenWidth = pt_gfx.width();
  screenHeight = pt_gfx.height();

  // Create display
  disp = lv_display_create(screenWidth, screenHeight);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp, pt_disp_flush);

  // Allocation helper
  auto alloc_buf = [&](size_t count, bool prefer_psram) -> lv_color_t *
  {
    int caps = MALLOC_CAP_8BIT;
    if (prefer_psram) caps |= MALLOC_CAP_SPIRAM;
    return (lv_color_t *)heap_caps_malloc(count * sizeof(lv_color_t), caps);
  };

  switch (mode)
  {
  case PT_LVGL_RENDER_FULL_1:
    bufSize = screenWidth * screenHeight;
    pt_disp_draw_buf = alloc_buf(bufSize, true);
    if (pt_disp_draw_buf)
    {
      lv_display_set_buffers(disp, pt_disp_draw_buf, NULL, bufSize * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_FULL);
      break;
    }

  case PT_LVGL_RENDER_FULL_2:
    bufSize = screenWidth * screenHeight;
    pt_disp_draw_buf = alloc_buf(bufSize, true);
    if (pt_disp_draw_buf)
      pt_disp_draw_buf2 = alloc_buf(bufSize, true);
    if (pt_disp_draw_buf && pt_disp_draw_buf2)
    {
      lv_display_set_buffers(disp, pt_disp_draw_buf, pt_disp_draw_buf2, bufSize * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_FULL);
      break;
    }

  case PT_LVGL_RENDER_PARTIAL_1:
  case PT_LVGL_RENDER_PARTIAL_1_PSRAM:
    bufSize = screenWidth * PT_LVGL_RENDER_PARTIAL_LINES;
    pt_disp_draw_buf = alloc_buf(bufSize, mode == PT_LVGL_RENDER_PARTIAL_1_PSRAM);
    if (!pt_disp_draw_buf) pt_disp_draw_buf = alloc_buf(bufSize, mode != PT_LVGL_RENDER_PARTIAL_1_PSRAM);
    if (!pt_disp_draw_buf) return;
    lv_display_set_buffers(disp, pt_disp_draw_buf, NULL, bufSize * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    break;

  case PT_LVGL_RENDER_PARTIAL_2:
  case PT_LVGL_RENDER_PARTIAL_2_PSRAM:
  default:
    bufSize = screenWidth * PT_LVGL_RENDER_PARTIAL_LINES;
    bool use_psram = (mode == PT_LVGL_RENDER_PARTIAL_2_PSRAM);
    pt_disp_draw_buf = alloc_buf(bufSize, use_psram);
    if (pt_disp_draw_buf) pt_disp_draw_buf2 = alloc_buf(bufSize, use_psram);
    
    if (pt_disp_draw_buf && pt_disp_draw_buf2)
      lv_display_set_buffers(disp, pt_disp_draw_buf, pt_disp_draw_buf2, bufSize * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    else if (pt_disp_draw_buf)
      lv_display_set_buffers(disp, pt_disp_draw_buf, NULL, bufSize * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    break;
  }

  // Touch input device
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, pt_touchpad_read);
}

/**
 * @brief Main loop for display tasks.
 *
 * This function is called repeatedly to handle
 * refreshing the screen and processing input events.
 */
inline void pt_loop_display()
{
  lv_task_handler();
}

#endif // PT_DISPLAY_H

