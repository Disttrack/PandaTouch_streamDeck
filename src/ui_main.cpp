#include "ui_main.h"
#include "ui_settings.h"
#include "ble_actions.h"
#include "pt/pt_display.h"
#include <LittleFS.h>

lv_obj_t* g_main_screen = nullptr;
lv_obj_t* g_wifi_label = nullptr;
volatile bool g_pending_ui_update = false;

static lv_obj_t* g_update_screen = nullptr;
static lv_obj_t* g_update_bar = nullptr;
static lv_obj_t* g_update_label = nullptr;
static lv_obj_t* g_update_pct_label = nullptr;

static void btn_event_cb(lv_event_t* e) {
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    handle_button_action(idx);
}

static void slider_event_cb(lv_event_t* e) {
    lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    pt_set_backlight((uint8_t)val, true);
}

static void settings_btn_cb(lv_event_t* e) {
    create_settings_ui();
}

void create_main_ui() {
    lv_obj_clean(g_main_screen);
    lv_obj_set_style_bg_color(g_main_screen, lv_color_hex(g_bg_color), LV_PART_MAIN);

    static int32_t col_dsc[10];
    static int32_t row_dsc[10];

    if (g_rows < 1 || g_rows > 5) g_rows = 3;
    if (g_cols < 1 || g_cols > 5) g_cols = 3;

    int32_t availW = 800 - 20 - (g_cols - 1) * 10;
    int32_t availH = 480 - 20 - 60 - g_rows * 10;

    int32_t cellW = availW / g_cols;
    int32_t cellH = availH / g_rows;

    for (int i = 0; i < g_cols; i++) col_dsc[i] = cellW;
    col_dsc[g_cols] = LV_GRID_TEMPLATE_LAST;

    for (int i = 0; i < g_rows; i++) row_dsc[i] = cellH;
    row_dsc[g_rows] = 60;
    row_dsc[g_rows + 1] = LV_GRID_TEMPLATE_LAST;

    lv_obj_t* grid = lv_obj_create(g_main_screen);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_size(grid, lv_pct(100), lv_pct(100));
    lv_obj_center(grid);
    lv_obj_set_style_bg_color(grid, lv_color_hex(g_bg_color), LV_PART_MAIN);
    lv_obj_set_style_border_width(grid, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(grid, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(grid, 10, LV_PART_MAIN);

    int btn_count = g_rows * g_cols;
    for (int i = 0; i < btn_count; i++) {
        lv_obj_t* btn = lv_btn_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % g_cols, 1, LV_GRID_ALIGN_STRETCH, i / g_cols, 1);
        lv_obj_set_style_bg_color(btn, lv_color_hex(g_configs[i].color), LV_PART_MAIN);

        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(btn, 5, 0);

        bool icon_or_img_present = false;
        if (g_configs[i].imgPath[0] != '\0') {
            String fpath = g_configs[i].imgPath;
            if (!fpath.startsWith("/")) fpath = "/" + fpath;

            if (LittleFS.exists(fpath)) {
                lv_obj_t* img = lv_image_create(btn);
                char full_path[64];
                sprintf(full_path, "L:%s", fpath.c_str());
                lv_image_set_src(img, full_path);

                if (g_cols > 4 || g_rows > 3) lv_obj_set_size(img, 48, 48);
                else lv_obj_set_size(img, 64, 64);

                icon_or_img_present = true;
            }
        }

        if (!icon_or_img_present && g_configs[i].icon[0] != '\0') {
            lv_obj_t* icon = lv_label_create(btn);
            lv_label_set_text(icon, g_configs[i].icon);
            if (g_cols > 4) lv_obj_set_style_text_font(icon, &lv_font_montserrat_18, 0);
            else lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
            icon_or_img_present = true;
        }

        if (g_configs[i].label[0] != '\0') {
            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_text(label, g_configs[i].label);
            if (g_cols > 4) lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
            else lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        }

        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
    }

    lv_obj_t* slider = lv_slider_create(grid);
    lv_slider_set_range(slider, 10, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    lv_obj_set_grid_cell(slider, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, g_rows, 1);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    g_wifi_label = lv_label_create(grid);
    String wtxt = "\xEF\x87\xAB " + g_ip_addr;
    lv_label_set_text(g_wifi_label, wtxt.c_str());
    lv_obj_set_grid_cell(g_wifi_label, LV_GRID_ALIGN_CENTER, 1, (g_cols > 2 ? g_cols - 2 : 1), LV_GRID_ALIGN_CENTER, g_rows, 1);

    lv_obj_t* set_btn = lv_btn_create(grid);
    lv_obj_set_grid_cell(set_btn, LV_GRID_ALIGN_STRETCH, g_cols - 1, 1, LV_GRID_ALIGN_STRETCH, g_rows, 1);
    lv_obj_t* set_label = lv_label_create(set_btn);
    lv_label_set_text(set_label, "\xEF\x80\x93 Config");
    lv_obj_add_event_cb(set_btn, settings_btn_cb, LV_EVENT_CLICKED, NULL);
}

void update_ota_progress(int pct, const char* msg) {
    if (!g_update_screen) return;

    if (msg && g_update_label) {
        lv_label_set_text(g_update_label, msg);
    }

    if (g_update_bar) {
        if (pct >= 0) {
            lv_bar_set_value(g_update_bar, pct, LV_ANIM_ON);
            if (g_update_pct_label) {
                lv_label_set_text_fmt(g_update_pct_label, "%d%%", pct);
            }
        }
    }

    lv_timer_handler();
}

void show_update_screen() {
    if (g_update_screen) {
        lv_scr_load(g_update_screen);
        return;
    }

    g_update_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_update_screen, lv_color_hex(0x1a1a1a), LV_PART_MAIN);

    lv_obj_t* cont = lv_obj_create(g_update_screen);
    lv_obj_set_size(cont, 400, 220);
    lv_obj_center(cont);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x2a2a2a), LV_PART_MAIN);
    lv_obj_set_style_border_color(cont, lv_color_hex(0xffaa00), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(cont, 10, LV_PART_MAIN);

    g_update_label = lv_label_create(cont);
    lv_label_set_text(g_update_label, "Updating System...");
    lv_obj_set_style_text_color(g_update_label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_align(g_update_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(g_update_label, LV_ALIGN_TOP_MID, 0, 20);

    g_update_bar = lv_bar_create(cont);
    lv_obj_set_size(g_update_bar, 300, 20);
    lv_obj_align(g_update_bar, LV_ALIGN_CENTER, 0, 10);
    lv_bar_set_range(g_update_bar, 0, 100);
    lv_bar_set_value(g_update_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g_update_bar, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_update_bar, lv_color_hex(0xffaa00), LV_PART_INDICATOR);

    g_update_pct_label = lv_label_create(cont);
    lv_label_set_text(g_update_pct_label, "0%");
    lv_obj_set_style_text_font(g_update_pct_label, &lv_font_montserrat_14, 0);
    lv_obj_align(g_update_pct_label, LV_ALIGN_CENTER, 0, 35);

    lv_obj_t* spinner = lv_spinner_create(cont);
    lv_obj_set_size(spinner, 30, 30);
    lv_obj_align(spinner, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(0xffaa00), LV_PART_INDICATOR);

    lv_scr_load(g_update_screen);
    lv_timer_handler();
}
