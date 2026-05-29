#include "screen_config_machine.h"
#include "ui/fonts/fonts.h"
#include "ui/ui_theme.h"
#include "ui/ui_manager.h"
#include "logic/alarm_config.h"
#include "logic/calibration.h"
#include "lvgl.h"
#include <stdio.h>

// ─── Speed alarm refs ─────────────────────────────────────────────
static lv_obj_t *btn_alarm_on;
static lv_obj_t *btn_alarm_off;
static lv_obj_t *lbl_alarm_on;
static lv_obj_t *lbl_alarm_off;
static lv_obj_t *threshold_row;
static lv_obj_t *label_threshold_val;

// ─── Pre-cut alarm refs ───────────────────────────────────────────
static lv_obj_t *btn_precut_on;
static lv_obj_t *btn_precut_off;
static lv_obj_t *lbl_precut_on;
static lv_obj_t *lbl_precut_off;
static lv_obj_t *precut_sec_row;
static lv_obj_t *label_precut_sec_val;

// ─── Marking relay refs ───────────────────────────────────────────
static lv_obj_t *btn_relay_on;
static lv_obj_t *btn_relay_off;
static lv_obj_t *lbl_relay_on;
static lv_obj_t *lbl_relay_off;

// ─── Calibration refs ─────────────────────────────────────────────
static lv_obj_t *label_cal_factor_current;
static lv_obj_t *label_cal_teorica_val;
static lv_obj_t *label_cal_real_val;
static lv_obj_t *label_cal_factor_preview;

static float    s_cal_teorica    = 10.0f;
static float    s_cal_real       = 10.0f;
static uint32_t s_teo_hold_ms    = 0;
static uint32_t s_real_hold_ms   = 0;

