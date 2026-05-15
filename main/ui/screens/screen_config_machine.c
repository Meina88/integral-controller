#include "screen_config_machine.h"
#include "ui/fonts/fonts.h"
#include "lvgl.h"

#define C_BG    lv_color_hex(0x111827)
#define C_MUTED lv_color_hex(0x64748B)

lv_obj_t *screen_config_machine_create(lv_obj_t *parent)
{
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, C_BG, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(root);
    lv_label_set_text(lbl, "Prox. disponible");
    lv_obj_set_style_text_font(lbl, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl, C_MUTED, 0);
    lv_obj_center(lbl);

    return root;
}
