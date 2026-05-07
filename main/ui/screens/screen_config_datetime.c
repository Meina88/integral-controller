#include "screen_config_datetime.h"

#include "lvgl.h"
#include "drivers/rtc/rtc.h"
#include "drivers/rtc/rtc_pcf85063a.h"

#include "ui/components/numpad.h"

// 🔥 FUENTES INTER
#include "inter_18.h"
#include "inter_24.h"
#include "inter_28.h"

#include <stdio.h>

static lv_obj_t *root;
static lv_obj_t *ta_date;
static lv_obj_t *ta_time;

// =========================
// NUMPAD
// =========================
static void ta_event_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);

    numpad_open(ta, NULL);
}

// =========================
// BOTÓN GUARDAR
// =========================
static void btn_save_cb(lv_event_t *e)
{
    const char *date = lv_textarea_get_text(ta_date);
    const char *time = lv_textarea_get_text(ta_time);

    datetime_t t;

    int year, month, day;
    int hour, min, sec;

    // FECHA
    if (sscanf(date, "%d.%d.%d", &year, &month, &day) != 3)
    {
        printf("ERROR fecha\n");
        return;
    }

    // HORA
    if (sscanf(time, "%d.%d.%d", &hour, &min, &sec) != 3)
    {
        printf("ERROR hora\n");
        return;
    }

    // ASIGNAR
    t.year  = (uint16_t)year;
    t.month = (uint8_t)month;
    t.day   = (uint8_t)day;

    t.hour = (uint8_t)hour;
    t.min  = (uint8_t)min;
    t.sec  = (uint8_t)sec;

    t.dotw = 1;

    PCF85063A_Set_All(t);

    printf("RTC actualizado OK\n");
}

