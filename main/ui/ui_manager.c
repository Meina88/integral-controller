#include "lvgl.h"

void ui_init(void)
{
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Integral Controller v0.1");
    lv_obj_center(label);
}