#include "lvgl.h"
#include <stdio.h>
#include "drivers/relay.h"
#include "CH422G.h"

static bool relay_state = false;

// 🔥 forward declaration
static void sensor_timer_cb(lv_timer_t *t);

// 🔥 callback botón
static void relay_btn_event_cb(lv_event_t *e)
{
    printf("TOUCH DETECTED\n");

    lv_obj_t *btn = lv_event_get_target(e);

    relay_set(!relay_get());
    relay_state = relay_get();

    lv_obj_t *label = lv_obj_get_child(btn, 0);
    lv_label_set_text(label, relay_state ? "RELAY ON" : "RELAY OFF");

    lv_obj_set_style_bg_color(btn,
                              relay_state ? lv_palette_main(LV_PALETTE_GREEN)
                                          : lv_palette_main(LV_PALETTE_RED),
                              0);
}

void screen_home_create(void)
{
    lv_obj_t *scr = lv_scr_act();

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Integral Controller");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *led = lv_led_create(scr);
    lv_obj_set_size(led, 30, 30);
    lv_obj_align(led, LV_ALIGN_CENTER, 0, -40);

    // 🔥 estado inicial
    lv_led_off(led);
    lv_obj_set_style_bg_color(led, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_led_off(led);

    lv_obj_t *sensor_label = lv_label_create(scr);
    lv_label_set_text(sensor_label, "Sensor PNP");
    lv_obj_align_to(sensor_label, led, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 150, 60);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 60);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "RELAY OFF");
    lv_obj_center(label);

    lv_obj_add_event_cb(btn, relay_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // 🔥 sensor controla LED
    lv_timer_create(sensor_timer_cb, 100, led);
}

static void sensor_timer_cb(lv_timer_t *t)
{
    lv_obj_t *led = (lv_obj_t *)t->user_data;

    static bool last_state = false;

    uint8_t di = CH422G_io_input(0x01); // DI0
     bool current_state = !di;

    // 🔥 actualizar LED
    if (current_state)
    {
        lv_led_on(led);
        lv_obj_set_style_bg_color(led, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
    else
    {
        lv_led_off(led);
        lv_obj_set_style_bg_color(led, lv_palette_main(LV_PALETTE_GREY), 0);
    }

    // 🔥 imprimir solo si cambia
    if (current_state != last_state)
    {
        if (current_state)
            printf("SENSOR DETECTADO\n");
        else
            printf("SENSOR LIBRE\n");

        last_state = current_state;
    }
}