#include "screen_config_datetime.h"

#include "lvgl.h"
#include "drivers/rtc/rtc.h"
#include "drivers/rtc/rtc_pcf85063a.h"

#include "ui/components/numpad.h"
#include "ui/fonts/fonts.h"
#include "ui/ui_theme.h"

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

    if (sscanf(date, "%d.%d.%d", &year, &month, &day) != 3)
    {
        printf("ERROR fecha\n");
        return;
    }

    if (sscanf(time, "%d.%d.%d", &hour, &min, &sec) != 3)
    {
        printf("ERROR hora\n");
        return;
    }

    t.year  = (uint16_t)year;
    t.month = (uint8_t)month;
    t.day   = (uint8_t)day;
    t.hour  = (uint8_t)hour;
    t.min   = (uint8_t)min;
    t.sec   = (uint8_t)sec;
    t.dotw  = 1;

    PCF85063A_Set_All(t);

    printf("RTC actualizado OK\n");
}

// =========================
// CREATE
// =========================
lv_obj_t *screen_config_datetime_create(lv_obj_t *parent)
{
    const ui_theme_t *th = ui_theme_get();

    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, th->bg, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    // Card centrada, ancho fijo, alto automático
    lv_obj_t *card = lv_obj_create(root);
    lv_obj_set_size(card, 460, LV_SIZE_CONTENT);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, th->surface, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, th->border, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_shadow_width(card, 20, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_shadow_color(card, lv_color_black(), 0);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(card, 20, 0);
    lv_obj_set_style_pad_gap(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Título
    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, "Fecha y Hora");
    lv_obj_set_style_text_font(title, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, th->text, 0);

    // Label FECHA
    lv_obj_t *lbl_date = lv_label_create(card);
    lv_label_set_text(lbl_date, "FECHA");
    lv_obj_set_style_text_font(lbl_date, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_date, th->muted, 0);

    // Input FECHA
    ta_date = lv_textarea_create(card);
    lv_obj_set_width(ta_date, LV_PCT(100));
    lv_obj_set_height(ta_date, 52);
    lv_textarea_set_placeholder_text(ta_date, "YYYY.MM.DD");
    lv_obj_add_event_cb(ta_date, ta_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_radius(ta_date, 8, 0);
    lv_obj_set_style_bg_color(ta_date, th->surface2, 0);
    lv_obj_set_style_bg_opa(ta_date, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ta_date, th->border, 0);
    lv_obj_set_style_border_width(ta_date, 1, 0);
    lv_obj_set_style_text_color(ta_date, th->text, 0);
    lv_obj_set_style_text_font(ta_date, FONT_MEDIUM, 0);
    lv_obj_set_style_pad_left(ta_date, 14, 0);

    // Label HORA
    lv_obj_t *lbl_time = lv_label_create(card);
    lv_label_set_text(lbl_time, "HORA");
    lv_obj_set_style_text_font(lbl_time, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_time, th->muted, 0);

    // Input HORA
    ta_time = lv_textarea_create(card);
    lv_obj_set_width(ta_time, LV_PCT(100));
    lv_obj_set_height(ta_time, 52);
    lv_textarea_set_placeholder_text(ta_time, "HH.MM.SS");
    lv_obj_add_event_cb(ta_time, ta_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_radius(ta_time, 8, 0);
    lv_obj_set_style_bg_color(ta_time, th->surface2, 0);
    lv_obj_set_style_bg_opa(ta_time, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ta_time, th->border, 0);
    lv_obj_set_style_border_width(ta_time, 1, 0);
    lv_obj_set_style_text_color(ta_time, th->text, 0);
    lv_obj_set_style_text_font(ta_time, FONT_MEDIUM, 0);
    lv_obj_set_style_pad_left(ta_time, 14, 0);

    // Botón guardar
    lv_obj_t *btn = lv_btn_create(card);
    lv_obj_set_width(btn, LV_PCT(100));
    lv_obj_set_height(btn, 52);
    lv_obj_add_event_cb(btn, btn_save_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_bg_color(btn, th->blue, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, "Guardar");
    lv_obj_set_style_text_font(lbl, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);

    return root;
}

// =========================
// UPDATE
// =========================
void screen_config_datetime_update(void)
{
}
