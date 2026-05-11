#include "screen_extruir.h"
#include "logic/extrusion.h"
#include <stdio.h>
#include "logic/active_profile.h"
#include <string.h>
#include "logic/production.h"
#include "lvgl.h"
#include "logic/profile.h"
#include <stdlib.h>

static lv_obj_t *root;
static lv_obj_t *label_speed;
static lv_obj_t *label_target_speed;

static lv_obj_t *btn_record;
static lv_obj_t *label_btn;

static bool recording_ui = false;
static bool auto_finished = false;
static profile_t current_profile;
static lv_obj_t *cut_container;
static lv_obj_t *cut_buttons[MAX_CUT_OPTIONS];

// =========================
// TARGET QTY
// =========================
static lv_obj_t *qty_container;
static lv_obj_t *btn_qty_minus;
static lv_obj_t *btn_qty_plus;
static lv_obj_t *label_qty;

static int target_qty_ui = 25;

// =========================
// MODAL CLOSE
// =========================
static void modal_close_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *modal = lv_obj_get_parent(btn);
    lv_obj_t *overlay = lv_obj_get_parent(modal);
    lv_obj_del(overlay);
}

// =========================
// MODAL ERROR
// =========================
static void show_error_modal(const char *msg)
{
    lv_obj_t *overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);

    lv_obj_t *modal = lv_obj_create(overlay);
    lv_obj_set_size(modal, 260, LV_SIZE_CONTENT);
    lv_obj_center(modal);

    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(modal, 10, 0);
    lv_obj_set_style_pad_gap(modal, 10, 0);

    // mensaje
    lv_obj_t *label = lv_label_create(modal);
    lv_label_set_text(label, msg);

    // botón OK
    lv_obj_t *btn = lv_btn_create(modal);
    lv_obj_set_width(btn, LV_PCT(100));
    lv_obj_add_event_cb(btn, modal_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, "OK");
    lv_obj_center(lbl);
}

// =========================
// ENABLE/DISABLE CUT BUTTONS
// =========================
static void set_cut_buttons_enabled(bool enabled)
{
    for (int i = 0; i < current_profile.cut_options_count; i++)
    {
        if (!cut_buttons[i])
            continue;

        if (enabled)
        {
            lv_obj_clear_state(cut_buttons[i], LV_STATE_DISABLED);
        }
        else
        {
            lv_obj_add_state(cut_buttons[i], LV_STATE_DISABLED);
        }
    }
}

// =========================
// UPDATE QTY LABEL
// =========================
static void update_qty_label(void)
{
    char buf[32];

    snprintf(buf, sizeof(buf), "%d perfiles", target_qty_ui);

    lv_label_set_text(label_qty, buf);

    extrusion_set_target_count(target_qty_ui);
}

