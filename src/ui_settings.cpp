#include "ui_settings.h"
#include "ui_main.h"
#include "ui_helpers.h"
#include "storage.h"
#include "l10n.h"
#include "ble_actions.h"
#include "pt/pt_display.h"
#include <LittleFS.h>
#include <WiFi.h>

static lv_obj_t* g_settings_screen = nullptr;
static lv_obj_t* g_edit_screen = nullptr;
static lv_obj_t* g_wifi_screen = nullptr;
static bool g_editing_bg = false;
static uint8_t g_editing_idx = 0;
static bool g_settings_needs_rebuild = true;
static lv_obj_t* g_slider_r = nullptr;
static lv_obj_t* g_slider_g = nullptr;
static lv_obj_t* g_slider_b = nullptr;
static lv_obj_t* g_preview = nullptr;

struct WifiUIData {
    lv_obj_t* ta_ssid;
    lv_obj_t* ta_pass;
};
static WifiUIData g_wifi_data;

struct EditUIData {
    lv_obj_t* dd_icon;
    lv_obj_t* dd_type;
    lv_obj_t* ta_label;
    lv_obj_t* ta_value;
    lv_obj_t* dd_img;
};
static EditUIData g_edit_data;

// ========== Forward Declarations ==========
static void back_to_main_cb(lv_event_t* e);
static void save_edit_cb(lv_event_t* e);
static void save_wifi_cb(lv_event_t* e);
static void color_slider_cb(lv_event_t* e);
static void kb_focus_cb(lv_event_t* e);
static void settings_bg_btn_cb(lv_event_t* e);
static void settings_grid_btn_cb(lv_event_t* e);
static void settings_os_btn_cb(lv_event_t* e);
static void settings_wifi_btn_cb(lv_event_t* e);
static void settings_lang_btn_cb(lv_event_t* e);
static void edit_btn_select_cb(lv_event_t* e);
static void grid_selected(const char* txt);
static void os_selected(const char* txt);
static void lang_selected(const char* txt);

