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

#endif