// =========================
// ENABLE/DISABLE QTY BUTTONS
// =========================
static void set_qty_buttons_enabled(bool enabled)
{
    if (!btn_qty_minus || !btn_qty_plus)
        return;

    if (enabled)
    {
        lv_obj_clear_state(btn_qty_minus, LV_STATE_DISABLED);
        lv_obj_clear_state(btn_qty_plus, LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_add_state(btn_qty_minus, LV_STATE_DISABLED);
        lv_obj_add_state(btn_qty_plus, LV_STATE_DISABLED);
    }
}

// =========================
// QTY BUTTON EVENT
// =========================
static void qty_minus_event_cb(lv_event_t *e)
{
    if (target_qty_ui > 1)
    {
        target_qty_ui--;
        update_qty_label();
    }
}

static void qty_plus_event_cb(lv_event_t *e)
{
    if (target_qty_ui < 999)
    {
        target_qty_ui++;
        update_qty_label();
    }
}


// =========================
// BOTÓN
// =========================
static void btn_event_cb(lv_event_t *e)
{
    if (!recording_ui)
    {
        const char *profile = active_profile_get();

        // 🔥 VALIDACIÓN CON MODAL
        if (!profile || strlen(profile) == 0)
        {
            show_error_modal("Seleccione un perfil para grabar");
            return;
        }

        production_start();
        auto_finished = false;

        set_cut_buttons_enabled(false);
        set_qty_buttons_enabled(false);

        recording_ui = true;
        lv_label_set_text(label_btn, "Detener");
        lv_obj_set_style_bg_color(btn_record, lv_palette_main(LV_PALETTE_RED), 0);
    }
    else
    {
        production_finish(PRODUCTION_FINISH_MANUAL);

        set_cut_buttons_enabled(true);
        set_qty_buttons_enabled(true);

        recording_ui = false;
        lv_label_set_text(label_btn, "Grabar");
        lv_obj_set_style_bg_color(btn_record, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
}

// =========================
// CUT BUTTON EVENT
// =========================
static void cut_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);

    float cut_m = (float)(uintptr_t)lv_event_get_user_data(e);

    extrusion_set_cut_distance_m(cut_m);

    // actualizar estilos
    for (int i = 0; i < current_profile.cut_options_count; i++)
    {
        if (cut_buttons[i] == btn)
        {
            lv_obj_set_style_bg_color(
                cut_buttons[i],
                lv_palette_main(LV_PALETTE_BLUE),
                0);
        }
        else
        {
            lv_obj_set_style_bg_color(
                cut_buttons[i],
                lv_palette_main(LV_PALETTE_GREY),
                0);
        }
    }

    printf("Nueva distancia seleccionada: %.2f m\n", cut_m);
}

// =========================
// CREATE
// =========================
lv_obj_t *screen_extruir_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));

    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);

    // =========================
    // VELOCIDAD
    // =========================
    label_speed = lv_label_create(root);
    lv_label_set_text(label_speed, "0.00 m/min");
    lv_obj_set_style_text_font(label_speed, &lv_font_montserrat_26, 0);
    lv_obj_align(label_speed, LV_ALIGN_CENTER, 0, -100);

    // =========================
    // VELOCIDAD OBJETIVO
    // =========================
    label_target_speed = lv_label_create(root);

    lv_label_set_text(label_target_speed, "Objetivo: --");

    lv_obj_set_style_text_font(
        label_target_speed,
        &lv_font_montserrat_16,
        0);

    lv_obj_align(label_target_speed, LV_ALIGN_CENTER, 0, -55);

    // =========================
    // CUT OPTIONS
    // =========================
    cut_container = lv_obj_create(root);

    lv_obj_set_size(cut_container, 320, 70);

    lv_obj_set_flex_flow(cut_container, LV_FLEX_FLOW_ROW);

    lv_obj_set_flex_align(
        cut_container,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);

    lv_obj_set_style_pad_all(cut_container, 4, 0);
    lv_obj_set_style_pad_gap(cut_container, 10, 0);

    // ocultar estética container
    lv_obj_set_style_bg_opa(cut_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cut_container, 0, 0);
    lv_obj_set_style_shadow_width(cut_container, 0, 0);

    lv_obj_align(cut_container, LV_ALIGN_CENTER, 0, -10);

    // =========================
// CANTIDAD OBJETIVO
// =========================
qty_container = lv_obj_create(root);

lv_obj_set_size(qty_container, 320, 70);

lv_obj_set_flex_flow(qty_container, LV_FLEX_FLOW_ROW);

lv_obj_set_flex_align(
    qty_container,
    LV_FLEX_ALIGN_CENTER,
    LV_FLEX_ALIGN_CENTER,
    LV_FLEX_ALIGN_CENTER);

lv_obj_set_style_pad_all(qty_container, 4, 0);
lv_obj_set_style_pad_gap(qty_container, 10, 0);

// ocultar estética container
lv_obj_set_style_bg_opa(qty_container, LV_OPA_TRANSP, 0);
lv_obj_set_style_border_width(qty_container, 0, 0);
lv_obj_set_style_shadow_width(qty_container, 0, 0);

lv_obj_align(qty_container, LV_ALIGN_CENTER, 0, 45);

// botón -
// botón -
btn_qty_minus = lv_btn_create(qty_container);

lv_obj_set_size(btn_qty_minus, 55, 45);

// click normal
lv_obj_add_event_cb(
    btn_qty_minus,
    qty_minus_event_cb,
    LV_EVENT_CLICKED,
    NULL);

// mantener presionado
lv_obj_add_event_cb(
    btn_qty_minus,
    qty_minus_event_cb,
    LV_EVENT_LONG_PRESSED_REPEAT,
    NULL);

lv_obj_t *lbl_minus = lv_label_create(btn_qty_minus);
lv_label_set_text(lbl_minus, "-");
lv_obj_center(lbl_minus);

// label cantidad
label_qty = lv_label_create(qty_container);
lv_obj_set_width(label_qty, 130);
lv_obj_set_style_text_align(label_qty, LV_TEXT_ALIGN_CENTER, 0);
lv_label_set_text(label_qty, "25 perfiles");

