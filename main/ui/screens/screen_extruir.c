#include "screen_extruir.h"
#include "logic/extrusion.h"
#include <stdio.h>
#include "logic/active_profile.h"
#include <string.h>
#include "logic/production.h"
#include "lvgl.h"
#include "logic/profile.h"
#include <stdlib.h>
#include "ui/fonts/fonts.h"

// Rango máximo del velocímetro en m/min
#define GAUGE_SIZE      240
#define GAUGE_X_OFS   (-190)
#define CTRL_X_OFS      160
#define SPEED_MAX        20

static lv_obj_t *root;

// ─── Velocímetro ──────────────────────────────────────────────
static lv_obj_t            *scale_speed;
static lv_obj_t            *needle_current;
static lv_scale_section_t  *section_target;
static lv_scale_section_t  *section_redzone;
static lv_style_t           style_needle;
static lv_style_t           style_section_target_arc;
static lv_style_t           style_section_target_tick;
static lv_style_t           style_section_red_arc;
static lv_style_t           style_section_red_tick;

// ─── Velocidad (texto) ────────────────────────────────────────
static lv_obj_t *label_speed;
static lv_obj_t *label_target_speed;

// ─── Grabar ───────────────────────────────────────────────────
static lv_obj_t *btn_record;
static lv_obj_t *label_btn;

// ─── Corte ────────────────────────────────────────────────────
static lv_obj_t *label_section_cut;
static lv_obj_t *cut_container;
static lv_obj_t *cut_buttons[MAX_CUT_OPTIONS];

// ─── Cantidad objetivo ────────────────────────────────────────
static lv_obj_t *label_section_qty;
static lv_obj_t *qty_container;
static lv_obj_t *btn_qty_minus;
static lv_obj_t *btn_qty_plus;
static lv_obj_t *label_qty;

static bool      recording_ui  = false;
static bool      auto_finished = false;
static profile_t current_profile;
static int       target_qty_ui = 25;
static float     display_speed = 0.0f; // velocidad suavizada para la aguja

