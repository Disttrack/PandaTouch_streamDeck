#pragma once
#ifndef UI_HELPERS_H
#define UI_HELPERS_H

#include <lvgl.h>

typedef void (*selection_cb_t)(const char* selected);

void create_selection_screen(const char* title, const char* icon,
                             const char* options[], uint8_t count,
                             selection_cb_t on_select);

#endif
