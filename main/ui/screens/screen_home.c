#include "lvgl.h"
#include <stdio.h>
#include "drivers/relay.h"
#include "CH422G.h"
#include "drivers/rtc/rtc.h"

static bool running = false;

static int pulse_count = 0;
static int total_count = 0;

static bool last_sensor_state = false;

static lv_obj_t *label_counter;
static lv_obj_t *label_total;
static lv_obj_t *btn_label;

// 🔥 CONTROL RELAY
static bool relay_active = false;
static uint32_t relay_start_time = 0;

// =========================
// BOTÓN START/STOP
// =========================
static void start_stop_cb(lv_event_t *e)
{
    running = !running;

    char time_str[16];
    rtc_get_time_string(time_str);

    if (running)
    {
        printf("START presionado a las %s\n", time_str);

        lv_label_set_text(btn_label, "STOP");
        lv_obj_set_style_bg_color(lv_event_get_target(e),
            lv_palette_main(LV_PALETTE_RED), 0);
    }
    else
    {
        printf("STOP presionado a las %s\n", time_str);

        lv_label_set_text(btn_label, "START");
        lv_obj_set_style_bg_color(lv_event_get_target(e),
            lv_palette_main(LV_PALETTE_GREEN), 0);
    }
}

// =========================
// TIMER PRINCIPAL
// =========================
static void process_timer_cb(lv_timer_t *t)
{
    uint8_t di = CH422G_io_input(0x01); // DI0
    bool current_state = !di;

    // =========================
    // DETECCIÓN DE FLANCO
    // =========================
    if (running && current_state && !last_sensor_state)
    {
        pulse_count++;

        lv_label_set_text_fmt(label_counter, "COUNT: %d", pulse_count);

        // =========================
        // ACTIVAR RELAY CADA 5
        // =========================
        if (pulse_count % 5 == 0)
        {
            total_count++;

            lv_label_set_text_fmt(label_total, "RELAY: %d", total_count);

            relay_set(true);
            relay_active = true;
            relay_start_time = lv_tick_get();
        }
    }

    last_sensor_state = current_state;

    // =========================
    // APAGAR RELAY (1000 ms)
    // =========================
    if (relay_active)
    {
        if (lv_tick_get() - relay_start_time >= 1000)
        {
            relay_set(false);
            relay_active = false;
        }
    }
}

// =========================
// UI
// =========================
void screen_home_create(void)
{
    lv_obj_t *scr = lv_scr_act();

    // =========================
    // BOTÓN START/STOP
    // =========================
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 150, 70);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 40);

    btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "START");
    lv_obj_center(btn_label);

    lv_obj_set_style_bg_color(btn,
        lv_palette_main(LV_PALETTE_GREEN), 0);

    lv_obj_add_event_cb(btn, start_stop_cb, LV_EVENT_CLICKED, NULL);

    // =========================
    // CONTADOR 1
    // =========================
    label_counter = lv_label_create(scr);
    lv_label_set_text(label_counter, "COUNT: 0");
    lv_obj_align(label_counter, LV_ALIGN_CENTER, 0, -20);

    // =========================
    // CONTADOR 2
    // =========================
    label_total = lv_label_create(scr);
    lv_label_set_text(label_total, "RELAY: 0");
    lv_obj_align(label_total, LV_ALIGN_CENTER, 0, 40);

    // =========================
    // TIMER PRINCIPAL
    // =========================
    lv_timer_create(process_timer_cb, 50, NULL);
}