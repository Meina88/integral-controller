#include "lvgl.h"
#include <stdio.h>

static bool relay_state = false;

// 🔥 callback botón
static void relay_btn_event_cb(lv_event_t *e)
{

    printf("TOUCH DETECTED\n");
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *led = (lv_obj_t *)lv_event_get_user_data(e);

    relay_state = !relay_state;

    // texto
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    lv_label_set_text(label, relay_state ? "RELAY ON" : "RELAY OFF");

    // color botón
    lv_obj_set_style_bg_color(btn,
        relay_state ? lv_palette_main(LV_PALETTE_GREEN)
                    : lv_palette_main(LV_PALETTE_RED),
        0);

    // LED
    if (relay_state)
        lv_led_on(led);
    else
        lv_led_off(led);
}

void screen_home_create(void)
{
    lv_obj_t *scr = lv_scr_act();

    // 🔥 título
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Integral Controller");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // 🔥 LED (sensor)
    lv_obj_t *led = lv_led_create(scr);
    lv_obj_set_size(led, 30, 30);
    lv_obj_align(led, LV_ALIGN_CENTER, 0, -40);
    lv_led_off(led);

    // 🔥 label sensor
    lv_obj_t *sensor_label = lv_label_create(scr);
    lv_label_set_text(sensor_label, "Sensor PNP");
    lv_obj_align_to(sensor_label, led, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // 🔥 botón relay
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 150, 60);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 60);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "RELAY OFF");
    lv_obj_center(label);

    // 🔥 evento
    lv_obj_add_event_cb(btn, relay_btn_event_cb, LV_EVENT_CLICKED, led);
}