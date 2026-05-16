#include "screen_extruir.h"
#include "logic/extrusion.h"
#include <stdio.h>
#include "logic/active_profile.h"
#include <string.h>
#include "logic/production.h"
#include "lvgl.h"
#include "logic/profile.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "ui/fonts/fonts.h"
#include "ui/ui_theme.h"
#include "drivers/rtc/rtc.h"

#define GAUGE_SIZE 220
#define SPEED_MAX 20

static lv_obj_t *root;

// ─── Velocímetro ──────────────────────────────────────────────
static lv_obj_t *scale_speed;
static lv_obj_t *needle_current;
static lv_scale_section_t *section_target;
static lv_scale_section_t *section_redzone;
static lv_style_t style_needle;
static lv_style_t style_section_target_arc;
static lv_style_t style_section_target_tick;
static lv_style_t style_section_red_arc;
static lv_style_t style_section_red_tick;

// ─── Velocidad (texto) ────────────────────────────────────────
static lv_obj_t *label_speed;
static lv_obj_t *label_target_speed;

// ─── Grabar ───────────────────────────────────────────────────
static lv_obj_t *btn_record;
static lv_obj_t *label_btn;

// ─── Corte ────────────────────────────────────────────────────
static lv_obj_t *cut_container;
static lv_obj_t *cut_buttons[MAX_CUT_OPTIONS];

// ─── Cantidad objetivo ────────────────────────────────────────
static lv_obj_t *btn_qty_minus;
static lv_obj_t *btn_qty_plus;
static lv_obj_t *label_qty;

// ─── Info producción ──────────────────────────────────────────
static lv_obj_t *label_extruded;
static lv_obj_t *label_remaining_time;
static lv_obj_t *label_finish_time;

static bool recording_ui = false;
static bool auto_finished = false;
static profile_t current_profile;
static int target_qty_ui = 25;
static float display_speed = 0.0f;

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
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", target_qty_ui);
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
    lv_obj_t *btn = lv_event_get_target(e);
    float cut_m = (float)(uintptr_t)lv_event_get_user_data(e);

    extrusion_set_cut_distance_m(cut_m);

    for (int i = 0; i < current_profile.cut_options_count; i++)
    {
        lv_color_t color = (cut_buttons[i] == btn)
                               ? ui_theme_get()->blue
                               : ui_theme_get()->btn_grey;
        lv_obj_set_style_bg_color(cut_buttons[i], color, 0);
    }
}

// =========================
// HELPERS UI
// =========================
static lv_obj_t *make_section_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl, ui_theme_get()->muted, 0);
    return lbl;
}