// ========== Settings Screen ==========
void create_settings_ui() {
    const L10n* l = get_l10n();
    if (g_settings_screen == nullptr || g_settings_needs_rebuild) {
        if (g_settings_screen != nullptr) {
            lv_obj_del(g_settings_screen);
        }

        g_settings_screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(g_settings_screen, lv_color_hex(g_bg_color), LV_PART_MAIN);

        lv_obj_t* title = lv_label_create(g_settings_screen);
        lv_label_set_text(title, l->settings_title);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

        lv_obj_t* list = lv_list_create(g_settings_screen);
        lv_obj_set_size(list, 600, 360);
        lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 45);

        lv_obj_t* bg_btn = lv_list_add_btn(list, "\xEF\x80\xBE", l->global_bg);
        lv_obj_add_event_cb(bg_btn, settings_bg_btn_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t* grid_btn = lv_list_add_btn(list, "\xEF\x80\x8A", l->grid_size);
        lv_obj_add_event_cb(grid_btn, settings_grid_btn_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t* os_btn = lv_list_add_btn(list, "\xEF\x84\xB9", l->target_os_label);
        lv_obj_add_event_cb(os_btn, settings_os_btn_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t* wifi_btn = lv_list_add_btn(list, "\xEF\x87\xAB", l->wifi_setup_label);
        lv_obj_add_event_cb(wifi_btn, settings_wifi_btn_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t* lang_btn = lv_list_add_btn(list, "\xEF\x81\x92", l->kb_lang_label);
        lv_obj_add_event_cb(lang_btn, settings_lang_btn_cb, LV_EVENT_CLICKED, NULL);

        int btn_count = g_rows * g_cols;
        for (int i = 0; i < btn_count; i++) {
            char buf[64];
            sprintf(buf, "%s %d: %s", (g_kb_lang == LANG_ES ? "Boton" : "Button"), (i + 1), g_configs[i].label);
            lv_obj_t* btn = lv_list_add_btn(list, "\xEF\x8C\x84", buf);
            lv_obj_add_event_cb(btn, edit_btn_select_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        }

        lv_obj_t* back = lv_btn_create(g_settings_screen);
        lv_obj_set_size(back, 140, 50);
        lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_t* lbl = lv_label_create(back);
        lv_label_set_text_fmt(lbl, "\xEF\x81\x93 %s", l->back_btn);
        lv_obj_add_event_cb(back, back_to_main_cb, LV_EVENT_CLICKED, NULL);

        g_settings_needs_rebuild = false;
    }

    lv_scr_load(g_settings_screen);
}

// ========== Edit Screen ==========
void create_edit_ui(uint8_t idx) {
    const L10n* l = get_l10n();
    if (!g_editing_bg) g_editing_idx = idx;
    g_edit_screen = lv_obj_create(NULL);
    lv_scr_load(g_edit_screen);
    lv_obj_set_style_bg_color(g_edit_screen, lv_color_hex(g_bg_color), LV_PART_MAIN);

    lv_obj_t* title = lv_label_create(g_edit_screen);
    if (g_editing_bg) lv_label_set_text(title, l->editing_bg_title);
    else lv_label_set_text_fmt(title, "%s %d", l->editing_btn_title, (idx + 1));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    uint32_t curr_color = g_editing_bg ? g_bg_color : g_configs[idx].color;

    if (!g_editing_bg) {
        lv_obj_t* l1 = lv_label_create(g_edit_screen);
        lv_label_set_text(l1, l->field_label);
        lv_obj_align(l1, LV_ALIGN_TOP_LEFT, 20, 35);
        g_edit_data.ta_label = lv_textarea_create(g_edit_screen);
        lv_textarea_set_one_line(g_edit_data.ta_label, true);
        lv_obj_set_size(g_edit_data.ta_label, 180, 40);
        lv_obj_align(g_edit_data.ta_label, LV_ALIGN_TOP_LEFT, 20, 55);
        lv_textarea_set_text(g_edit_data.ta_label, g_configs[idx].label);

        lv_obj_t* li = lv_label_create(g_edit_screen);
        lv_label_set_text(li, l->field_icon);
        lv_obj_align(li, LV_ALIGN_TOP_LEFT, 220, 35);
        g_edit_data.dd_icon = lv_dropdown_create(g_edit_screen);
        lv_obj_set_size(g_edit_data.dd_icon, 180, 40);

        String dd_opts = "";
        for (int j = 0; j < 20; j++) {
            if (j > 0) dd_opts += "\n";
            if (strlen(g_sym_codes[j]) > 0) dd_opts += String(g_sym_codes[j]) + " " + String(g_sym_names[j]);
            else dd_opts += String(g_sym_names[j]);
        }
        lv_dropdown_set_options(g_edit_data.dd_icon, dd_opts.c_str());
        lv_obj_align(g_edit_data.dd_icon, LV_ALIGN_TOP_LEFT, 220, 55);
        lv_dropdown_set_selected(g_edit_data.dd_icon, get_index_by_symbol(g_configs[idx].icon));

        lv_obj_t* l2 = lv_label_create(g_edit_screen);
        lv_label_set_text(l2, l->field_action);
        lv_obj_align(l2, LV_ALIGN_TOP_LEFT, 20, 105);
        g_edit_data.dd_type = lv_dropdown_create(g_edit_screen);
        lv_obj_set_size(g_edit_data.dd_type, 180, 40);
        String type_opts = String(l->type_app) + "\n" + l->type_media + "\n" + l->type_basic + "\n" + l->type_adv;
        lv_dropdown_set_options(g_edit_data.dd_type, type_opts.c_str());
        lv_obj_align(g_edit_data.dd_type, LV_ALIGN_TOP_LEFT, 20, 125);
        lv_dropdown_set_selected(g_edit_data.dd_type, g_configs[idx].type);

        lv_obj_t* l3 = lv_label_create(g_edit_screen);
        lv_label_set_text(l3, l->field_cmd);
        lv_obj_align(l3, LV_ALIGN_TOP_LEFT, 220, 105);
        g_edit_data.ta_value = lv_textarea_create(g_edit_screen);
        lv_textarea_set_one_line(g_edit_data.ta_value, true);
        lv_textarea_set_max_length(g_edit_data.ta_value, 255);
        lv_obj_set_size(g_edit_data.ta_value, 180, 40);
        lv_obj_align(g_edit_data.ta_value, LV_ALIGN_TOP_LEFT, 220, 125);
        lv_textarea_set_text(g_edit_data.ta_value, g_configs[idx].value);

        lv_obj_t* li2 = lv_label_create(g_edit_screen);
        lv_label_set_text(li2, l->field_img);
        lv_obj_align(li2, LV_ALIGN_TOP_LEFT, 120, 175);
        g_edit_data.dd_img = lv_dropdown_create(g_edit_screen);
        lv_obj_set_size(g_edit_data.dd_img, 180, 40);
        lv_obj_align(g_edit_data.dd_img, LV_ALIGN_TOP_LEFT, 120, 195);

        String opts = l->none;
        File root = LittleFS.open("/");
        File f = root.openNextFile();
        int sel_idx = 0;
        int current_f = 1;
        while (f) {
            String fname = f.name();
            if (!fname.startsWith("/")) fname = "/" + fname;
            opts += "\n" + fname.substring(1);
            if (fname == g_configs[idx].imgPath) sel_idx = current_f;
            f = root.openNextFile();
            current_f++;
        }
        lv_dropdown_set_options(g_edit_data.dd_img, opts.c_str());
        lv_dropdown_set_selected(g_edit_data.dd_img, sel_idx);
    }

    int panel_x = 450;
    auto create_rgb_slider = [&](lv_obj_t** slider, int y, uint8_t val, lv_color_t color) {
        *slider = lv_slider_create(g_edit_screen);
        lv_obj_set_size(*slider, 200, 15);
        lv_obj_align(*slider, LV_ALIGN_TOP_LEFT, panel_x, y);
        lv_slider_set_range(*slider, 0, 255);
        lv_slider_set_value(*slider, val, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(*slider, color, LV_PART_KNOB);
        lv_obj_add_event_cb(*slider, color_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    };

    create_rgb_slider(&g_slider_r, 55, (curr_color >> 16) & 0xFF, lv_color_hex(0xFF0000));
    create_rgb_slider(&g_slider_g, 95, (curr_color >> 8) & 0xFF, lv_color_hex(0x00FF00));
    create_rgb_slider(&g_slider_b, 135, (curr_color >> 0) & 0xFF, lv_color_hex(0x0000FF));

    g_preview = lv_obj_create(g_edit_screen);
    lv_obj_set_size(g_preview, 100, 100);
    lv_obj_align(g_preview, LV_ALIGN_TOP_LEFT, panel_x + 220, 50);
    lv_obj_set_style_bg_color(g_preview, lv_color_hex(curr_color), LV_PART_MAIN);

    if (!g_editing_bg) {
        lv_obj_t* kb = lv_keyboard_create(g_edit_screen);
        lv_keyboard_set_textarea(kb, g_edit_data.ta_label);
        lv_obj_set_size(kb, 780, 220);
        lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, -5);

        lv_obj_add_event_cb(g_edit_data.ta_label, kb_focus_cb, LV_EVENT_FOCUSED, kb);
        lv_obj_add_event_cb(g_edit_data.ta_value, kb_focus_cb, LV_EVENT_FOCUSED, kb);
    }

    lv_obj_t* save = lv_btn_create(g_edit_screen);
    lv_obj_set_size(save, 140, 50);
    lv_obj_align(save, LV_ALIGN_BOTTOM_RIGHT, -10, -5);
    lv_obj_t* sl = lv_label_create(save);
    lv_label_set_text_fmt(sl, "\xEF\x83\x87 %s", l->save);
    lv_obj_add_event_cb(save, save_edit_cb, LV_EVENT_CLICKED, &g_edit_data);

    lv_obj_t* cancel = lv_btn_create(g_edit_screen);
    lv_obj_set_size(cancel, 140, 50);
    lv_obj_align(cancel, LV_ALIGN_BOTTOM_LEFT, 10, -5);
    lv_obj_t* cl = lv_label_create(cancel);
    lv_label_set_text_fmt(cl, "\xEF\x80\x8D %s", l->cancel_btn);
    lv_obj_add_event_cb(cancel, back_to_main_cb, LV_EVENT_CLICKED, NULL);
}

// ========== WiFi Screen ==========
void create_wifi_ui() {
    const L10n* l = get_l10n();
    g_wifi_screen = lv_obj_create(NULL);
    lv_scr_load(g_wifi_screen);
    lv_obj_set_style_bg_color(g_wifi_screen, lv_color_hex(g_bg_color), LV_PART_MAIN);

    lv_obj_t* title = lv_label_create(g_wifi_screen);
    lv_label_set_text(title, l->wifi_setup_label);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t* l1 = lv_label_create(g_wifi_screen);
    lv_label_set_text(l1, l->field_ssid);
    lv_obj_align(l1, LV_ALIGN_TOP_LEFT, 20, 50);
    g_wifi_data.ta_ssid = lv_textarea_create(g_wifi_screen);
    lv_textarea_set_one_line(g_wifi_data.ta_ssid, true);
    lv_obj_set_size(g_wifi_data.ta_ssid, 350, 40);
    lv_obj_align(g_wifi_data.ta_ssid, LV_ALIGN_TOP_LEFT, 20, 70);
    lv_textarea_set_text(g_wifi_data.ta_ssid, g_wifi_ssid);

    lv_obj_t* l2 = lv_label_create(g_wifi_screen);
    lv_label_set_text(l2, l->field_pass);
    lv_obj_align(l2, LV_ALIGN_TOP_LEFT, 20, 120);
    g_wifi_data.ta_pass = lv_textarea_create(g_wifi_screen);
    lv_textarea_set_one_line(g_wifi_data.ta_pass, true);
    lv_textarea_set_password_mode(g_wifi_data.ta_pass, true);
    lv_obj_set_size(g_wifi_data.ta_pass, 350, 40);
    lv_obj_align(g_wifi_data.ta_pass, LV_ALIGN_TOP_LEFT, 20, 140);
    lv_textarea_set_text(g_wifi_data.ta_pass, g_wifi_pass);

    lv_obj_t* kb = lv_keyboard_create(g_wifi_screen);
    lv_keyboard_set_textarea(kb, g_wifi_data.ta_ssid);
    lv_obj_set_size(kb, 780, 240);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, -5);

    lv_obj_add_event_cb(g_wifi_data.ta_ssid, kb_focus_cb, LV_EVENT_FOCUSED, kb);
    lv_obj_add_event_cb(g_wifi_data.ta_pass, kb_focus_cb, LV_EVENT_FOCUSED, kb);

    lv_obj_t* save = lv_btn_create(g_wifi_screen);
    lv_obj_set_size(save, 160, 50);
    lv_obj_align(save, LV_ALIGN_TOP_RIGHT, -20, 70);
    lv_obj_t* sl = lv_label_create(save);
    lv_label_set_text_fmt(sl, "\xEF\x83\x87 %s", l->wifi_save_connect);
    lv_obj_add_event_cb(save, save_wifi_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* cancel = lv_btn_create(g_wifi_screen);
    lv_obj_set_size(cancel, 140, 50);
    lv_obj_align(cancel, LV_ALIGN_TOP_RIGHT, -20, 130);
    lv_obj_t* cl = lv_label_create(cancel);
    lv_label_set_text_fmt(cl, "\xEF\x80\x8D %s", l->cancel_btn);
    lv_obj_add_event_cb(cancel, back_to_main_cb, LV_EVENT_CLICKED, NULL);
}

// ========== Selection Callbacks ==========
static void grid_selected(const char* txt) {
    if (strcmp(txt, "2x2") == 0) { g_cols = 2; g_rows = 2; }
    else if (strcmp(txt, "3x2") == 0) { g_cols = 3; g_rows = 2; }
    else if (strcmp(txt, "3x3") == 0) { g_cols = 3; g_rows = 3; }
    else if (strcmp(txt, "4x3") == 0) { g_cols = 4; g_rows = 3; }
    else if (strcmp(txt, "5x3") == 0) { g_cols = 5; g_rows = 3; }

    g_settings_needs_rebuild = true;
    save_settings();
    lv_scr_load(g_main_screen);
    create_main_ui();
}

static void os_selected(const char* txt) {
    if (strcmp(txt, "Windows") == 0) g_target_os = OS_WINDOWS;
    else if (strcmp(txt, "macOS") == 0) g_target_os = OS_MACOS;

    save_settings(false);
    load_settings();
    lv_scr_load(g_main_screen);
    refresh_main_ui();
}

static void lang_selected(const char* txt) {
    if (strcmp(txt, "English (US)") == 0) g_kb_lang = LANG_US;
    else if (strcmp(txt, "Espanol (ES)") == 0) g_kb_lang = LANG_ES;

    save_settings(false);
    lv_scr_load(g_main_screen);
    refresh_main_ui();
}

// ========== Settings Button Callbacks ==========
static void settings_bg_btn_cb(lv_event_t* e) {
    g_editing_bg = true;
    create_edit_ui(0);
}

static void settings_grid_btn_cb(lv_event_t* e) {
    const char* grid_opts[] = {"2x2", "3x2", "3x3", "4x3", "5x3"};
    create_selection_screen(get_l10n()->select_grid, "\xEF\x80\x8A", grid_opts, 5, grid_selected);
}

static void settings_os_btn_cb(lv_event_t* e) {
    const char* os_opts[] = {"Windows", "macOS"};
    create_selection_screen(get_l10n()->select_os, "\xEF\x84\xB9", os_opts, 2, os_selected);
}

static void settings_wifi_btn_cb(lv_event_t* e) {
    create_wifi_ui();
}

static void settings_lang_btn_cb(lv_event_t* e) {
    const char* lang_opts[] = {"English (US)", "Espanol (ES)"};
    create_selection_screen(get_l10n()->select_lang, "\xEF\x81\x92", lang_opts, 2, lang_selected);
}

static void edit_btn_select_cb(lv_event_t* e) {
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    g_editing_bg = false;
    create_edit_ui(idx);
}

// ========== Other Callbacks ==========
static void back_to_main_cb(lv_event_t* e) {
    g_editing_bg = false;
    lv_scr_load(g_main_screen);
    refresh_main_ui();
}

static void save_wifi_cb(lv_event_t* e) {
    strncpy(g_wifi_ssid, lv_textarea_get_text(g_wifi_data.ta_ssid), 31);
    strncpy(g_wifi_pass, lv_textarea_get_text(g_wifi_data.ta_pass), 63);

    save_settings();
    WiFi.disconnect();
    WiFi.begin(g_wifi_ssid, g_wifi_pass);

    lv_scr_load(g_main_screen);
    refresh_main_ui();
}

static void color_slider_cb(lv_event_t* e) {
    uint8_t r = (uint8_t)lv_slider_get_value(g_slider_r);
    uint8_t g = (uint8_t)lv_slider_get_value(g_slider_g);
    uint8_t b = (uint8_t)lv_slider_get_value(g_slider_b);
    lv_obj_set_style_bg_color(g_preview, lv_color_make(r, g, b), LV_PART_MAIN);
}

static void kb_focus_cb(lv_event_t* e) {
    lv_obj_t* ta = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t* kb = (lv_obj_t*)lv_event_get_user_data(e);
    lv_keyboard_set_textarea(kb, ta);
}

static void save_edit_cb(lv_event_t* e) {
    const L10n* l = get_l10n();
    EditUIData* data = (EditUIData*)lv_event_get_user_data(e);

    uint8_t r = (uint8_t)lv_slider_get_value(g_slider_r);
    uint8_t g = (uint8_t)lv_slider_get_value(g_slider_g);
    uint8_t b = (uint8_t)lv_slider_get_value(g_slider_b);
    uint32_t hex = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;

    if (g_editing_bg) {
        g_bg_color = hex;
    } else {
        memset(g_configs[g_editing_idx].label, 0, 16);
        memset(g_configs[g_editing_idx].value, 0, 256);
        memset(g_configs[g_editing_idx].icon, 0, 8);
        memset(g_configs[g_editing_idx].imgPath, 0, 32);

        if (data) {
            strncpy(g_configs[g_editing_idx].label, lv_textarea_get_text(data->ta_label), 15);
            strncpy(g_configs[g_editing_idx].value, lv_textarea_get_text(data->ta_value), 255);
            g_configs[g_editing_idx].type = (uint8_t)lv_dropdown_get_selected(data->dd_type);

            const char* sym = get_symbol_by_index(lv_dropdown_get_selected(data->dd_icon));
            if (sym) {
                size_t sym_len = strlen(sym);
                if (sym_len > 0 && sym_len < 8) {
                    strncpy(g_configs[g_editing_idx].icon, sym, sym_len);
                    g_configs[g_editing_idx].icon[sym_len] = '\0';
                }
            }

            char buf[64];
            lv_dropdown_get_selected_str(data->dd_img, buf, sizeof(buf));
            if (strcmp(buf, l->none) == 0) {
                g_configs[g_editing_idx].imgPath[0] = '\0';
            } else {
                String val = buf;
                if (!val.startsWith("/")) val = "/" + val;
                strncpy(g_configs[g_editing_idx].imgPath, val.c_str(), 31);
                g_configs[g_editing_idx].imgPath[31] = '\0';
            }
        }

        g_configs[g_editing_idx].color = hex;
    }

    save_settings();
    g_editing_bg = false;
    lv_scr_load(g_main_screen);
    refresh_main_ui();
}
