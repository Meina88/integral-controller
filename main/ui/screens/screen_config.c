#include "screen_config.h"
#include "lvgl.h"
#include "drivers/rtc/rtc.h"
#include "drivers/rtc/rtc_pcf85063a.h"
#include <stdio.h>
#include "ui/components/numpad.h"

static lv_obj_t *root;
static lv_obj_t *ta_date;
static lv_obj_t *ta_time;

static void ta_event_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);

    numpad_open(ta, NULL); // NULL si no usás callback extra
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

    // =========================
    // PARSE FECHA
    // =========================
    if (sscanf(date, "%d.%d.%d", &year, &month, &day) != 3)
    {
        printf("ERROR fecha\n");
        return;
    }

    // =========================
    // PARSE HORA
    // =========================
    if (sscanf(time, "%d.%d.%d", &hour, &min, &sec) != 3)
    {
        printf("ERROR hora\n");
        return;
    }

    // =========================
    // ASIGNACIÓN SEGURA
    // =========================
    t.year = (uint16_t)year;
    t.month = (uint8_t)month;
    t.day = (uint8_t)day;

    t.hour = (uint8_t)hour;
    t.min = (uint8_t)min;
    t.sec = (uint8_t)sec;

    t.dotw = 1;

    PCF85063A_Set_All(t);

    printf("RTC actualizado OK\n");
}

// =========================
// CREATE
// =========================
lv_obj_t *screen_config_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));

    lv_obj_set_style_border_width(root, 0, 0);

    // 🔥 IMPORTANTE
    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(root, 20, 0);
    lv_obj_set_style_pad_gap(root, 10, 0);

    // =========================
    // TÍTULO
    // =========================
    lv_obj_t *title = lv_label_create(root);
    lv_label_set_text(title, "Configurar Fecha y Hora");

    // INPUT FECHA
    ta_date = lv_textarea_create(root);
    lv_obj_set_width(ta_date, LV_PCT(100));
    lv_textarea_set_placeholder_text(ta_date, "YYYY.MM.DD");
    lv_obj_add_event_cb(ta_date, ta_event_cb, LV_EVENT_CLICKED, NULL);

    // INPUT HORA
    ta_time = lv_textarea_create(root);
    lv_obj_set_width(ta_time, LV_PCT(100));
    lv_textarea_set_placeholder_text(ta_time, "HH.MM.SS");
    lv_obj_add_event_cb(ta_time, ta_event_cb, LV_EVENT_CLICKED, NULL);

    // BOTÓN GUARDAR
    lv_obj_t *btn = lv_btn_create(root);
    lv_obj_set_width(btn, LV_PCT(100));
    lv_obj_add_event_cb(btn, btn_save_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, "Guardar");
    lv_obj_center(lbl);

    return root;
}

// =========================
// UPDATE (opcional)
// =========================
void screen_config_update(void)
{
}