// =========================
// HELPERS
// =========================
static void update_alarm_buttons(bool enabled)
{
    const ui_theme_t *th = ui_theme_get();

    lv_obj_set_style_bg_color(btn_alarm_on,  enabled  ? th->blue : th->pressed, 0);
    lv_obj_set_style_bg_color(btn_alarm_off, !enabled ? th->blue : th->pressed, 0);
    lv_obj_set_style_text_color(lbl_alarm_on,  enabled  ? lv_color_white() : th->subtle, 0);
    lv_obj_set_style_text_color(lbl_alarm_off, !enabled ? lv_color_white() : th->subtle, 0);

    if (enabled)
        lv_obj_clear_flag(threshold_row, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(threshold_row, LV_OBJ_FLAG_HIDDEN);
}

// =========================
// THEME CALLBACKS
// =========================
static void set_dark_cb(lv_event_t *e)
{
    ui_theme_set(UI_THEME_DARK);
    ui_rebuild();
}

static void set_light_cb(lv_event_t *e)
{
    ui_theme_set(UI_THEME_LIGHT);
    ui_rebuild();
}

// =========================
// ALARM CALLBACKS
// =========================
static void alarm_on_cb(lv_event_t *e)
{
    alarm_config_set(true, alarm_config_get_threshold());
    update_alarm_buttons(true);
}

static void alarm_off_cb(lv_event_t *e)
{
    alarm_config_set(false, alarm_config_get_threshold());
    update_alarm_buttons(false);
}

static void thr_minus_cb(lv_event_t *e)
{
    int t = alarm_config_get_threshold();
    if (t <= 1) return;
    alarm_config_set(alarm_config_is_enabled(), t - 1);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", t - 1);
    lv_label_set_text(label_threshold_val, buf);
}

static void thr_plus_cb(lv_event_t *e)
{
    int t = alarm_config_get_threshold();
    if (t >= 99) return;
    alarm_config_set(alarm_config_is_enabled(), t + 1);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", t + 1);
    lv_label_set_text(label_threshold_val, buf);
}

// =========================
// PRE-CUT ALARM CALLBACKS
// =========================
static void update_precut_buttons(bool enabled)
{
    const ui_theme_t *th = ui_theme_get();

    lv_obj_set_style_bg_color(btn_precut_on,  enabled  ? th->blue : th->pressed, 0);
    lv_obj_set_style_bg_color(btn_precut_off, !enabled ? th->blue : th->pressed, 0);
    lv_obj_set_style_text_color(lbl_precut_on,  enabled  ? lv_color_white() : th->subtle, 0);
    lv_obj_set_style_text_color(lbl_precut_off, !enabled ? lv_color_white() : th->subtle, 0);

    if (enabled)
        lv_obj_clear_flag(precut_sec_row, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(precut_sec_row, LV_OBJ_FLAG_HIDDEN);
}

static void precut_on_cb(lv_event_t *e)
{
    alarm_config_pre_cut_set(true, alarm_config_pre_cut_get_seconds());
    update_precut_buttons(true);
}

static void precut_off_cb(lv_event_t *e)
{
    alarm_config_pre_cut_set(false, alarm_config_pre_cut_get_seconds());
    update_precut_buttons(false);
}

static void precut_minus_cb(lv_event_t *e)
{
    int s = alarm_config_pre_cut_get_seconds();
    if (s <= 1) return;
    alarm_config_pre_cut_set(alarm_config_pre_cut_is_enabled(), s - 1);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d s", s - 1);
    lv_label_set_text(label_precut_sec_val, buf);
}

static void precut_plus_cb(lv_event_t *e)
{
    int s = alarm_config_pre_cut_get_seconds();
    if (s >= 30) return;
    alarm_config_pre_cut_set(alarm_config_pre_cut_is_enabled(), s + 1);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d s", s + 1);
    lv_label_set_text(label_precut_sec_val, buf);
}

// =========================
// MARKING RELAY CALLBACKS
// =========================
static void update_relay_buttons(bool enabled)
{
    const ui_theme_t *th = ui_theme_get();
    lv_obj_set_style_bg_color(btn_relay_on,  enabled  ? th->blue : th->pressed, 0);
    lv_obj_set_style_bg_color(btn_relay_off, !enabled ? th->blue : th->pressed, 0);
    lv_obj_set_style_text_color(lbl_relay_on,  enabled  ? lv_color_white() : th->subtle, 0);
    lv_obj_set_style_text_color(lbl_relay_off, !enabled ? lv_color_white() : th->subtle, 0);
}

static void relay_on_cb(lv_event_t *e)
{
    alarm_config_marking_relay_set(true);
    update_relay_buttons(true);
}

static void relay_off_cb(lv_event_t *e)
{
    alarm_config_marking_relay_set(false);
    update_relay_buttons(false);
}

// =========================
// CALIBRATION CALLBACKS
// =========================
static void update_cal_preview(void)
{
    if (s_cal_teorica <= 0.0f) return;
    float f = s_cal_real / s_cal_teorica;
    float pct = (f - 1.0f) * 100.0f;
    char buf[64];
    if (f < 0.8f)
        snprintf(buf, sizeof(buf), "Factor calculado: %.4f  (%.1f%% — fuera del rango -20%%)", f, pct);
    else if (f > 1.2f)
        snprintf(buf, sizeof(buf), "Factor calculado: %.4f  (+%.1f%% — fuera del rango +20%%)", f, pct);
    else
        snprintf(buf, sizeof(buf), "Factor calculado: %.4f  (%+.1f%%)", f, pct);
    lv_label_set_text(label_cal_factor_preview, buf);
}

// Snap float to 1 decimal place to avoid floating-point drift
static float snap1(float v)
{
    int i = (int)(v * 10.0f + 0.5f);
    return (float)i / 10.0f;
}

// Progressive step: slow at first, faster the longer you hold
static float accel_step(uint32_t held_ms)
{
    if (held_ms > 4000) return 5.0f;
    if (held_ms > 2000) return 1.0f;
    if (held_ms > 1000) return 0.5f;
    return 0.1f;
}

static void cal_teo_minus_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    float step;
    if (code == LV_EVENT_LONG_PRESSED) {
        s_teo_hold_ms = lv_tick_get();
        step = 0.1f;
    } else if (code == LV_EVENT_LONG_PRESSED_REPEAT) {
        step = accel_step(lv_tick_get() - s_teo_hold_ms);
    } else {
        step = 0.1f;
    }
    if (s_cal_teorica <= step + 0.001f) s_cal_teorica = 0.1f;
    else s_cal_teorica = snap1(s_cal_teorica - step);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f m", s_cal_teorica);
    lv_label_set_text(label_cal_teorica_val, buf);
    update_cal_preview();
}

static void cal_teo_plus_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    float step;
    if (code == LV_EVENT_LONG_PRESSED) {
        s_teo_hold_ms = lv_tick_get();
        step = 0.1f;
    } else if (code == LV_EVENT_LONG_PRESSED_REPEAT) {
        step = accel_step(lv_tick_get() - s_teo_hold_ms);
    } else {
        step = 0.1f;
    }
    s_cal_teorica = snap1(s_cal_teorica + step);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f m", s_cal_teorica);
    lv_label_set_text(label_cal_teorica_val, buf);
    update_cal_preview();
}

static void cal_real_minus_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    float step;
    if (code == LV_EVENT_LONG_PRESSED) {
        s_real_hold_ms = lv_tick_get();
        step = 0.1f;
    } else if (code == LV_EVENT_LONG_PRESSED_REPEAT) {
        step = accel_step(lv_tick_get() - s_real_hold_ms);
    } else {
        step = 0.1f;
    }
    if (s_cal_real <= step + 0.001f) s_cal_real = 0.1f;
    else s_cal_real = snap1(s_cal_real - step);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f m", s_cal_real);
    lv_label_set_text(label_cal_real_val, buf);
    update_cal_preview();
}

