#include "screen_setup.h"
#include "lvgl.h"
#include "logic/profile.h"
#include "ui/components/numpad.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// =========================
// OBJETOS
// =========================
static lv_obj_t *root;
static lv_obj_t *ta_search;
static lv_obj_t *list_results;

// campos (solo lectura)
static lv_obj_t *label_profile;
static lv_obj_t *label_matrix;
static lv_obj_t *label_screw;
static lv_obj_t *label_vfd;
static lv_obj_t *label_extrusion;
static lv_obj_t *label_density;
static lv_obj_t *label_length;

// =========================
// FREE USER DATA (🔥 clave)
// =========================
static void free_code_cb(lv_event_t *e)
{
    char *code = (char *)lv_event_get_user_data(e);
    free(code);
}

// =========================
// CALLBACK BOTÓN RESULTADO
// =========================
static void result_btn_cb(lv_event_t *e)
{
    char *code = (char *)lv_event_get_user_data(e);

    profile_t p;

    if (profile_get_by_code(code, &p))
    {
        char buf[64];

        snprintf(buf, sizeof(buf), "Perfil: %s", p.code);
        lv_label_set_text(label_profile, buf);

        snprintf(buf, sizeof(buf), "Matriz: %s", p.matrix);
        lv_label_set_text(label_matrix, buf);

        snprintf(buf, sizeof(buf), "Gusano: %d", p.screw);
        lv_label_set_text(label_screw, buf);

        snprintf(buf, sizeof(buf), "Vel VFD: %d", p.vfd_speed);
        lv_label_set_text(label_vfd, buf);

        snprintf(buf, sizeof(buf), "Vel extrusión: %.2f", p.extrusion_speed);
        lv_label_set_text(label_extrusion, buf);

        snprintf(buf, sizeof(buf), "Densidad: %.2f", p.density);
        lv_label_set_text(label_density, buf);

        snprintf(buf, sizeof(buf), "Longitud: %.2f", p.cut_length);
        lv_label_set_text(label_length, buf);
    }

    // 🚫 NO free acá → lo maneja LV_EVENT_DELETE
}

// =========================
// RENDER RESULTADOS
// =========================
static void show_results(const char results[][32], int count)
{
    lv_obj_clean(list_results);

    for (int i = 0; i < count; i++)
    {
        lv_obj_t *btn = lv_btn_create(list_results);
        lv_obj_set_width(btn, LV_PCT(100));

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, results[i]);
        lv_obj_center(label);

        // 🔥 copiar string correctamente
        char *code_copy = malloc(32);
        strcpy(code_copy, results[i]);

        // click
        lv_obj_add_event_cb(btn, result_btn_cb, LV_EVENT_CLICKED, code_copy);

        // 🔥 liberar cuando LVGL destruya el botón
        lv_obj_add_event_cb(btn, free_code_cb, LV_EVENT_DELETE, code_copy);
    }
}

// =========================
// BUSCAR
// =========================
static void search_profiles(void)
{
    const char *txt = lv_textarea_get_text(ta_search);

    char results[10][32];
    int count = profile_search(txt, results, 10);

    show_results(results, count);
}

// =========================
// CALLBACK NUMPAD OK
// =========================
static void numpad_done_cb(const char *value)
{
    lv_textarea_set_text(ta_search, value);
    search_profiles();
}

// =========================
// CLICK SEARCH
// =========================
static void ta_click_cb(lv_event_t *e)
{
    numpad_open(ta_search, numpad_done_cb);
}

// =========================
// CREATE SCREEN
// =========================
lv_obj_t *screen_setup_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));

    lv_obj_set_style_border_width(root, 0, 0);

    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(root, 10, 0);
    lv_obj_set_style_pad_gap(root, 10, 0);

    // =========================
    // BUSCADOR
    // =========================
    ta_search = lv_textarea_create(root);
    lv_obj_set_width(ta_search, LV_PCT(100));
    lv_textarea_set_placeholder_text(ta_search, "Buscar perfil...");
    lv_obj_add_event_cb(ta_search, ta_click_cb, LV_EVENT_CLICKED, NULL);

    // =========================
    // RESULTADOS
    // =========================
    list_results = lv_obj_create(root);
    lv_obj_set_width(list_results, LV_PCT(100));
    lv_obj_set_height(list_results, 120);

    lv_obj_set_layout(list_results, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list_results, LV_FLEX_FLOW_COLUMN);

    // =========================
    // CAMPOS
    // =========================
    label_profile = lv_label_create(root);
    lv_label_set_text(label_profile, "Perfil:");

    label_matrix = lv_label_create(root);
    lv_label_set_text(label_matrix, "Matriz:");

    label_screw = lv_label_create(root);
    lv_label_set_text(label_screw, "Gusano:");

    label_vfd = lv_label_create(root);
    lv_label_set_text(label_vfd, "Vel VFD:");

    label_extrusion = lv_label_create(root);
    lv_label_set_text(label_extrusion, "Vel extrusión:");

    label_density = lv_label_create(root);
    lv_label_set_text(label_density, "Densidad:");

    label_length = lv_label_create(root);
    lv_label_set_text(label_length, "Longitud:");

    return root;
}