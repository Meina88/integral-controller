#include "screen_config_machine.h"
#include "ui/fonts/fonts.h"
#include "ui/ui_theme.h"
#include "ui/ui_manager.h"
#include "logic/alarm_config.h"
#include "logic/calibration.h"
#include "logic/extrusion.h"
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
static lv_obj_t *relay_dur_row;
static lv_obj_t *label_relay_dur_val;
static lv_obj_t *btn_relay_test;
static lv_obj_t *lbl_relay_test;

// ─── Relay test cooldown state ────────────────────────────────────
static int         s_relay_dur_ds          = 5;   // loaded from alarm_config on build
static int         s_test_count            = 0;
static lv_timer_t *s_relay_cooldown_timer  = NULL;

// ─── Calibration refs ─────────────────────────────────────────────
static lv_obj_t *label_cal_factor_current;
static lv_obj_t *label_cal_teorica_val;
static lv_obj_t *label_cal_real_val;
static lv_obj_t *label_cal_factor_preview;

static float    s_cal_teorica    = 10.0f;
static float    s_cal_real       = 10.0f;
static uint32_t s_teo_hold_ms    = 0;
static uint32_t s_real_hold_ms   = 0;

// ─── Spray shots refs ─────────────────────────────────────────────
static lv_obj_t *label_spray_remaining;
static lv_obj_t *label_spray_shots_val;
static uint32_t  s_spray_hold_ms = 0;

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
// SCREEN CLEANUP
// =========================
static void screen_config_machine_delete_cb(lv_event_t *e)
{
    relay_dur_row       = NULL;
    label_relay_dur_val = NULL;
    btn_relay_test      = NULL;
    lbl_relay_test      = NULL;

    if (s_relay_cooldown_timer) {
        lv_timer_del(s_relay_cooldown_timer);
        s_relay_cooldown_timer = NULL;
    }
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

    if (lv_obj_is_valid(relay_dur_row)) {
        if (enabled)
            lv_obj_clear_flag(relay_dur_row, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(relay_dur_row, LV_OBJ_FLAG_HIDDEN);
    }
    if (lv_obj_is_valid(btn_relay_test)) {
        if (enabled)
            lv_obj_clear_flag(btn_relay_test, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(btn_relay_test, LV_OBJ_FLAG_HIDDEN);
    }
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

static void relay_dur_minus_cb(lv_event_t *e)
{
    if (s_relay_dur_ds <= 1) return;
    s_relay_dur_ds--;
    alarm_config_marking_relay_set_duration_ds(s_relay_dur_ds);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f s", s_relay_dur_ds / 10.0f);
    lv_label_set_text(label_relay_dur_val, buf);
}

static void relay_dur_plus_cb(lv_event_t *e)
{
    if (s_relay_dur_ds >= 50) return;
    s_relay_dur_ds++;
    alarm_config_marking_relay_set_duration_ds(s_relay_dur_ds);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f s", s_relay_dur_ds / 10.0f);
    lv_label_set_text(label_relay_dur_val, buf);
}

static void relay_cooldown_timer_cb(lv_timer_t *t)
{
    (void)t;
    s_test_count = 0;
    s_relay_cooldown_timer = NULL;
    if (lv_obj_is_valid(btn_relay_test) && lv_obj_is_valid(lbl_relay_test)) {
        const ui_theme_t *th = ui_theme_get();
        lv_obj_set_style_bg_color(btn_relay_test, th->blue, 0);
        lv_obj_set_style_text_color(lbl_relay_test, lv_color_white(), 0);
    }
}

static void relay_overheat_close_cb(lv_event_t *e)
{
    lv_obj_t *modal = (lv_obj_t *)lv_event_get_user_data(e);
    if (lv_obj_is_valid(modal))
        lv_obj_del(modal);
}

static void show_relay_overheat_modal(void)
{
    const ui_theme_t *th = ui_theme_get();

    lv_obj_t *modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_60, 0);
    lv_obj_set_style_border_width(modal, 0, 0);
    lv_obj_set_style_pad_all(modal, 0, 0);
    lv_obj_clear_flag(modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(modal);
    lv_obj_set_size(card, 460, LV_SIZE_CONTENT);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, th->surface, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 28, 0);
    lv_obj_set_style_pad_gap(card, 16, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, "Atencion");
    lv_obj_set_style_text_font(lbl_title, FONT_LARGE, 0);
    lv_obj_set_style_text_color(lbl_title, th->text, 0);

    lv_obj_t *lbl_msg = lv_label_create(card);
    lv_label_set_text(lbl_msg,
        "Riesgo de sobrecalentamiento del solenoide.\n"
        "Por favor aguarde un minuto antes de continuar.");
    lv_obj_set_style_text_font(lbl_msg, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_msg, th->text, 0);
    lv_label_set_long_mode(lbl_msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_msg, 400);
    lv_obj_set_style_text_align(lbl_msg, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *btn_ok = lv_btn_create(card);
    lv_obj_set_size(btn_ok, 200, 52);
    lv_obj_set_style_bg_color(btn_ok, th->blue, 0);
    lv_obj_set_style_bg_opa(btn_ok, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_ok, 0, 0);
    lv_obj_set_style_shadow_width(btn_ok, 0, 0);
    lv_obj_set_style_radius(btn_ok, 8, 0);
    lv_obj_add_event_cb(btn_ok, relay_overheat_close_cb, LV_EVENT_CLICKED, modal);

    lv_obj_t *lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, "Entendido");
    lv_obj_set_style_text_font(lbl_ok, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_ok, lv_color_white(), 0);
    lv_obj_center(lbl_ok);
}

static void relay_test_cb(lv_event_t *e)
{
    if (s_test_count >= 5) {
        show_relay_overheat_modal();
        return;
    }

    extrusion_relay_test();
    s_test_count++;

    if (s_test_count >= 5) {
        if (s_relay_cooldown_timer == NULL) {
            s_relay_cooldown_timer = lv_timer_create(relay_cooldown_timer_cb, 60000, NULL);
            lv_timer_set_repeat_count(s_relay_cooldown_timer, 1);
        }
        if (lv_obj_is_valid(btn_relay_test) && lv_obj_is_valid(lbl_relay_test)) {
            const ui_theme_t *th = ui_theme_get();
            lv_obj_set_style_bg_color(btn_relay_test, th->pressed, 0);
            lv_obj_set_style_text_color(lbl_relay_test, th->subtle, 0);
        }
    }
}

// =========================
// SPRAY SHOTS CALLBACKS
// =========================
static void update_spray_remaining(void)
{
    char buf[48];
    snprintf(buf, sizeof(buf), "Restantes: %d / %d disparos",
             alarm_config_spray_shots_get_remaining(),
             alarm_config_spray_shots_get_max());
    lv_label_set_text(label_spray_remaining, buf);
}

static int accel_step_int(uint32_t held_ms)
{
    if (held_ms > 4000) return 100;
    if (held_ms > 2000) return 50;
    if (held_ms > 1000) return 10;
    return 1;
}

static void spray_minus_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int step;
    if (code == LV_EVENT_LONG_PRESSED) {
        s_spray_hold_ms = lv_tick_get();
        step = 1;
    } else if (code == LV_EVENT_LONG_PRESSED_REPEAT) {
        step = accel_step_int(lv_tick_get() - s_spray_hold_ms);
    } else {
        step = 1;
    }
    int cur = alarm_config_spray_shots_get_max();
    int nxt = cur - step;
    if (nxt < 1) nxt = 1;
    if (nxt == cur) return;
    alarm_config_spray_shots_set_max(nxt);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", nxt);
    lv_label_set_text(label_spray_shots_val, buf);
    update_spray_remaining();
}

static void spray_plus_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int step;
    if (code == LV_EVENT_LONG_PRESSED) {
        s_spray_hold_ms = lv_tick_get();
        step = 1;
    } else if (code == LV_EVENT_LONG_PRESSED_REPEAT) {
        step = accel_step_int(lv_tick_get() - s_spray_hold_ms);
    } else {
        step = 1;
    }
    int cur = alarm_config_spray_shots_get_max();
    int nxt = cur + step;
    if (nxt > 9999) nxt = 9999;
    if (nxt == cur) return;
    alarm_config_spray_shots_set_max(nxt);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", nxt);
    lv_label_set_text(label_spray_shots_val, buf);
    update_spray_remaining();
}