static void cal_real_plus_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    float step;
    if (code == LV_EVENT_LONG_PRESSED) {
        s_real_hold_ms = lv_tick_get();
        step = 0.1f;
    } else if (code == LV_EVENT_LONG_PRESSED_REPEAT) {
        step = accel_step(lv_tick_get() - s_real_hold_ms);
    } else {
        step = 0.1f;
    }
    s_cal_real = snap1(s_cal_real + step);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f m", s_cal_real);
    lv_label_set_text(label_cal_real_val, buf);
    update_cal_preview();
}

static void cal_apply_cb(lv_event_t *e)
{
    if (s_cal_teorica <= 0.0f) return;
    float new_factor = s_cal_real / s_cal_teorica;
    float pct = (new_factor - 1.0f) * 100.0f;
    char buf[64];

    if (new_factor > 1.2f) {
        snprintf(buf, sizeof(buf), "Error: +%.1f%% supera el maximo permitido (+20%%)", pct);
        lv_label_set_text(label_cal_factor_current, buf);
        return;
    }
    if (new_factor < 0.8f) {
        snprintf(buf, sizeof(buf), "Error: %.1f%% supera el maximo permitido (-20%%)", pct);
        lv_label_set_text(label_cal_factor_current, buf);
        return;
    }

    calibration_set_factor(new_factor);
    snprintf(buf, sizeof(buf), "Factor actual: %.4f", new_factor);
    lv_label_set_text(label_cal_factor_current, buf);
}

