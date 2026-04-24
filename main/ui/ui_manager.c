#include "lvgl.h"
#include "ui_manager.h"
#include "logic/extrusion.h"
#include "storage/sdcard/sd_files.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *label_speed;

static lv_obj_t *btn_record;
static lv_obj_t *label_btn;

static lv_obj_t *list_files;
static lv_obj_t *text_area;
static lv_obj_t *btn_refresh;

static bool recording_ui = false;

// 🔥 DECLARACIÓN
static void load_sd_files(void);

// =========================
// EVENTO BOTÓN GRABAR
// =========================
static void btn_event_cb(lv_event_t *e)
{
    printf("BOTON PRESIONADO\n");

    if (!recording_ui)
    {
        printf("START RECORDING\n");

        recording_start();
        recording_ui = true;

        lv_label_set_text(label_btn, "Detener");
        lv_obj_set_style_bg_color(btn_record,
                                  lv_palette_main(LV_PALETTE_RED), 0);
    }
    else
    {
        printf("STOP RECORDING\n");

        recording_stop();
        recording_ui = false;

        lv_label_set_text(label_btn, "Grabar");
        lv_obj_set_style_bg_color(btn_record,
                                  lv_palette_main(LV_PALETTE_GREEN), 0);
        // 🔥 REFRESH AUTOMÁTICO
        load_sd_files();
    }
}

// =========================
// CLICK ARCHIVO
// =========================
static void file_click_event(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    const char *filename = lv_list_get_btn_text(list_files, btn);

    char buffer[512];

    int len = sd_read_last_chunk(filename, buffer, sizeof(buffer));

    if (len > 0)
    {
        lv_textarea_set_text(text_area, buffer);

        // 🔥 SCROLL AL FINAL
        lv_textarea_set_cursor_pos(text_area, LV_TEXTAREA_CURSOR_LAST);
        lv_textarea_set_cursor_pos(text_area, LV_TEXTAREA_CURSOR_LAST);
    }
    else
    {
        lv_textarea_set_text(text_area, "Error leyendo archivo");
    }
}

// =========================
// CARGAR ARCHIVOS SD
// =========================
static void load_sd_files(void)
{
    lv_obj_clean(list_files);

    char files[20][64];
    int count = sd_list_files(files, 20);

    if (count <= 0)
    {
        lv_list_add_text(list_files, "No files");
        return;
    }

    for (int i = 0; i < count; i++)
    {
        lv_obj_t *btn = lv_list_add_btn(list_files, NULL, files[i]);
        lv_obj_add_event_cb(btn, file_click_event, LV_EVENT_CLICKED, NULL);
    }
}

// =========================
// REFRESH BUTTON
// =========================
static void refresh_event_cb(lv_event_t *e)
{
    load_sd_files();
}

// =========================
// UI INIT
// =========================
void ui_start(void)
{
    lv_obj_t *scr = lv_scr_act();

    // =========================
    // VELOCIDAD
    // =========================
    label_speed = lv_label_create(scr);
    lv_label_set_text(label_speed, "0.0 m/min");
    lv_obj_align(label_speed, LV_ALIGN_TOP_LEFT, 10, 10);  

    // =========================
    // BOTÓN GRABAR
    // =========================
    btn_record = lv_btn_create(scr);
    lv_obj_set_size(btn_record, 140, 60);
    lv_obj_align(btn_record, LV_ALIGN_TOP_LEFT, 10, 50);

    lv_obj_set_style_bg_color(btn_record,
                              lv_palette_main(LV_PALETTE_GREEN), 0);

    label_btn = lv_label_create(btn_record);
    lv_label_set_text(label_btn, "Grabar");
    lv_obj_center(label_btn);

    lv_obj_add_event_cb(btn_record, btn_event_cb, LV_EVENT_CLICKED, NULL);

    // =========================
    // LISTA ARCHIVOS
    // =========================
    list_files = lv_list_create(scr);
    lv_obj_set_size(list_files, 200, 200);
    lv_obj_align(list_files, LV_ALIGN_TOP_RIGHT, -10, 10);

    // =========================
    // BOTÓN REFRESH
    // =========================
    btn_refresh = lv_btn_create(scr);
    lv_obj_set_size(btn_refresh, 100, 40);
    lv_obj_align(btn_refresh, LV_ALIGN_TOP_RIGHT, -10, 220);

    lv_obj_t *label_refresh = lv_label_create(btn_refresh);
    lv_label_set_text(label_refresh, "Refresh");
    lv_obj_center(label_refresh);

    lv_obj_add_event_cb(btn_refresh, refresh_event_cb, LV_EVENT_CLICKED, NULL);

    // =========================
    // TEXT AREA
    // =========================
    text_area = lv_textarea_create(scr);
    lv_obj_set_size(text_area, 460, 140);
    lv_obj_align(text_area, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_textarea_set_placeholder_text(text_area, "Contenido del archivo...");

    // =========================
    // CARGAR ARCHIVOS
    // =========================
    load_sd_files();
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