// =========================
// CREATE
// =========================
lv_obj_t *screen_config_datetime_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);

    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));

    lv_obj_set_style_border_width(root, 0, 0);

    lv_obj_set_style_bg_color(
        root,
        lv_color_hex(0x101418),
        0);

    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

    // =========================
    // CARD
    // =========================
    lv_obj_t *card = lv_obj_create(root);

    lv_obj_set_size(card, 500, 520);

    lv_obj_center(card);

    lv_obj_set_style_radius(card, 18, 0);

    lv_obj_set_style_bg_color(
        card,
        lv_color_hex(0x1B222A),
        0);

    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);

    lv_obj_set_style_border_width(card, 0, 0);

    lv_obj_set_style_shadow_width(card, 25, 0);

    lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);

    lv_obj_set_style_shadow_color(
        card,
        lv_color_black(),
        0);

    lv_obj_set_layout(card, LV_LAYOUT_FLEX);

    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);

    lv_obj_set_style_pad_all(card, 28, 0);

    lv_obj_set_style_pad_gap(card, 18, 0);

    // =========================
    // TÍTULO
    // =========================
    lv_obj_t *title = lv_label_create(card);

    lv_label_set_text(
        title,
        "Configuración Ñandú ÁÉÍÓÚ");

    lv_obj_set_style_text_font(
        title,
        &inter_28,
        0);

    lv_obj_set_style_text_color(
        title,
        lv_color_white(),
        0);

    // =========================
    // SUBTEXTO
    // =========================
    lv_obj_t *subtitle = lv_label_create(card);

    lv_label_set_text(
        subtitle,
        "Prueba de caracteres especiales:\n"
        "mañana, producción, hidráulica,\n"
        "configuración, extrusión, conexión.");

    lv_obj_set_style_text_font(
        subtitle,
        &inter_18,
        0);

    lv_obj_set_style_text_color(
        subtitle,
        lv_color_hex(0xB0B7C0),
        0);

    // =========================
    // LABEL FECHA
    // =========================
    lv_obj_t *lbl_date = lv_label_create(card);

    lv_label_set_text(
        lbl_date,
        "FECHA");

    lv_obj_set_style_text_font(
        lbl_date,
        &inter_18,
        0);

    lv_obj_set_style_text_color(
        lbl_date,
        lv_color_hex(0xAAAAAA),
        0);

    // =========================
    // INPUT FECHA
    // =========================
    ta_date = lv_textarea_create(card);

    lv_obj_set_width(ta_date, LV_PCT(100));

    lv_obj_set_height(ta_date, 60);

    lv_textarea_set_placeholder_text(
        ta_date,
        "YYYY.MM.DD");

    lv_obj_add_event_cb(
        ta_date,
        ta_event_cb,
        LV_EVENT_CLICKED,
        NULL);

    lv_obj_set_style_radius(ta_date, 12, 0);

    lv_obj_set_style_bg_color(
        ta_date,
        lv_color_hex(0x2A313A),
        0);

    lv_obj_set_style_bg_opa(
        ta_date,
        LV_OPA_COVER,
        0);

    lv_obj_set_style_border_width(
        ta_date,
        0,
        0);

    lv_obj_set_style_text_color(
        ta_date,
        lv_color_white(),
        0);

    lv_obj_set_style_text_font(
        ta_date,
        &inter_24,
        0);

    lv_obj_set_style_pad_left(
        ta_date,
        18,
        0);

    // =========================
    // LABEL HORA
    // =========================
    lv_obj_t *lbl_time = lv_label_create(card);

    lv_label_set_text(
        lbl_time,
        "HORA");

    lv_obj_set_style_text_font(
        lbl_time,
        &inter_18,
        0);

    lv_obj_set_style_text_color(
        lbl_time,
        lv_color_hex(0xAAAAAA),
        0);

    // =========================
    // INPUT HORA
    // =========================
    ta_time = lv_textarea_create(card);

    lv_obj_set_width(ta_time, LV_PCT(100));

    lv_obj_set_height(ta_time, 60);

    lv_textarea_set_placeholder_text(
        ta_time,
        "HH.MM.SS");

    lv_obj_add_event_cb(
        ta_time,
        ta_event_cb,
        LV_EVENT_CLICKED,
        NULL);

    lv_obj_set_style_radius(ta_time, 12, 0);

    lv_obj_set_style_bg_color(
        ta_time,
        lv_color_hex(0x2A313A),
        0);

    lv_obj_set_style_bg_opa(
        ta_time,
        LV_OPA_COVER,
        0);

    lv_obj_set_style_border_width(
        ta_time,
        0,
        0);

    lv_obj_set_style_text_color(
        ta_time,
        lv_color_white(),
        0);

    lv_obj_set_style_text_font(
        ta_time,
        &inter_24,
        0);

    lv_obj_set_style_pad_left(
        ta_time,
        18,
        0);

    // =========================
    // BOTÓN
    // =========================
    lv_obj_t *btn = lv_btn_create(card);

    lv_obj_set_width(btn, LV_PCT(100));

    lv_obj_set_height(btn, 64);

    lv_obj_add_event_cb(
        btn,
        btn_save_cb,
        LV_EVENT_CLICKED,
        NULL);

    lv_obj_set_style_radius(btn, 14, 0);

    lv_obj_set_style_bg_color(
        btn,
        lv_palette_main(LV_PALETTE_BLUE),
        0);

    lv_obj_set_style_bg_grad_color(
        btn,
        lv_palette_darken(LV_PALETTE_BLUE, 2),
        0);

    lv_obj_set_style_bg_grad_dir(
        btn,
        LV_GRAD_DIR_VER,
        0);

    lv_obj_t *lbl = lv_label_create(btn);

    lv_label_set_text(
        lbl,
        "Guardar configuración");

    lv_obj_set_style_text_font(
        lbl,
        &inter_24,
        0);

    lv_obj_center(lbl);

    return root;
}

// =========================
// UPDATE
// =========================
void screen_config_datetime_update(void)
{
}