// =========================
// CREATE
// =========================
lv_obj_t *screen_config_machine_create(lv_obj_t *parent)
{
    const ui_theme_t *th = ui_theme_get();

    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, th->bg, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 20, 0);
    lv_obj_set_style_pad_gap(root, 12, 0);
    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    // ── Apariencia ────────────────────────────────────────────────
    lv_obj_t *lbl_section = lv_label_create(root);
    lv_label_set_text(lbl_section, "Apariencia");
    lv_obj_set_style_text_font(lbl_section, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_section, th->muted, 0);

    lv_obj_t *row = lv_obj_create(root);
    lv_obj_set_width(row, LV_SIZE_CONTENT);
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_gap(row, 12, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    bool is_dark = (th->id == UI_THEME_DARK);

    lv_obj_t *btn_dark = lv_btn_create(row);
    lv_obj_set_size(btn_dark, 180, 52);
    lv_obj_set_style_bg_color(btn_dark, is_dark ? th->blue : th->pressed, 0);
    lv_obj_set_style_bg_opa(btn_dark, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_dark, 0, 0);
    lv_obj_set_style_shadow_width(btn_dark, 0, 0);
    lv_obj_set_style_radius(btn_dark, 8, 0);
    if (!is_dark)
        lv_obj_add_event_cb(btn_dark, set_dark_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_dark = lv_label_create(btn_dark);
    lv_label_set_text(lbl_dark, "Oscuro");
    lv_obj_set_style_text_font(lbl_dark, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_dark, is_dark ? lv_color_white() : th->subtle, 0);
    lv_obj_center(lbl_dark);

    lv_obj_t *btn_light = lv_btn_create(row);
    lv_obj_set_size(btn_light, 180, 52);
    lv_obj_set_style_bg_color(btn_light, !is_dark ? th->blue : th->pressed, 0);
    lv_obj_set_style_bg_opa(btn_light, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_light, 0, 0);
    lv_obj_set_style_shadow_width(btn_light, 0, 0);
    lv_obj_set_style_radius(btn_light, 8, 0);
    if (is_dark)
        lv_obj_add_event_cb(btn_light, set_light_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_light = lv_label_create(btn_light);
    lv_label_set_text(lbl_light, "Claro");
    lv_obj_set_style_text_font(lbl_light, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_light, !is_dark ? lv_color_white() : th->subtle, 0);
    lv_obj_center(lbl_light);

    // ── Separador ─────────────────────────────────────────────────
    lv_obj_t *sep = lv_obj_create(root);
    lv_obj_set_size(sep, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(sep, th->border, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_pad_all(sep, 0, 0);
    lv_obj_clear_flag(sep, LV_OBJ_FLAG_SCROLLABLE);

    // ── Alarma de velocidad ───────────────────────────────────────
    lv_obj_t *lbl_alarm_sec = lv_label_create(root);
    lv_label_set_text(lbl_alarm_sec, "Alarma de velocidad");
    lv_obj_set_style_text_font(lbl_alarm_sec, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_alarm_sec, th->muted, 0);

    bool alarm_en = alarm_config_is_enabled();

    // Toggle row
    lv_obj_t *alarm_row = lv_obj_create(root);
    lv_obj_set_width(alarm_row, LV_SIZE_CONTENT);
    lv_obj_set_height(alarm_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(alarm_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(alarm_row, 0, 0);
    lv_obj_set_style_pad_all(alarm_row, 0, 0);
    lv_obj_set_style_pad_gap(alarm_row, 12, 0);
    lv_obj_set_flex_flow(alarm_row, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(alarm_row, LV_OBJ_FLAG_SCROLLABLE);

    btn_alarm_on = lv_btn_create(alarm_row);
    lv_obj_set_size(btn_alarm_on, 180, 52);
    lv_obj_set_style_bg_color(btn_alarm_on, alarm_en ? th->blue : th->pressed, 0);
    lv_obj_set_style_bg_opa(btn_alarm_on, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_alarm_on, 0, 0);
    lv_obj_set_style_shadow_width(btn_alarm_on, 0, 0);
    lv_obj_set_style_radius(btn_alarm_on, 8, 0);
    lv_obj_add_event_cb(btn_alarm_on, alarm_on_cb, LV_EVENT_CLICKED, NULL);

    lbl_alarm_on = lv_label_create(btn_alarm_on);
    lv_label_set_text(lbl_alarm_on, "Activa");
    lv_obj_set_style_text_font(lbl_alarm_on, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_alarm_on, alarm_en ? lv_color_white() : th->subtle, 0);
    lv_obj_center(lbl_alarm_on);

    btn_alarm_off = lv_btn_create(alarm_row);
    lv_obj_set_size(btn_alarm_off, 180, 52);
    lv_obj_set_style_bg_color(btn_alarm_off, !alarm_en ? th->blue : th->pressed, 0);
    lv_obj_set_style_bg_opa(btn_alarm_off, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_alarm_off, 0, 0);
    lv_obj_set_style_shadow_width(btn_alarm_off, 0, 0);
    lv_obj_set_style_radius(btn_alarm_off, 8, 0);
    lv_obj_add_event_cb(btn_alarm_off, alarm_off_cb, LV_EVENT_CLICKED, NULL);

    lbl_alarm_off = lv_label_create(btn_alarm_off);
    lv_label_set_text(lbl_alarm_off, "Desactiva");
    lv_obj_set_style_text_font(lbl_alarm_off, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_alarm_off, !alarm_en ? lv_color_white() : th->subtle, 0);
    lv_obj_center(lbl_alarm_off);

    // Threshold row: [Umbral:]   [▼] [10%] [▲]
    threshold_row = lv_obj_create(root);
    lv_obj_set_size(threshold_row, LV_PCT(100), 52);
    lv_obj_set_flex_flow(threshold_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(threshold_row,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(threshold_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(threshold_row, 0, 0);
    lv_obj_set_style_shadow_width(threshold_row, 0, 0);
    lv_obj_set_style_pad_all(threshold_row, 0, 0);
    lv_obj_clear_flag(threshold_row, LV_OBJ_FLAG_SCROLLABLE);
    if (!alarm_en)
        lv_obj_add_flag(threshold_row, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *lbl_thr_text = lv_label_create(threshold_row);
    lv_label_set_text(lbl_thr_text, "Umbral de desviacion:");
    lv_obj_set_style_text_font(lbl_thr_text, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_thr_text, th->text, 0);

    // Right side: [▼] [value] [▲]
    lv_obj_t *thr_ctrl = lv_obj_create(threshold_row);
    lv_obj_set_size(thr_ctrl, LV_SIZE_CONTENT, 52);
    lv_obj_set_flex_flow(thr_ctrl, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(thr_ctrl,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(thr_ctrl, 8, 0);
    lv_obj_set_style_bg_opa(thr_ctrl, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(thr_ctrl, 0, 0);
    lv_obj_set_style_shadow_width(thr_ctrl, 0, 0);
    lv_obj_set_style_pad_all(thr_ctrl, 0, 0);
    lv_obj_clear_flag(thr_ctrl, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_minus = lv_btn_create(thr_ctrl);
    lv_obj_set_size(btn_minus, 48, 44);
    lv_obj_set_style_shadow_width(btn_minus, 0, 0);
    lv_obj_set_style_radius(btn_minus, 6, 0);
    lv_obj_add_event_cb(btn_minus, thr_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_minus, thr_minus_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_t *lbl_m = lv_label_create(btn_minus);
    lv_label_set_text(lbl_m, "▼");
    lv_obj_set_style_text_font(lbl_m, FONT_SMALL, 0);
    lv_obj_center(lbl_m);

    label_threshold_val = lv_label_create(thr_ctrl);
    char thr_buf[8];
    snprintf(thr_buf, sizeof(thr_buf), "%d%%", alarm_config_get_threshold());
    lv_label_set_text(label_threshold_val, thr_buf);
    lv_obj_set_style_text_font(label_threshold_val, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(label_threshold_val, th->text, 0);
    lv_obj_set_width(label_threshold_val, 60);
    lv_obj_set_style_text_align(label_threshold_val, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *btn_plus = lv_btn_create(thr_ctrl);
    lv_obj_set_size(btn_plus, 48, 44);
    lv_obj_set_style_shadow_width(btn_plus, 0, 0);
    lv_obj_set_style_radius(btn_plus, 6, 0);
    lv_obj_add_event_cb(btn_plus, thr_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_plus, thr_plus_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_t *lbl_p = lv_label_create(btn_plus);
    lv_label_set_text(lbl_p, "▲");
    lv_obj_set_style_text_font(lbl_p, FONT_SMALL, 0);
    lv_obj_center(lbl_p);

    // ── Separador ─────────────────────────────────────────────────
    lv_obj_t *sep2 = lv_obj_create(root);
    lv_obj_set_size(sep2, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(sep2, th->border, 0);
    lv_obj_set_style_bg_opa(sep2, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep2, 0, 0);
    lv_obj_set_style_pad_all(sep2, 0, 0);
    lv_obj_clear_flag(sep2, LV_OBJ_FLAG_SCROLLABLE);

    // ── Alarma de corte ───────────────────────────────────────────
    lv_obj_t *lbl_precut_sec = lv_label_create(root);
    lv_label_set_text(lbl_precut_sec, "Alarma de corte");
    lv_obj_set_style_text_font(lbl_precut_sec, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_precut_sec, th->muted, 0);

    bool precut_en = alarm_config_pre_cut_is_enabled();

    lv_obj_t *precut_row = lv_obj_create(root);
    lv_obj_set_width(precut_row, LV_SIZE_CONTENT);
    lv_obj_set_height(precut_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(precut_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(precut_row, 0, 0);
    lv_obj_set_style_pad_all(precut_row, 0, 0);
    lv_obj_set_style_pad_gap(precut_row, 12, 0);
    lv_obj_set_flex_flow(precut_row, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(precut_row, LV_OBJ_FLAG_SCROLLABLE);

    btn_precut_on = lv_btn_create(precut_row);
    lv_obj_set_size(btn_precut_on, 180, 52);
    lv_obj_set_style_bg_color(btn_precut_on, precut_en ? th->blue : th->pressed, 0);
    lv_obj_set_style_bg_opa(btn_precut_on, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_precut_on, 0, 0);
    lv_obj_set_style_shadow_width(btn_precut_on, 0, 0);
    lv_obj_set_style_radius(btn_precut_on, 8, 0);
    lv_obj_add_event_cb(btn_precut_on, precut_on_cb, LV_EVENT_CLICKED, NULL);

    lbl_precut_on = lv_label_create(btn_precut_on);
    lv_label_set_text(lbl_precut_on, "Activa");
    lv_obj_set_style_text_font(lbl_precut_on, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_precut_on, precut_en ? lv_color_white() : th->subtle, 0);
    lv_obj_center(lbl_precut_on);

    btn_precut_off = lv_btn_create(precut_row);
    lv_obj_set_size(btn_precut_off, 180, 52);
    lv_obj_set_style_bg_color(btn_precut_off, !precut_en ? th->blue : th->pressed, 0);
    lv_obj_set_style_bg_opa(btn_precut_off, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_precut_off, 0, 0);
    lv_obj_set_style_shadow_width(btn_precut_off, 0, 0);
    lv_obj_set_style_radius(btn_precut_off, 8, 0);
    lv_obj_add_event_cb(btn_precut_off, precut_off_cb, LV_EVENT_CLICKED, NULL);

    lbl_precut_off = lv_label_create(btn_precut_off);
    lv_label_set_text(lbl_precut_off, "Desactiva");
    lv_obj_set_style_text_font(lbl_precut_off, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_precut_off, !precut_en ? lv_color_white() : th->subtle, 0);
    lv_obj_center(lbl_precut_off);

    // Seconds row: [Antelación:]  [▼] [3 s] [▲]
    precut_sec_row = lv_obj_create(root);
    lv_obj_set_size(precut_sec_row, LV_PCT(100), 52);
    lv_obj_set_flex_flow(precut_sec_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(precut_sec_row,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(precut_sec_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(precut_sec_row, 0, 0);
    lv_obj_set_style_shadow_width(precut_sec_row, 0, 0);
    lv_obj_set_style_pad_all(precut_sec_row, 0, 0);
    lv_obj_clear_flag(precut_sec_row, LV_OBJ_FLAG_SCROLLABLE);
    if (!precut_en)
        lv_obj_add_flag(precut_sec_row, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *lbl_precut_text = lv_label_create(precut_sec_row);
    lv_label_set_text(lbl_precut_text, "Antelacion (segundos):");
    lv_obj_set_style_text_font(lbl_precut_text, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_precut_text, th->text, 0);

    lv_obj_t *sec_ctrl = lv_obj_create(precut_sec_row);
    lv_obj_set_size(sec_ctrl, LV_SIZE_CONTENT, 52);
    lv_obj_set_flex_flow(sec_ctrl, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sec_ctrl,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(sec_ctrl, 8, 0);
    lv_obj_set_style_bg_opa(sec_ctrl, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sec_ctrl, 0, 0);
    lv_obj_set_style_shadow_width(sec_ctrl, 0, 0);
    lv_obj_set_style_pad_all(sec_ctrl, 0, 0);
    lv_obj_clear_flag(sec_ctrl, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_sec_minus = lv_btn_create(sec_ctrl);
    lv_obj_set_size(btn_sec_minus, 48, 44);
    lv_obj_set_style_shadow_width(btn_sec_minus, 0, 0);
    lv_obj_set_style_radius(btn_sec_minus, 6, 0);
    lv_obj_add_event_cb(btn_sec_minus, precut_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_sec_minus, precut_minus_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_t *lbl_sm = lv_label_create(btn_sec_minus);
    lv_label_set_text(lbl_sm, "▼");
    lv_obj_set_style_text_font(lbl_sm, FONT_SMALL, 0);
    lv_obj_center(lbl_sm);

    label_precut_sec_val = lv_label_create(sec_ctrl);
    char sec_buf[8];
    snprintf(sec_buf, sizeof(sec_buf), "%d s", alarm_config_pre_cut_get_seconds());
    lv_label_set_text(label_precut_sec_val, sec_buf);
    lv_obj_set_style_text_font(label_precut_sec_val, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(label_precut_sec_val, th->text, 0);
    lv_obj_set_width(label_precut_sec_val, 60);
    lv_obj_set_style_text_align(label_precut_sec_val, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *btn_sec_plus = lv_btn_create(sec_ctrl);
    lv_obj_set_size(btn_sec_plus, 48, 44);
    lv_obj_set_style_shadow_width(btn_sec_plus, 0, 0);
    lv_obj_set_style_radius(btn_sec_plus, 6, 0);
    lv_obj_add_event_cb(btn_sec_plus, precut_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_sec_plus, precut_plus_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_t *lbl_sp = lv_label_create(btn_sec_plus);
    lv_label_set_text(lbl_sp, "▲");
    lv_obj_set_style_text_font(lbl_sp, FONT_SMALL, 0);
    lv_obj_center(lbl_sp);

    // ── Separador ─────────────────────────────────────────────────
    lv_obj_t *sep_relay = lv_obj_create(root);
    lv_obj_set_size(sep_relay, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(sep_relay, th->border, 0);
    lv_obj_set_style_bg_opa(sep_relay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep_relay, 0, 0);
    lv_obj_set_style_pad_all(sep_relay, 0, 0);
    lv_obj_clear_flag(sep_relay, LV_OBJ_FLAG_SCROLLABLE);

    // ── Relay de marcación ────────────────────────────────────────
    lv_obj_t *lbl_relay_sec = lv_label_create(root);
    lv_label_set_text(lbl_relay_sec, "Relay de marcacion");
    lv_obj_set_style_text_font(lbl_relay_sec, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_relay_sec, th->muted, 0);

    bool relay_en = alarm_config_marking_relay_is_enabled();

    lv_obj_t *relay_row = lv_obj_create(root);
    lv_obj_set_width(relay_row, LV_SIZE_CONTENT);
    lv_obj_set_height(relay_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(relay_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(relay_row, 0, 0);
    lv_obj_set_style_pad_all(relay_row, 0, 0);
    lv_obj_set_style_pad_gap(relay_row, 12, 0);
    lv_obj_set_flex_flow(relay_row, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(relay_row, LV_OBJ_FLAG_SCROLLABLE);

    btn_relay_on = lv_btn_create(relay_row);
    lv_obj_set_size(btn_relay_on, 180, 52);
    lv_obj_set_style_bg_color(btn_relay_on, relay_en ? th->blue : th->pressed, 0);
    lv_obj_set_style_bg_opa(btn_relay_on, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_relay_on, 0, 0);
    lv_obj_set_style_shadow_width(btn_relay_on, 0, 0);
    lv_obj_set_style_radius(btn_relay_on, 8, 0);
    lv_obj_add_event_cb(btn_relay_on, relay_on_cb, LV_EVENT_CLICKED, NULL);

    lbl_relay_on = lv_label_create(btn_relay_on);
    lv_label_set_text(lbl_relay_on, "Activo");
    lv_obj_set_style_text_font(lbl_relay_on, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_relay_on, relay_en ? lv_color_white() : th->subtle, 0);
    lv_obj_center(lbl_relay_on);

    btn_relay_off = lv_btn_create(relay_row);
    lv_obj_set_size(btn_relay_off, 180, 52);
    lv_obj_set_style_bg_color(btn_relay_off, !relay_en ? th->blue : th->pressed, 0);
    lv_obj_set_style_bg_opa(btn_relay_off, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_relay_off, 0, 0);
    lv_obj_set_style_shadow_width(btn_relay_off, 0, 0);
    lv_obj_set_style_radius(btn_relay_off, 8, 0);
    lv_obj_add_event_cb(btn_relay_off, relay_off_cb, LV_EVENT_CLICKED, NULL);

    lbl_relay_off = lv_label_create(btn_relay_off);
    lv_label_set_text(lbl_relay_off, "Desactivado");
    lv_obj_set_style_text_font(lbl_relay_off, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_relay_off, !relay_en ? lv_color_white() : th->subtle, 0);
    lv_obj_center(lbl_relay_off);

    // ── Separador ─────────────────────────────────────────────────
    lv_obj_t *sep3 = lv_obj_create(root);
    lv_obj_set_size(sep3, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(sep3, th->border, 0);
    lv_obj_set_style_bg_opa(sep3, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep3, 0, 0);
    lv_obj_set_style_pad_all(sep3, 0, 0);
    lv_obj_clear_flag(sep3, LV_OBJ_FLAG_SCROLLABLE);

    // ── Calibración de longitud ───────────────────────────────────
    lv_obj_t *lbl_cal_sec = lv_label_create(root);
    lv_label_set_text(lbl_cal_sec, "Calibracion de longitud");
    lv_obj_set_style_text_font(lbl_cal_sec, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_cal_sec, th->muted, 0);

    // Current factor display
    label_cal_factor_current = lv_label_create(root);
    char cal_cur_buf[32];
    snprintf(cal_cur_buf, sizeof(cal_cur_buf), "Factor actual: %.4f",
             calibration_get_factor());
    lv_label_set_text(label_cal_factor_current, cal_cur_buf);
    lv_obj_set_style_text_font(label_cal_factor_current, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(label_cal_factor_current, th->text, 0);

    // Helper: build a spinbox row [label] [▼] [val] [▲]
    // ── Longitud teórica ─────────────────────────────────────────
    lv_obj_t *teo_row = lv_obj_create(root);
    lv_obj_set_size(teo_row, LV_PCT(100), 52);
    lv_obj_set_flex_flow(teo_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(teo_row,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(teo_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(teo_row, 0, 0);
    lv_obj_set_style_shadow_width(teo_row, 0, 0);
    lv_obj_set_style_pad_all(teo_row, 0, 0);
    lv_obj_clear_flag(teo_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_teo_text = lv_label_create(teo_row);
    lv_label_set_text(lbl_teo_text, "Longitud teorica (m):");
    lv_obj_set_style_text_font(lbl_teo_text, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_teo_text, th->text, 0);

    lv_obj_t *teo_ctrl = lv_obj_create(teo_row);
    lv_obj_set_size(teo_ctrl, LV_SIZE_CONTENT, 52);
    lv_obj_set_flex_flow(teo_ctrl, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(teo_ctrl, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(teo_ctrl, 8, 0);
    lv_obj_set_style_bg_opa(teo_ctrl, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(teo_ctrl, 0, 0);
    lv_obj_set_style_shadow_width(teo_ctrl, 0, 0);
    lv_obj_set_style_pad_all(teo_ctrl, 0, 0);
    lv_obj_clear_flag(teo_ctrl, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_teo_m = lv_btn_create(teo_ctrl);
    lv_obj_set_size(btn_teo_m, 48, 44);
    lv_obj_set_style_shadow_width(btn_teo_m, 0, 0);
    lv_obj_set_style_radius(btn_teo_m, 6, 0);
    lv_obj_add_event_cb(btn_teo_m, cal_teo_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_teo_m, cal_teo_minus_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(btn_teo_m, cal_teo_minus_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_t *lbl_teo_m = lv_label_create(btn_teo_m);
    lv_label_set_text(lbl_teo_m, "▼");
    lv_obj_set_style_text_font(lbl_teo_m, FONT_SMALL, 0);
    lv_obj_center(lbl_teo_m);

    label_cal_teorica_val = lv_label_create(teo_ctrl);
    lv_label_set_text(label_cal_teorica_val, "10.0 m");
    lv_obj_set_style_text_font(label_cal_teorica_val, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(label_cal_teorica_val, th->text, 0);
    lv_obj_set_width(label_cal_teorica_val, 110);
    lv_label_set_long_mode(label_cal_teorica_val, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(label_cal_teorica_val, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *btn_teo_p = lv_btn_create(teo_ctrl);
    lv_obj_set_size(btn_teo_p, 48, 44);
    lv_obj_set_style_shadow_width(btn_teo_p, 0, 0);
    lv_obj_set_style_radius(btn_teo_p, 6, 0);
    lv_obj_add_event_cb(btn_teo_p, cal_teo_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_teo_p, cal_teo_plus_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(btn_teo_p, cal_teo_plus_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_t *lbl_teo_p = lv_label_create(btn_teo_p);
    lv_label_set_text(lbl_teo_p, "▲");
    lv_obj_set_style_text_font(lbl_teo_p, FONT_SMALL, 0);
    lv_obj_center(lbl_teo_p);

    // ── Longitud real ─────────────────────────────────────────────
    lv_obj_t *real_row = lv_obj_create(root);
    lv_obj_set_size(real_row, LV_PCT(100), 52);
    lv_obj_set_flex_flow(real_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(real_row,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(real_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(real_row, 0, 0);
    lv_obj_set_style_shadow_width(real_row, 0, 0);
    lv_obj_set_style_pad_all(real_row, 0, 0);
    lv_obj_clear_flag(real_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_real_text = lv_label_create(real_row);
    lv_label_set_text(lbl_real_text, "Longitud real (m):");
    lv_obj_set_style_text_font(lbl_real_text, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_real_text, th->text, 0);

    lv_obj_t *real_ctrl = lv_obj_create(real_row);
    lv_obj_set_size(real_ctrl, LV_SIZE_CONTENT, 52);
    lv_obj_set_flex_flow(real_ctrl, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(real_ctrl, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(real_ctrl, 8, 0);
    lv_obj_set_style_bg_opa(real_ctrl, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(real_ctrl, 0, 0);
    lv_obj_set_style_shadow_width(real_ctrl, 0, 0);
    lv_obj_set_style_pad_all(real_ctrl, 0, 0);
    lv_obj_clear_flag(real_ctrl, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_real_m = lv_btn_create(real_ctrl);
    lv_obj_set_size(btn_real_m, 48, 44);
    lv_obj_set_style_shadow_width(btn_real_m, 0, 0);
    lv_obj_set_style_radius(btn_real_m, 6, 0);
    lv_obj_add_event_cb(btn_real_m, cal_real_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_real_m, cal_real_minus_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(btn_real_m, cal_real_minus_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_t *lbl_real_m = lv_label_create(btn_real_m);
    lv_label_set_text(lbl_real_m, "▼");
    lv_obj_set_style_text_font(lbl_real_m, FONT_SMALL, 0);
    lv_obj_center(lbl_real_m);

    label_cal_real_val = lv_label_create(real_ctrl);
    lv_label_set_text(label_cal_real_val, "10.0 m");
    lv_obj_set_style_text_font(label_cal_real_val, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(label_cal_real_val, th->text, 0);
    lv_obj_set_width(label_cal_real_val, 110);
    lv_label_set_long_mode(label_cal_real_val, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(label_cal_real_val, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *btn_real_p = lv_btn_create(real_ctrl);
    lv_obj_set_size(btn_real_p, 48, 44);
    lv_obj_set_style_shadow_width(btn_real_p, 0, 0);
    lv_obj_set_style_radius(btn_real_p, 6, 0);
    lv_obj_add_event_cb(btn_real_p, cal_real_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_real_p, cal_real_plus_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(btn_real_p, cal_real_plus_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_t *lbl_real_p = lv_label_create(btn_real_p);
    lv_label_set_text(lbl_real_p, "▲");
    lv_obj_set_style_text_font(lbl_real_p, FONT_SMALL, 0);
    lv_obj_center(lbl_real_p);

    // Factor preview label
    label_cal_factor_preview = lv_label_create(root);
    lv_label_set_text(label_cal_factor_preview, "Factor calculado: 1.0000");
    lv_obj_set_style_text_font(label_cal_factor_preview, FONT_SMALL, 0);
    lv_obj_set_style_text_color(label_cal_factor_preview, th->muted, 0);

    // Apply button
    lv_obj_t *btn_cal_apply = lv_btn_create(root);
    lv_obj_set_size(btn_cal_apply, LV_PCT(100), 52);
    lv_obj_set_style_bg_color(btn_cal_apply, th->blue, 0);
    lv_obj_set_style_bg_opa(btn_cal_apply, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_cal_apply, 0, 0);
    lv_obj_set_style_shadow_width(btn_cal_apply, 0, 0);
    lv_obj_set_style_radius(btn_cal_apply, 8, 0);
    lv_obj_add_event_cb(btn_cal_apply, cal_apply_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_cal_apply = lv_label_create(btn_cal_apply);
    lv_label_set_text(lbl_cal_apply, "Aplicar factor de correccion");
    lv_obj_set_style_text_font(lbl_cal_apply, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_cal_apply, lv_color_white(), 0);
    lv_obj_center(lbl_cal_apply);

    return root;
}