// =========================
// MODAL CLOSE
// =========================
static void modal_close_cb(lv_event_t *e)
{
    lv_obj_t *btn     = lv_event_get_target(e);
    lv_obj_t *modal   = lv_obj_get_parent(btn);
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
    lv_obj_set_size(modal, 320, LV_SIZE_CONTENT);
    lv_obj_center(modal);
    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(modal, 16, 0);
    lv_obj_set_style_pad_gap(modal, 12, 0);

    lv_obj_t *label = lv_label_create(modal);
    lv_label_set_text(label, msg);
    lv_obj_set_style_text_font(label, FONT_SMALL, 0);

    lv_obj_t *btn = lv_btn_create(modal);
    lv_obj_set_width(btn, LV_PCT(100));
    lv_obj_add_event_cb(btn, modal_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, "OK");
    lv_obj_set_style_text_font(lbl, FONT_MEDIUM, 0);
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
            lv_obj_clear_state(cut_buttons[i], LV_STATE_DISABLED);
        else
            lv_obj_add_state(cut_buttons[i], LV_STATE_DISABLED);
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
// QTY BUTTON EVENTS
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
// BOTÓN GRABAR
// =========================
static void btn_event_cb(lv_event_t *e)
{
    if (!recording_ui)
    {
        const char *profile = active_profile_get();
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
    lv_obj_t *btn   = lv_event_get_target(e);
    float     cut_m = (float)(uintptr_t)lv_event_get_user_data(e);

    extrusion_set_cut_distance_m(cut_m);

    for (int i = 0; i < current_profile.cut_options_count; i++)
    {
        lv_color_t color = (cut_buttons[i] == btn)
            ? lv_palette_main(LV_PALETTE_BLUE)
            : lv_palette_main(LV_PALETTE_GREY);
        lv_obj_set_style_bg_color(cut_buttons[i], color, 0);
    }

    printf("Nueva distancia seleccionada: %.2f m\n", cut_m);
}

// =========================
// HELPERS UI
// =========================
static lv_obj_t *make_flex_row(lv_obj_t *parent, int w, int h)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, w, h);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(cont, 4, 0);
    lv_obj_set_style_pad_gap(cont, 12, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_shadow_width(cont, 0, 0);
    return cont;
}

static lv_obj_t *make_section_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl, lv_palette_main(LV_PALETTE_GREY), 0);
    return lbl;
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

    // ─── Velocímetro ──────────────────────────────────────────

    // estilo aguja velocidad actual
    lv_style_init(&style_needle);
    lv_style_set_line_width(&style_needle, 4);
    lv_style_set_line_color(&style_needle, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_line_rounded(&style_needle, true);

    // estilo sección objetivo → arco naranja
    lv_style_init(&style_section_target_arc);
    lv_style_set_arc_color(&style_section_target_arc, lv_color_hex(0xFF8C00));
    lv_style_set_arc_width(&style_section_target_arc, 8);

    lv_style_init(&style_section_target_tick);
    lv_style_set_line_color(&style_section_target_tick, lv_color_hex(0xFF8C00));
    lv_style_set_line_width(&style_section_target_tick, 3);
    lv_style_set_text_color(&style_section_target_tick, lv_color_hex(0xFF8C00));

    // estilo sección zona roja (sobrevelocidad)
    lv_style_init(&style_section_red_arc);
    lv_style_set_arc_color(&style_section_red_arc, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_arc_width(&style_section_red_arc, 8);

    lv_style_init(&style_section_red_tick);
    lv_style_set_line_color(&style_section_red_tick, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_line_width(&style_section_red_tick, 3);
    lv_style_set_text_color(&style_section_red_tick, lv_palette_main(LV_PALETTE_RED));

    // escala (velocímetro)
    scale_speed = lv_scale_create(root);
    lv_obj_set_size(scale_speed, GAUGE_SIZE, GAUGE_SIZE);
    lv_scale_set_mode(scale_speed, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_range(scale_speed, 0, SPEED_MAX);
    lv_scale_set_angle_range(scale_speed, 270);
    lv_scale_set_rotation(scale_speed, 135);
    lv_scale_set_total_tick_count(scale_speed, 41); // un tick cada 0.5 m/min
    lv_scale_set_major_tick_every(scale_speed, 10); // label cada 5 m/min
    lv_scale_set_label_show(scale_speed, true);
    lv_obj_set_style_length(scale_speed, 5,  LV_PART_ITEMS);
    lv_obj_set_style_length(scale_speed, 12, LV_PART_INDICATOR);
    lv_obj_align(scale_speed, LV_ALIGN_CENTER, GAUGE_X_OFS, -10);

    // sección zona roja (estática, últimos 2 m/min)
    section_redzone = lv_scale_add_section(scale_speed);
    lv_scale_section_set_range(section_redzone, SPEED_MAX - 2, SPEED_MAX);
    lv_scale_section_set_style(section_redzone, LV_PART_MAIN,      &style_section_red_arc);
    lv_scale_section_set_style(section_redzone, LV_PART_INDICATOR, &style_section_red_tick);
    lv_scale_section_set_style(section_redzone, LV_PART_ITEMS,     &style_section_red_tick);

    // sección objetivo (se actualiza al cargar perfil)
    section_target = lv_scale_add_section(scale_speed);
    lv_scale_section_set_range(section_target, 0, 0);
    lv_scale_section_set_style(section_target, LV_PART_MAIN,      &style_section_target_arc);
    lv_scale_section_set_style(section_target, LV_PART_INDICATOR, &style_section_target_tick);
    lv_scale_section_set_style(section_target, LV_PART_ITEMS,     &style_section_target_tick);

    // aguja velocidad actual
    needle_current = lv_line_create(scale_speed);
    lv_obj_add_style(needle_current, &style_needle, 0);
    lv_scale_set_line_needle_value(scale_speed, needle_current, 95, 0);

    // etiqueta velocidad actual (debajo del gauge)
    label_speed = lv_label_create(root);
    lv_label_set_text(label_speed, "0.00 m/min");
    lv_obj_set_style_text_font(label_speed, FONT_LARGE, 0);
    lv_obj_align(label_speed, LV_ALIGN_CENTER, GAUGE_X_OFS, 148);

    // etiqueta velocidad objetivo
    label_target_speed = lv_label_create(root);
    lv_label_set_text(label_target_speed, "Objetivo: --");
    lv_obj_set_style_text_font(label_target_speed, FONT_SMALL, 0);
    lv_obj_set_style_text_color(label_target_speed, lv_color_hex(0xFF8C00), 0);
    lv_obj_align(label_target_speed, LV_ALIGN_CENTER, GAUGE_X_OFS, 185);

    // ─── Sección Corte ────────────────────────────────────────
    label_section_cut = make_section_label(root, "Corte:");
    lv_obj_align(label_section_cut, LV_ALIGN_CENTER, CTRL_X_OFS, -150);

    cut_container = make_flex_row(root, 380, 65);
    lv_obj_align(cut_container, LV_ALIGN_CENTER, CTRL_X_OFS, -108);

    // ─── Sección Cantidad objetivo ────────────────────────────
    label_section_qty = make_section_label(root, "Cantidad objetivo:");
    lv_obj_align(label_section_qty, LV_ALIGN_CENTER, CTRL_X_OFS, -35);

    qty_container = make_flex_row(root, 340, 65);
    lv_obj_align(qty_container, LV_ALIGN_CENTER, CTRL_X_OFS, 10);

    // botón −
    btn_qty_minus = lv_btn_create(qty_container);
    lv_obj_set_size(btn_qty_minus, 65, 50);
    lv_obj_add_event_cb(btn_qty_minus, qty_minus_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_qty_minus, qty_minus_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);

    lv_obj_t *lbl_minus = lv_label_create(btn_qty_minus);
    lv_label_set_text(lbl_minus, "-");
    lv_obj_set_style_text_font(lbl_minus, FONT_MEDIUM, 0);
    lv_obj_center(lbl_minus);

    // valor cantidad
    label_qty = lv_label_create(qty_container);
    lv_obj_set_width(label_qty, 150);
    lv_obj_set_style_text_align(label_qty, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_qty, FONT_MEDIUM, 0);
    lv_label_set_text(label_qty, "25 perfiles");

    // botón +
    btn_qty_plus = lv_btn_create(qty_container);
    lv_obj_set_size(btn_qty_plus, 65, 50);
    lv_obj_add_event_cb(btn_qty_plus, qty_plus_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_qty_plus, qty_plus_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);

    lv_obj_t *lbl_plus = lv_label_create(btn_qty_plus);
    lv_label_set_text(lbl_plus, "+");
    lv_obj_set_style_text_font(lbl_plus, FONT_MEDIUM, 0);
    lv_obj_center(lbl_plus);

    update_qty_label();

    // ─── Botón Grabar ─────────────────────────────────────────
    btn_record = lv_btn_create(root);
    lv_obj_set_size(btn_record, 220, 60);
    lv_obj_set_style_bg_color(btn_record, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align(btn_record, LV_ALIGN_CENTER, CTRL_X_OFS, 110);

    label_btn = lv_label_create(btn_record);
    lv_label_set_text(label_btn, "Grabar");
    lv_obj_set_style_text_font(label_btn, FONT_MEDIUM, 0);
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

        // ocultar marcador objetivo: rango de anchura cero fuera del arco
        lv_scale_section_set_range(section_target, 0, 0);

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

    // actualizar marcador objetivo en el arco (banda de ±1 m/min redondeada)
    int32_t tgt = (int32_t)(current_profile.belt_speed + 0.5f);
    lv_scale_section_set_range(section_target,
                               LV_MAX(0, tgt - 1),
                               LV_MIN(SPEED_MAX, tgt + 1));

    char speed_buf[64];
    snprintf(speed_buf, sizeof(speed_buf),
             "Objetivo: %.2f m/min", current_profile.belt_speed);
    lv_label_set_text(label_target_speed, speed_buf);

    printf("Opciones de corte: %d\n", current_profile.cut_options_count);

    if (!cut_container)
    {
        printf("ERROR: cut_container no creado\n");
        return;
    }
    lv_obj_clean(cut_container);

    for (int i = 0; i < current_profile.cut_options_count; i++)
    {
        float cut_m = current_profile.cut_options[i];
        printf("Creando boton %.2f m\n", cut_m);

        cut_buttons[i] = lv_btn_create(cut_container);
        lv_obj_set_size(cut_buttons[i], 100, 55);

        lv_color_t color = (cut_m == current_profile.default_cut)
            ? lv_palette_main(LV_PALETTE_BLUE)
            : lv_palette_main(LV_PALETTE_GREY);
        lv_obj_set_style_bg_color(cut_buttons[i], color, 0);

        lv_obj_add_event_cb(cut_buttons[i], cut_btn_event_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)((int)cut_m));

        char txt[32];
        snprintf(txt, sizeof(txt), "%.0f m", cut_m);

        lv_obj_t *label = lv_label_create(cut_buttons[i]);
        lv_label_set_text(label, txt);
        lv_obj_set_style_text_font(label, FONT_SMALL, 0);
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
    float raw_speed = extrusion_get_speed_m_min();

    // EMA asimétrico: aceleración más rápida que desaceleración
    // Simula la inercia mecánica del sistema (correa + rodillo)
    float alpha = (raw_speed > display_speed) ? 0.25f : 0.10f;
    display_speed = alpha * raw_speed + (1.0f - alpha) * display_speed;

    // Snap a cero para evitar que la aguja quede vibrando cerca de 0
    if (display_speed < 0.05f)
        display_speed = 0.0f;

    // Etiqueta: velocidad real (exacta)
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f m/min", raw_speed);
    lv_label_set_text(label_speed, buf);

    // Aguja: velocidad suavizada
    lv_scale_set_line_needle_value(scale_speed, needle_current, 95,
                                   (int32_t)(display_speed + 0.5f));

    if (extrusion_is_target_reached() && !auto_finished)
    {
        auto_finished = true;
        production_finish(PRODUCTION_FINISH_TARGET);
        show_error_modal("Lote completado");

        set_cut_buttons_enabled(true);
        set_qty_buttons_enabled(true);

        recording_ui = false;
        lv_label_set_text(label_btn, "Grabar");
        lv_obj_set_style_bg_color(btn_record, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
}
