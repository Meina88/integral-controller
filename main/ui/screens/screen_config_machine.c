#include "screen_config_machine.h"
#include "ui/fonts/fonts.h"
#include "ui/ui_theme.h"
#include "ui/ui_manager.h"
#include "logic/alarm_config.h"
#include "lvgl.h"
#include <stdio.h>

// ─── Refs para actualizar en callbacks ──────────────────────────
static lv_obj_t *btn_alarm_on;
static lv_obj_t *btn_alarm_off;
static lv_obj_t *lbl_alarm_on;
static lv_obj_t *lbl_alarm_off;
static lv_obj_t *threshold_row;
static lv_obj_t *label_threshold_val;

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
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

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

    return root;
}
