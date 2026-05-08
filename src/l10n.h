#pragma once
#ifndef L10N_H
#define L10N_H

#include <Arduino.h>

struct L10n {
    const char* dash_title;
    const char* kb_label;
    const char* os_label;
    const char* grid_label;
    const char* bg_label;
    const char* btn_config;
    const char* btn_name_ph;
    const char* btn_cmd_ph;
    const char* type_app;
    const char* type_media;
    const char* type_basic;
    const char* type_adv;
    const char* save_changes;
    const char* library;
    const char* upload;
    const char* backup_title;
    const char* backup_btn;
    const char* restore_btn;
    const char* firmware_title;
    const char* firmware_info;
    const char* update_btn;
    const char* updating_msg;
    const char* confirm_restore;
    const char* restore_ok;
    const char* config_saved;
    const char* delete_file_confirm;
    const char* update_firmware_confirm;
    const char* settings_title;
    const char* global_bg;
    const char* grid_size;
    const char* target_os_label;
    const char* wifi_setup_label;
    const char* kb_lang_label;
    const char* back_btn;
    const char* cancel_btn;
    const char* save;
    const char* editing_btn_title;
    const char* editing_bg_title;
    const char* field_label;
    const char* field_icon;
    const char* field_action;
    const char* field_cmd;
    const char* field_img;
    const char* field_ssid;
    const char* field_pass;
    const char* wifi_save_connect;
    const char* select_grid;
    const char* select_os;
    const char* select_lang;
    const char* none;
    const char* basic_combo_desc;
    const char* button_label;
    const char* select_key_ph;
    const char* sym_names[20];
    const char* color_title;
    const char* icon_title;
    const char* image_title;
};

const L10n* get_l10n();

extern const char* g_sym_names[20];
extern const char* g_sym_codes[20];

const char* get_symbol_by_index(int idx);
int get_index_by_symbol(const char* sym);

#endif