// botón +
// botón +
btn_qty_plus = lv_btn_create(qty_container);

lv_obj_set_size(btn_qty_plus, 55, 45);

// click normal
lv_obj_add_event_cb(
    btn_qty_plus,
    qty_plus_event_cb,
    LV_EVENT_CLICKED,
    NULL);

// mantener presionado
lv_obj_add_event_cb(
    btn_qty_plus,
    qty_plus_event_cb,
    LV_EVENT_LONG_PRESSED_REPEAT,
    NULL);

lv_obj_t *lbl_plus = lv_label_create(btn_qty_plus);
lv_label_set_text(lbl_plus, "+");
lv_obj_center(lbl_plus);

update_qty_label();

    // =========================
    // BOTÓN GRABAR
    // =========================
    btn_record = lv_btn_create(root);
    lv_obj_set_size(btn_record, 180, 70);
    lv_obj_align(btn_record, LV_ALIGN_CENTER, 0, 125);

    label_btn = lv_label_create(btn_record);
    lv_label_set_text(label_btn, "Grabar");
    lv_obj_center(label_btn);

    lv_obj_add_event_cb(btn_record, btn_event_cb, LV_EVENT_CLICKED, NULL);

    screen_extruir_refresh_profile();

    return root;
}

// =========================
// REFRESH PROFILE
// =========================
void screen_extruir_refresh_profile(void)
{
    const char *profile_code = active_profile_get();

    if (!profile_code || strlen(profile_code) == 0)
    {
        memset(&current_profile, 0, sizeof(current_profile));

        extrusion_set_cut_distance_m(0);

        lv_label_set_text(label_target_speed, "Objetivo: --");

        if (cut_container)
            lv_obj_clean(cut_container);

        printf("No hay perfil activo\n");
        return;
    }

    if (!profile_get_by_code(profile_code, &current_profile))
    {
        printf("ERROR cargando perfil activo\n");
        return;
    }

    extrusion_set_cut_distance_m(current_profile.default_cut);

    // =========================
    // VELOCIDAD OBJETIVO
    // =========================
    char speed_buf[64];

    snprintf(
        speed_buf,
        sizeof(speed_buf),
        "Objetivo: %.2f m/min",
        current_profile.belt_speed);

    lv_label_set_text(label_target_speed, speed_buf);

    printf("Opciones de corte: %d\n", current_profile.cut_options_count);

    // =========================
    // LIMPIAR BOTONES ANTERIORES
    // =========================

    if (!cut_container)
    {
        printf("ERROR: cut_container no creado\n");
        return;
    }
    lv_obj_clean(cut_container);

    // =========================
    // CREAR BOTONES
    // =========================
    for (int i = 0; i < current_profile.cut_options_count; i++)
    {
        float cut_m = current_profile.cut_options[i];

        printf("Creando boton %.2f m\n", cut_m);

        cut_buttons[i] = lv_btn_create(cut_container);

        lv_obj_set_size(cut_buttons[i], 90, 50);

        // color default
        if (cut_m == current_profile.default_cut)
        {
            lv_obj_set_style_bg_color(
                cut_buttons[i],
                lv_palette_main(LV_PALETTE_BLUE),
                0);
        }
        else
        {
            lv_obj_set_style_bg_color(
                cut_buttons[i],
                lv_palette_main(LV_PALETTE_GREY),
                0);
        }

        lv_obj_add_event_cb(
            cut_buttons[i],
            cut_btn_event_cb,
            LV_EVENT_CLICKED,
            (void *)(uintptr_t)((int)cut_m));

        char txt[32];

        snprintf(txt, sizeof(txt), "%.0f m", cut_m);

        lv_obj_t *label = lv_label_create(cut_buttons[i]);

        lv_label_set_text(label, txt);

        lv_obj_center(label);
    }

    printf("Perfil refrescado: %s\n", current_profile.code);
    printf("Cut default: %.2f m\n", current_profile.default_cut);
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

    // =========================
    // TARGET COMPLETADO
    // =========================
    if (extrusion_is_target_reached() &&
        !auto_finished)
    {
        auto_finished = true;

        production_finish(PRODUCTION_FINISH_TARGET);

        show_error_modal("Lote completado");

        set_cut_buttons_enabled(true);
        set_qty_buttons_enabled(true);

        recording_ui = false;

        lv_label_set_text(label_btn, "Grabar");

        lv_obj_set_style_bg_color(
            btn_record,
            lv_palette_main(LV_PALETTE_GREEN),
            0);
    }
}