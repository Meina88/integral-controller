#include "lvgl.h"
#include "ui_manager.h"
#include "logic/extrusion.h"
#include <stdio.h>

static lv_obj_t *label_speed;

// =========================
// UI INIT
// =========================
void ui_start(void)
{
    lv_obj_t *scr = lv_scr_act();

    label_speed = lv_label_create(scr);
    lv_label_set_text(label_speed, "0.0 m/min");

    //lv_obj_set_style_text_font(label_speed, &lv_font_montserrat_28, 0);
    lv_obj_center(label_speed);
}

// =========================
// UI UPDATE
// =========================
void ui_update(void)
{
    char buf[32];

    float speed = extrusion_get_speed_m_min();

    snprintf(buf, sizeof(buf), "%.2f m/min", speed);

    lv_label_set_text(label_speed, buf);
}