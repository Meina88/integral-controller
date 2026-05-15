#include "screen_config_machine.h"
#include "ui/fonts/fonts.h"
#include "ui/ui_theme.h"
#include "ui/ui_manager.h"
#include "lvgl.h"

static void set_dark_cb(lv_event_t *e)
{
    ui_theme_set(UI_THEME_DARK);
    ui_rebuild();
}

static void set_light_cb(lv_event_t *e)
{
    ui_theme_set(UI_THEME_LIGHT);
    ui_rebuild();
}

lv_obj_t *screen_config_machine_create(lv_obj_t *parent)
{
    const ui_theme_t *th = ui_theme_get();

    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, th->bg, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 20, 0);
    lv_obj_set_style_pad_gap(root, 12, 0);
    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_section = lv_label_create(root);
    lv_label_set_text(lbl_section, "Apariencia");
    lv_obj_set_style_text_font(lbl_section, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_section, th->muted, 0);

    lv_obj_t *row = lv_obj_create(root);
    lv_obj_set_width(row, LV_SIZE_CONTENT);
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_gap(row, 12, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    bool is_dark = (th->id == UI_THEME_DARK);

    lv_obj_t *btn_dark = lv_btn_create(row);
    lv_obj_set_size(btn_dark, 180, 52);
    lv_obj_set_style_bg_color(btn_dark, is_dark ? th->blue : th->pressed, 0);
    lv_obj_set_style_bg_opa(btn_dark, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_dark, 0, 0);
    lv_obj_set_style_shadow_width(btn_dark, 0, 0);
    lv_obj_set_style_radius(btn_dark, 8, 0);
    if (!is_dark)
        lv_obj_add_event_cb(btn_dark, set_dark_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_dark = lv_label_create(btn_dark);
    lv_label_set_text(lbl_dark, "Oscuro");
    lv_obj_set_style_text_font(lbl_dark, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_dark, is_dark ? lv_color_white() : th->subtle, 0);
    lv_obj_center(lbl_dark);

    lv_obj_t *btn_light = lv_btn_create(row);
    lv_obj_set_size(btn_light, 180, 52);
    lv_obj_set_style_bg_color(btn_light, !is_dark ? th->blue : th->pressed, 0);
    lv_obj_set_style_bg_opa(btn_light, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_light, 0, 0);
    lv_obj_set_style_shadow_width(btn_light, 0, 0);
    lv_obj_set_style_radius(btn_light, 8, 0);
    if (is_dark)
        lv_obj_add_event_cb(btn_light, set_light_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_light = lv_label_create(btn_light);
    lv_label_set_text(lbl_light, "Claro");
    lv_obj_set_style_text_font(lbl_light, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_light, !is_dark ? lv_color_white() : th->subtle, 0);
    lv_obj_center(lbl_light);

    return root;
}