static void spray_reset_cb(lv_event_t *e)
{
    alarm_config_spray_shots_reset();
    update_spray_remaining();
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

    // Fila de duración: [Duracion de activacion:]  [▼] [0.5 s] [▲]
    s_relay_dur_ds = alarm_config_marking_relay_get_duration_ds();

    relay_dur_row = lv_obj_create(root);
    lv_obj_set_size(relay_dur_row, LV_PCT(100), 52);
    lv_obj_set_flex_flow(relay_dur_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(relay_dur_row,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(relay_dur_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(relay_dur_row, 0, 0);
    lv_obj_set_style_shadow_width(relay_dur_row, 0, 0);
    lv_obj_set_style_pad_all(relay_dur_row, 0, 0);
    lv_obj_clear_flag(relay_dur_row, LV_OBJ_FLAG_SCROLLABLE);
    if (!relay_en)
        lv_obj_add_flag(relay_dur_row, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *lbl_dur_text = lv_label_create(relay_dur_row);
    lv_label_set_text(lbl_dur_text, "Duracion de activacion:");
    lv_obj_set_style_text_font(lbl_dur_text, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_dur_text, th->text, 0);

    lv_obj_t *dur_ctrl = lv_obj_create(relay_dur_row);
    lv_obj_set_size(dur_ctrl, LV_SIZE_CONTENT, 52);
    lv_obj_set_flex_flow(dur_ctrl, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dur_ctrl, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(dur_ctrl, 8, 0);
    lv_obj_set_style_bg_opa(dur_ctrl, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dur_ctrl, 0, 0);
    lv_obj_set_style_shadow_width(dur_ctrl, 0, 0);
    lv_obj_set_style_pad_all(dur_ctrl, 0, 0);
    lv_obj_clear_flag(dur_ctrl, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_dur_m = lv_btn_create(dur_ctrl);
    lv_obj_set_size(btn_dur_m, 48, 44);
    lv_obj_set_style_shadow_width(btn_dur_m, 0, 0);
    lv_obj_set_style_radius(btn_dur_m, 6, 0);
    lv_obj_add_event_cb(btn_dur_m, relay_dur_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_dur_m, relay_dur_minus_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_t *lbl_dur_m = lv_label_create(btn_dur_m);
    lv_label_set_text(lbl_dur_m, "▼");
    lv_obj_set_style_text_font(lbl_dur_m, FONT_SMALL, 0);
    lv_obj_center(lbl_dur_m);

    label_relay_dur_val = lv_label_create(dur_ctrl);
    char dur_buf[16];
    snprintf(dur_buf, sizeof(dur_buf), "%.1f s", s_relay_dur_ds / 10.0f);
    lv_label_set_text(label_relay_dur_val, dur_buf);
    lv_obj_set_style_text_font(label_relay_dur_val, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(label_relay_dur_val, th->text, 0);
    lv_obj_set_width(label_relay_dur_val, 70);
    lv_obj_set_style_text_align(label_relay_dur_val, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *btn_dur_p = lv_btn_create(dur_ctrl);
    lv_obj_set_size(btn_dur_p, 48, 44);
    lv_obj_set_style_shadow_width(btn_dur_p, 0, 0);
    lv_obj_set_style_radius(btn_dur_p, 6, 0);
    lv_obj_add_event_cb(btn_dur_p, relay_dur_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_dur_p, relay_dur_plus_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_t *lbl_dur_p = lv_label_create(btn_dur_p);
    lv_label_set_text(lbl_dur_p, "▲");
    lv_obj_set_style_text_font(lbl_dur_p, FONT_SMALL, 0);
    lv_obj_center(lbl_dur_p);

    // Botón Test
    btn_relay_test = lv_btn_create(root);
    lv_obj_set_size(btn_relay_test, LV_PCT(100), 52);
    lv_obj_set_style_bg_color(btn_relay_test, th->blue, 0);
    lv_obj_set_style_bg_opa(btn_relay_test, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_relay_test, 0, 0);
    lv_obj_set_style_shadow_width(btn_relay_test, 0, 0);
    lv_obj_set_style_radius(btn_relay_test, 8, 0);
    lv_obj_add_event_cb(btn_relay_test, relay_test_cb, LV_EVENT_CLICKED, NULL);
    if (!relay_en)
        lv_obj_add_flag(btn_relay_test, LV_OBJ_FLAG_HIDDEN);

    lbl_relay_test = lv_label_create(btn_relay_test);
    lv_label_set_text(lbl_relay_test, "Test");
    lv_obj_set_style_text_font(lbl_relay_test, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_relay_test, lv_color_white(), 0);
    lv_obj_center(lbl_relay_test);

    // ── Separador ─────────────────────────────────────────────────
    lv_obj_t *sep3 = lv_obj_create(root);
    lv_obj_set_size(sep3, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(sep3, th->border, 0);
    lv_obj_set_style_bg_opa(sep3, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep3, 0, 0);
    lv_obj_set_style_pad_all(sep3, 0, 0);
    lv_obj_clear_flag(sep3, LV_OBJ_FLAG_SCROLLABLE);

    // ── Contador de pintura ───────────────────────────────────────
    lv_obj_t *lbl_spray_sec = lv_label_create(root);
    lv_label_set_text(lbl_spray_sec, "Contador de pintura");
    lv_obj_set_style_text_font(lbl_spray_sec, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_spray_sec, th->muted, 0);

    label_spray_remaining = lv_label_create(root);
    lv_obj_set_style_text_font(label_spray_remaining, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(label_spray_remaining, th->text, 0);
    update_spray_remaining();

    // Fila: [Capacidad del spray (disparos):]  [▼] [valor] [▲]
    lv_obj_t *spray_row = lv_obj_create(root);
    lv_obj_set_size(spray_row, LV_PCT(100), 52);
    lv_obj_set_flex_flow(spray_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(spray_row,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(spray_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spray_row, 0, 0);
    lv_obj_set_style_shadow_width(spray_row, 0, 0);
    lv_obj_set_style_pad_all(spray_row, 0, 0);
    lv_obj_clear_flag(spray_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_spray_text = lv_label_create(spray_row);
    lv_label_set_text(lbl_spray_text, "Capacidad del spray (disparos):");
    lv_obj_set_style_text_font(lbl_spray_text, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_spray_text, th->text, 0);

    lv_obj_t *spray_ctrl = lv_obj_create(spray_row);
    lv_obj_set_size(spray_ctrl, LV_SIZE_CONTENT, 52);
    lv_obj_set_flex_flow(spray_ctrl, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(spray_ctrl, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(spray_ctrl, 8, 0);
    lv_obj_set_style_bg_opa(spray_ctrl, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spray_ctrl, 0, 0);
    lv_obj_set_style_shadow_width(spray_ctrl, 0, 0);
    lv_obj_set_style_pad_all(spray_ctrl, 0, 0);
    lv_obj_clear_flag(spray_ctrl, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_spray_m = lv_btn_create(spray_ctrl);
    lv_obj_set_size(btn_spray_m, 48, 44);
    lv_obj_set_style_shadow_width(btn_spray_m, 0, 0);
    lv_obj_set_style_radius(btn_spray_m, 6, 0);
    lv_obj_add_event_cb(btn_spray_m, spray_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_spray_m, spray_minus_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(btn_spray_m, spray_minus_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_t *lbl_spray_m = lv_label_create(btn_spray_m);
    lv_label_set_text(lbl_spray_m, "▼");
    lv_obj_set_style_text_font(lbl_spray_m, FONT_SMALL, 0);
    lv_obj_center(lbl_spray_m);

    label_spray_shots_val = lv_label_create(spray_ctrl);
    char spray_buf[8];
    snprintf(spray_buf, sizeof(spray_buf), "%d", alarm_config_spray_shots_get_max());
    lv_label_set_text(label_spray_shots_val, spray_buf);
    lv_obj_set_style_text_font(label_spray_shots_val, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(label_spray_shots_val, th->text, 0);
    lv_obj_set_width(label_spray_shots_val, 80);
    lv_label_set_long_mode(label_spray_shots_val, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(label_spray_shots_val, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *btn_spray_p = lv_btn_create(spray_ctrl);
    lv_obj_set_size(btn_spray_p, 48, 44);
    lv_obj_set_style_shadow_width(btn_spray_p, 0, 0);
    lv_obj_set_style_radius(btn_spray_p, 6, 0);
    lv_obj_add_event_cb(btn_spray_p, spray_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_spray_p, spray_plus_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(btn_spray_p, spray_plus_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_t *lbl_spray_p = lv_label_create(btn_spray_p);
    lv_label_set_text(lbl_spray_p, "▲");
    lv_obj_set_style_text_font(lbl_spray_p, FONT_SMALL, 0);
    lv_obj_center(lbl_spray_p);

    lv_obj_t *btn_spray_reset = lv_btn_create(root);
    lv_obj_set_size(btn_spray_reset, LV_PCT(100), 52);
    lv_obj_set_style_bg_color(btn_spray_reset, th->blue, 0);
    lv_obj_set_style_bg_opa(btn_spray_reset, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_spray_reset, 0, 0);
    lv_obj_set_style_shadow_width(btn_spray_reset, 0, 0);
    lv_obj_set_style_radius(btn_spray_reset, 8, 0);
    lv_obj_add_event_cb(btn_spray_reset, spray_reset_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_spray_reset = lv_label_create(btn_spray_reset);
    lv_label_set_text(lbl_spray_reset, "Reiniciar contador");
    lv_obj_set_style_text_font(lbl_spray_reset, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_spray_reset, lv_color_white(), 0);
    lv_obj_center(lbl_spray_reset);

    // ── Separador ─────────────────────────────────────────────────
    lv_obj_t *sep4 = lv_obj_create(root);
    lv_obj_set_size(sep4, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(sep4, th->border, 0);
    lv_obj_set_style_bg_opa(sep4, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep4, 0, 0);
    lv_obj_set_style_pad_all(sep4, 0, 0);
    lv_obj_clear_flag(sep4, LV_OBJ_FLAG_SCROLLABLE);

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

    lv_obj_add_event_cb(root, screen_config_machine_delete_cb, LV_EVENT_DELETE, NULL);

    return root;
}