// Creates an info row "[label]  [value]" inside parent; returns the value label.
static lv_obj_t *make_info_row(lv_obj_t *parent, const char *text)
{
    const ui_theme_t *th = ui_theme_get();

    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_shadow_width(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl, th->muted, 0);

    lv_obj_t *val = lv_label_create(row);
    lv_label_set_text(val, "--");
    lv_obj_set_style_text_font(val, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(val, th->text, 0);

    return val;
}

// =========================
// CREATE
// =========================
lv_obj_t *screen_extruir_create(lv_obj_t *parent)
{
    const ui_theme_t *th = ui_theme_get();

    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, th->bg, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    // ── LEFT COLUMN ───────────────────────────────────────────────
    lv_obj_t *left_col = lv_obj_create(root);
    lv_obj_set_size(left_col, 300, LV_PCT(100));
    lv_obj_set_flex_flow(left_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left_col,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(left_col, 12, 0);
    lv_obj_set_style_pad_gap(left_col, 8, 0);
    lv_obj_set_style_bg_opa(left_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left_col, 0, 0);
    lv_obj_set_style_shadow_width(left_col, 0, 0);
    lv_obj_clear_flag(left_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(left_col, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    // ─── Estilos velocímetro ──────────────────────────────────────
    lv_style_init(&style_needle);
    lv_style_set_line_width(&style_needle, 4);
    lv_style_set_line_color(&style_needle, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_line_rounded(&style_needle, true);

    lv_style_init(&style_section_target_arc);
    lv_style_set_arc_color(&style_section_target_arc, lv_color_hex(0xFF8C00));
    lv_style_set_arc_width(&style_section_target_arc, 8);

    lv_style_init(&style_section_target_tick);
    lv_style_set_line_color(&style_section_target_tick, lv_color_hex(0xFF8C00));
    lv_style_set_line_width(&style_section_target_tick, 3);
    lv_style_set_text_color(&style_section_target_tick, lv_color_hex(0xFF8C00));

    lv_style_init(&style_section_red_arc);
    lv_style_set_arc_color(&style_section_red_arc, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_arc_width(&style_section_red_arc, 8);

    lv_style_init(&style_section_red_tick);
    lv_style_set_line_color(&style_section_red_tick, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_line_width(&style_section_red_tick, 3);
    lv_style_set_text_color(&style_section_red_tick, lv_palette_main(LV_PALETTE_RED));

    // ─── Escala (velocímetro) ─────────────────────────────────────
    scale_speed = lv_scale_create(left_col);
    lv_obj_set_size(scale_speed, GAUGE_SIZE, GAUGE_SIZE);
    lv_scale_set_mode(scale_speed, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_range(scale_speed, 0, SPEED_MAX);
    lv_scale_set_angle_range(scale_speed, 270);
    lv_scale_set_rotation(scale_speed, 135);
    lv_scale_set_total_tick_count(scale_speed, 41);
    lv_scale_set_major_tick_every(scale_speed, 10);
    lv_scale_set_label_show(scale_speed, true);
    lv_obj_set_style_length(scale_speed, 5, LV_PART_ITEMS);
    lv_obj_set_style_length(scale_speed, 12, LV_PART_INDICATOR);

    lv_obj_set_style_arc_color(scale_speed, th->border, LV_PART_MAIN);
    lv_obj_set_style_line_color(scale_speed, th->text, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(scale_speed, th->muted, LV_PART_ITEMS);
    lv_obj_set_style_text_color(scale_speed, th->text, LV_PART_INDICATOR);

    section_redzone = lv_scale_add_section(scale_speed);
    lv_scale_section_set_range(section_redzone, SPEED_MAX - 2, SPEED_MAX);
    lv_scale_set_section_style_main(scale_speed, section_redzone, &style_section_red_arc);
    lv_scale_set_section_style_indicator(scale_speed, section_redzone, &style_section_red_tick);
    lv_scale_set_section_style_items(scale_speed, section_redzone, &style_section_red_tick);

    section_target = lv_scale_add_section(scale_speed);
    lv_scale_section_set_range(section_target, 0, 0);
    lv_scale_set_section_style_main(scale_speed, section_target, &style_section_target_arc);
    lv_scale_set_section_style_indicator(scale_speed, section_target, &style_section_target_tick);
    lv_scale_set_section_style_items(scale_speed, section_target, &style_section_target_tick);

    needle_current = lv_line_create(scale_speed);
    lv_obj_add_style(needle_current, &style_needle, 0);
    lv_scale_set_line_needle_value(scale_speed, needle_current, 95, 0);

    // ─── Velocidad actual ─────────────────────────────────────────
    label_speed = lv_label_create(left_col);
    lv_label_set_text(label_speed, "0.00 m/min");
    lv_obj_set_style_text_font(label_speed, FONT_LARGE, 0);
    lv_obj_set_style_text_color(label_speed, th->text, 0);

    // ─── Velocidad objetivo ───────────────────────────────────────
    label_target_speed = lv_label_create(left_col);
    lv_label_set_text(label_target_speed, "Objetivo: --");
    lv_obj_set_style_text_font(label_target_speed, FONT_SMALL, 0);
    lv_obj_set_style_text_color(label_target_speed, lv_color_hex(0xFF8C00), 0);

    // ─── Botón Grabar ─────────────────────────────────────────────
    btn_record = lv_btn_create(left_col);
    lv_obj_set_size(btn_record, 220, 52);
    lv_obj_set_style_bg_color(btn_record, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(btn_record, btn_event_cb, LV_EVENT_CLICKED, NULL);

    label_btn = lv_label_create(btn_record);
    lv_label_set_text(label_btn, "Grabar");
    lv_obj_set_style_text_font(label_btn, FONT_MEDIUM, 0);
    lv_obj_center(label_btn);

    // ── RIGHT COLUMN ──────────────────────────────────────────────
    lv_obj_t *right_col = lv_obj_create(root);
    lv_obj_set_size(right_col, 380, LV_PCT(100));
    lv_obj_set_flex_flow(right_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right_col,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_hor(right_col, 12, 0);
    lv_obj_set_style_pad_ver(right_col, 12, 0);
    lv_obj_set_style_pad_gap(right_col, 10, 0);
    lv_obj_set_style_bg_opa(right_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_col, 0, 0);
    lv_obj_set_style_shadow_width(right_col, 0, 0);
    lv_obj_clear_flag(right_col, LV_OBJ_FLAG_SCROLLABLE);

    // ─── Sección Corte ────────────────────────────────────────────
    make_section_label(right_col, "Corte:");

    cut_container = lv_obj_create(right_col);
    lv_obj_set_size(cut_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cut_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(cut_container,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_min_height(cut_container, 50, 0);
    lv_obj_set_style_pad_all(cut_container, 0, 0);
    lv_obj_set_style_pad_gap(cut_container, 8, 0);
    lv_obj_set_style_bg_opa(cut_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cut_container, 0, 0);
    lv_obj_set_style_shadow_width(cut_container, 0, 0);
    lv_obj_clear_flag(cut_container, LV_OBJ_FLAG_SCROLLABLE);

    // ─── Sección Cantidad objetivo ────────────────────────────────
    make_section_label(right_col, "Cantidad objetivo:");

    // Fila: [número grande]  [▲][▼]  (botones lado a lado)
    lv_obj_t *qty_row = lv_obj_create(right_col);
    lv_obj_set_size(qty_row, LV_PCT(100), 60);
    lv_obj_set_flex_flow(qty_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(qty_row,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(qty_row, 0, 0);
    lv_obj_set_style_bg_opa(qty_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(qty_row, 0, 0);
    lv_obj_set_style_shadow_width(qty_row, 0, 0);
    lv_obj_clear_flag(qty_row, LV_OBJ_FLAG_SCROLLABLE);

    label_qty = lv_label_create(qty_row);
    lv_obj_set_style_text_font(label_qty, FONT_LARGE, 0);
    lv_obj_set_style_text_color(label_qty, th->text, 0);
    lv_label_set_text(label_qty, "--");

    // contenedor de botones lado a lado
    lv_obj_t *btns_row = lv_obj_create(qty_row);
    lv_obj_set_size(btns_row, 140, 56);
    lv_obj_set_flex_flow(btns_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btns_row,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(btns_row, 0, 0);
    lv_obj_set_style_pad_gap(btns_row, 8, 0);
    lv_obj_set_style_bg_opa(btns_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btns_row, 0, 0);
    lv_obj_set_style_shadow_width(btns_row, 0, 0);
    lv_obj_clear_flag(btns_row, LV_OBJ_FLAG_SCROLLABLE);

    btn_qty_minus = lv_btn_create(btns_row);
    lv_obj_set_size(btn_qty_minus, 64, 56);
    lv_obj_add_event_cb(btn_qty_minus, qty_minus_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_qty_minus, qty_minus_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_t *lbl_down = lv_label_create(btn_qty_minus);
    lv_label_set_text(lbl_down, "▼");
    lv_obj_center(lbl_down);

    btn_qty_plus = lv_btn_create(btns_row);
    lv_obj_set_size(btn_qty_plus, 64, 56);
    lv_obj_add_event_cb(btn_qty_plus, qty_plus_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_qty_plus, qty_plus_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_t *lbl_up = lv_label_create(btn_qty_plus);
    lv_label_set_text(lbl_up, "▲");
    lv_obj_center(lbl_up);

    extrusion_set_target_count(target_qty_ui);

    // ─── Tarjeta de información ───────────────────────────────────
    lv_obj_t *info_card = lv_obj_create(right_col);
    lv_obj_set_size(info_card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(info_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(info_card, 12, 0);
    lv_obj_set_style_pad_gap(info_card, 10, 0);
    lv_obj_set_style_bg_color(info_card, th->surface, 0);
    lv_obj_set_style_bg_opa(info_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(info_card, th->border, 0);
    lv_obj_set_style_border_width(info_card, 1, 0);
    lv_obj_set_style_radius(info_card, 8, 0);
    lv_obj_set_style_shadow_width(info_card, 0, 0);
    lv_obj_clear_flag(info_card, LV_OBJ_FLAG_SCROLLABLE);

    label_extruded = make_info_row(info_card, "Longitud extruida:");
    label_remaining_time = make_info_row(info_card, "Tiempo restante:");
    label_finish_time = make_info_row(info_card, "Fin estimado:");

    screen_extruir_refresh_profile();

    // Rebuild can happen mid-recording (theme switch). Restore visual state.
    if (recording_ui)
    {
        lv_label_set_text(label_btn, "Detener");
        lv_obj_set_style_bg_color(btn_record, lv_palette_main(LV_PALETTE_RED), 0);
        set_cut_buttons_enabled(false);
        set_qty_buttons_enabled(false);
    }

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
        lv_scale_section_set_range(section_target, 0, 0);

        if (cut_container)
            lv_obj_clean(cut_container);
        return;
    }

    if (!profile_get_by_code(profile_code, &current_profile))
        return;

    update_qty_label();
    extrusion_set_cut_distance_m(current_profile.default_cut);

    int32_t tgt = (int32_t)(current_profile.belt_speed + 0.5f);
    lv_scale_section_set_range(section_target,
                               LV_MAX(0, tgt - 1),
                               LV_MIN(SPEED_MAX, tgt + 1));

    char speed_buf[64];
    snprintf(speed_buf, sizeof(speed_buf),
             "Objetivo: %.2f m/min", current_profile.belt_speed);
    lv_label_set_text(label_target_speed, speed_buf);

    if (!cut_container)
        return;
    lv_obj_clean(cut_container);

    for (int i = 0; i < current_profile.cut_options_count; i++)
    {
        float cut_m = current_profile.cut_options[i];

        cut_buttons[i] = lv_btn_create(cut_container);
        lv_obj_set_size(cut_buttons[i], 80, 48);

        lv_color_t color = (cut_m == current_profile.default_cut)
                               ? ui_theme_get()->blue
                               : ui_theme_get()->btn_grey;
        lv_obj_set_style_bg_color(cut_buttons[i], color, 0);

        lv_obj_add_event_cb(cut_buttons[i], cut_btn_event_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)((int)cut_m));

        char txt[16];
        snprintf(txt, sizeof(txt), "%.0f m", cut_m);

        lv_obj_t *label = lv_label_create(cut_buttons[i]);
        lv_label_set_text(label, txt);
        lv_obj_set_style_text_font(label, FONT_SMALL, 0);
        lv_obj_center(label);
    }
}

// =========================
// UPDATE
// =========================
void screen_extruir_update(void)
{
    float raw_speed = extrusion_get_speed_m_min();

    float alpha = (raw_speed > display_speed) ? 0.25f : 0.10f;
    display_speed = alpha * raw_speed + (1.0f - alpha) * display_speed;
    if (display_speed < 0.05f)
        display_speed = 0.0f;

    char buf[48];
    snprintf(buf, sizeof(buf), "%.2f m/min", raw_speed);
    lv_label_set_text(label_speed, buf);

    lv_scale_set_line_needle_value(scale_speed, needle_current, 95,
                                   (int32_t)(display_speed + 0.5f));

    // ─── Longitud extruida (progreso de la tirada actual) ─────────
    // Se reinicia solo al llegar a cut_dist gracias al módulo
    float cut_dist = extrusion_get_cut_distance_m();
    if (cut_dist > 0.0f)
    {
        float total_m = extrusion_get_total_mm() / 1000.0f;
        float piece_m = fmodf(total_m, cut_dist);
        snprintf(buf, sizeof(buf), "%.1f / %.1f m", piece_m, cut_dist);
        lv_label_set_text(label_extruded, buf);
    }
    else
    {
        lv_label_set_text(label_extruded, "--");
    }

    // ─── Tiempo restante y fin estimado ───────────────────────────
    // Tiempo por tirada a velocidad actual × tiradas restantes
    if (recording_ui && display_speed > 0.1f && cut_dist > 0.0f)
    {
        int completed = extrusion_get_total_count();
        int remaining_count = target_qty_ui - completed;
        if (remaining_count < 0)
            remaining_count = 0;

        float time_per_cut_min = cut_dist / display_speed;
        float remaining_min = time_per_cut_min * (float)remaining_count;

        int h = (int)(remaining_min / 60.0f);
        int m = (int)remaining_min % 60;
        int s = (int)((remaining_min - (float)(int)remaining_min) * 60.0f);

        if (h > 0)
            snprintf(buf, sizeof(buf), "%dh %02dm", h, m);
        else
            snprintf(buf, sizeof(buf), "%d:%02d", m, s);
        lv_label_set_text(label_remaining_time, buf);

        datetime_t rtc_now;
        rtc_get_datetime(&rtc_now);

        int current_minutes =
            (rtc_now.hour * 60) +
            rtc_now.min;

        int finish_minutes =
            current_minutes +
            (int)remaining_min;

        int finish_hour =
            (finish_minutes / 60) % 24;

        int finish_min =
            finish_minutes % 60;

        snprintf(buf,
                 sizeof(buf),
                 "%02d:%02d",
                 finish_hour,
                 finish_min);

        lv_label_set_text(label_finish_time, buf);
    }
    else
    {
        lv_label_set_text(label_remaining_time, "--");
        lv_label_set_text(label_finish_time, "--");
    }

    // ─── Lote completado ──────────────────────────────────────────
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
