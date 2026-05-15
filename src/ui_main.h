#pragma once
#ifndef UI_MAIN_H
#define UI_MAIN_H

#include <lvgl.h>
#include "storage.h"

extern lv_obj_t* g_main_screen;
extern lv_obj_t* g_wifi_label;
extern volatile bool g_pending_ui_update;
extern volatile bool g_ota_screen_requested;
extern volatile int g_ota_progress;

void create_main_ui();
void refresh_main_ui();
void show_update_screen();
void update_ota_progress(int pct, const char* msg);

// Dirty region tracking
extern uint32_t g_dirty_buttons_mask;
extern bool g_dirty_bg;
extern bool g_dirty_layout;

void mark_all_dirty();
void io_yield();

#endif
