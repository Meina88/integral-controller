#include "screen_extruir.h"
#include "logic/extrusion.h"
#include "storage/sdcard/sd_files.h"
#include <stdio.h>

static lv_obj_t *root;
static lv_obj_t *label_speed;

static lv_obj_t *btn_record;
static lv_obj_t *label_btn;

static lv_obj_t *list_files;
static lv_obj_t *text_area;

static bool recording_ui = false;

// =========================
// BOTÓN
// =========================
static void btn_event_cb(lv_event_t *e)
{
    if (!recording_ui)
    {
        recording_start();
        recording_ui = true;

        lv_label_set_text(label_btn, "Detener");
        lv_obj_set_style_bg_color(btn_record, lv_palette_main(LV_PALETTE_RED), 0);
    }
    else
    {
        recording_stop();
        recording_ui = false;

        lv_label_set_text(label_btn, "Grabar");
        lv_obj_set_style_bg_color(btn_record, lv_palette_main(LV_PALETTE_GREEN), 0);
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
        lv_textarea_set_cursor_pos(text_area, LV_TEXTAREA_CURSOR_LAST);
    }
}

// =========================
// LOAD FILES
// =========================
static void load_files(void)
{
    lv_obj_clean(list_files);

    char files[20][64];
    int count = sd_list_files(files, 20);

    for (int i = 0; i < count; i++)
    {
        lv_obj_t *btn = lv_list_add_btn(list_files, NULL, files[i]);
        lv_obj_add_event_cb(btn, file_click_event, LV_EVENT_CLICKED, NULL);
    }
}

// =========================
// CREATE
// =========================
lv_obj_t *screen_extruir_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));

    // velocidad
    label_speed = lv_label_create(root);
    lv_obj_align(label_speed, LV_ALIGN_TOP_LEFT, 20, 20);

    // botón grabar
    btn_record = lv_btn_create(root);
    lv_obj_set_size(btn_record, 140, 60);
    lv_obj_align(btn_record, LV_ALIGN_TOP_LEFT, 20, 60);

    label_btn = lv_label_create(btn_record);
    lv_label_set_text(label_btn, "Grabar");
    lv_obj_center(label_btn);

    lv_obj_add_event_cb(btn_record, btn_event_cb, LV_EVENT_CLICKED, NULL);

    // lista archivos
    list_files = lv_list_create(root);
    lv_obj_set_size(list_files, 200, 200);
    lv_obj_align(list_files, LV_ALIGN_TOP_RIGHT, -20, 20);

    // textarea
    text_area = lv_textarea_create(root);
    lv_obj_set_size(text_area, 460, 140);
    lv_obj_align(text_area, LV_ALIGN_BOTTOM_MID, 0, -20);

    load_files();

    return root;
}

// =========================
// UPDATE
// =========================
void screen_extruir_update(void)
{
    char buf[32];
    float speed = extrusion_get_speed_m_min();

    snprintf(buf, sizeof(buf), "%.2f m/min", speed);
    lv_label_set_text(label_speed, buf);
}