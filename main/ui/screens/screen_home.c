#include "lvgl.h"
#include "logic/extrusion.h"
#include "storage/sdcard/sd_files.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =========================
// UI ELEMENTOS
// =========================
static lv_obj_t *label_counter;
static lv_obj_t *label_total;
static lv_obj_t *btn_label;
static lv_obj_t *txt_area;

// =========================
// START / STOP
// =========================
static void start_stop_cb(lv_event_t *e)
{
    if (extrusion_is_running())
    {
        extrusion_stop();

        lv_label_set_text(btn_label, "START");
        lv_obj_set_style_bg_color(lv_event_get_target(e),
                                  lv_palette_main(LV_PALETTE_GREEN), 0);
    }
    else
    {
        extrusion_start();

        lv_label_set_text(btn_label, "STOP");
        lv_obj_set_style_bg_color(lv_event_get_target(e),
                                  lv_palette_main(LV_PALETTE_RED), 0);
    }
}

// =========================
// CLICK ARCHIVO
// =========================
static void file_click_cb(lv_event_t *e)
{
    const char *filename = (const char *)lv_event_get_user_data(e);

    char buffer[512];

    int res = sd_read_file(filename, buffer, sizeof(buffer));

    if (res < 0)
    {
        lv_textarea_set_text(txt_area, "ERROR ABRIENDO ARCHIVO");
        return;
    }

    lv_textarea_set_text(txt_area, buffer);
}

// =========================
// TIMER UI
// =========================
static void ui_timer_cb(lv_timer_t *t)
{
    (void)t;

    // proceso real
    extrusion_process_tick();

    // actualizar UI
    lv_label_set_text_fmt(label_counter, "COUNT: %d",
                          extrusion_get_pulse_count());

    lv_label_set_text_fmt(label_total, "RELAY: %d",
                          extrusion_get_total_count());
}

// =========================
// CREAR PANTALLA
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
    // CONTADORES
    // =========================
    label_counter = lv_label_create(scr);
    lv_label_set_text(label_counter, "COUNT: 0");
    lv_obj_align(label_counter, LV_ALIGN_CENTER, 0, -20);

    label_total = lv_label_create(scr);
    lv_label_set_text(label_total, "RELAY: 0");
    lv_obj_align(label_total, LV_ALIGN_CENTER, 0, 40);

    // =========================
    // LISTA ARCHIVOS
    // =========================
    lv_obj_t *list = lv_list_create(scr);
    lv_obj_set_size(list, 400, 200);
    lv_obj_align(list, LV_ALIGN_BOTTOM_LEFT, 0, -10);

    char files[20][64];
    int count = sd_list_files(files, 20);

    if (count <= 0)
    {
        lv_list_add_text(list, "SD vacia / error");
    }
    else
    {
        for (int i = 0; i < count; i++)
        {
            lv_obj_t *btn_file = lv_list_add_btn(list, NULL, files[i]);

            char *name_copy = malloc(strlen(files[i]) + 1);
            if (name_copy)
            {
                strcpy(name_copy, files[i]);
            }

            lv_obj_add_event_cb(btn_file, file_click_cb,
                                LV_EVENT_CLICKED, name_copy);
        }
    }

    // =========================
    // TEXT AREA
    // =========================
    txt_area = lv_textarea_create(scr);
    lv_obj_set_size(txt_area, 400, 120);
    lv_obj_align(txt_area, LV_ALIGN_BOTTOM_RIGHT, 0, -10);
    lv_textarea_set_text(txt_area, "Selecciona archivo...");

    // =========================
    // TIMER
    // =========================
    lv_timer_create(ui_timer_cb, 50, NULL);
}