#include "ui_helpers.h"
#include "storage.h"
#include <stdlib.h>

struct SelectionCtx {
    selection_cb_t cb;
    lv_obj_t* screen;
};

static void selection_item_cb(lv_event_t* e) {
    SelectionCtx* ctx = (SelectionCtx*)lv_event_get_user_data(e);
    lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);
    const char* txt = lv_list_get_button_text(lv_obj_get_parent(obj), obj);
    if (txt && ctx->cb) {
        ctx->cb(txt);
    }
    if (ctx->screen) {
        lv_obj_del(ctx->screen);
    }
    free(ctx);
}

static void selection_back_cb(lv_event_t* e) {
    SelectionCtx* ctx = (SelectionCtx*)lv_event_get_user_data(e);
    if (ctx->screen) {
        lv_obj_del(ctx->screen);
    }
    free(ctx);
}

void create_selection_screen(const char* title, const char* icon,
                             const char* options[], uint8_t count,
                             selection_cb_t on_select) {
    SelectionCtx* ctx = (SelectionCtx*)malloc(sizeof(SelectionCtx));
    if (!ctx) return;
    ctx->cb = on_select;

    ctx->screen = lv_obj_create(NULL);
    lv_scr_load(ctx->screen);
    lv_obj_set_style_bg_color(ctx->screen, lv_color_hex(g_bg_color), LV_PART_MAIN);

    lv_obj_t* title_label = lv_label_create(ctx->screen);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_18, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t* list = lv_list_create(ctx->screen);
    lv_obj_set_size(list, 400, 320);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);

    for (uint8_t i = 0; i < count; i++) {
        lv_obj_t* btn = lv_list_add_btn(list, icon, options[i]);
        lv_obj_add_event_cb(btn, selection_item_cb, LV_EVENT_CLICKED, ctx);
    }

    lv_obj_t* back = lv_btn_create(ctx->screen);
    lv_obj_set_size(back, 140, 50);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_t* lbl = lv_label_create(back);
    lv_label_set_text_fmt(lbl, "\xEF\x80\x8D Back");
    lv_obj_add_event_cb(back, selection_back_cb, LV_EVENT_CLICKED, ctx);
}
