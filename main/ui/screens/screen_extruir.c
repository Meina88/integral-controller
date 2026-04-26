#include "screen_extruir.h"
#include "logic/extrusion.h"
#include <stdio.h>
#include "logic/active_profile.h"
#include <string.h>
#include "logic/production.h"

static lv_obj_t *root;
static lv_obj_t *label_speed;

static lv_obj_t *btn_record;
static lv_obj_t *label_btn;

static bool recording_ui = false;

// =========================
// BOTÓN
// =========================
static void btn_event_cb(lv_event_t *e)
{
    if (!recording_ui)
    {
        const char *profile = active_profile_get();

        // 🔥 VALIDACIÓN
        if (!profile || strlen(profile) == 0)
        {
            printf("ERROR: No hay perfil seleccionado\n");
            return;
        }

        production_start();

        recording_ui = true;
        lv_label_set_text(label_btn, "Detener");
        lv_obj_set_style_bg_color(btn_record, lv_palette_main(LV_PALETTE_RED), 0);
    }
    else
    {
        production_stop();

        recording_ui = false;
        lv_label_set_text(label_btn, "Grabar");
        lv_obj_set_style_bg_color(btn_record, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
}

// =========================
// CREATE
// =========================
lv_obj_t *screen_extruir_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));

    // 🔥 eliminar bordes y padding innecesario
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);

    // =========================
    // VELOCIDAD
    // =========================
    label_speed = lv_label_create(root);
    lv_label_set_text(label_speed, "0.00 m/min");
    lv_obj_set_style_text_font(label_speed, &lv_font_montserrat_26, 0);
    lv_obj_align(label_speed, LV_ALIGN_CENTER, 0, -40);

    // =========================
    // BOTÓN GRABAR
    // =========================
    btn_record = lv_btn_create(root);
    lv_obj_set_size(btn_record, 180, 70);
    lv_obj_align(btn_record, LV_ALIGN_CENTER, 0, 40);

    label_btn = lv_label_create(btn_record);
    lv_label_set_text(label_btn, "Grabar");
    lv_obj_center(label_btn);

    lv_obj_add_event_cb(btn_record, btn_event_cb, LV_EVENT_CLICKED, NULL);

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