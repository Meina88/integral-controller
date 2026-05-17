#include "screen_config_datetime.h"

#include "lvgl.h"
#include "drivers/rtc/rtc.h"
#include "drivers/rtc/rtc_pcf85063a.h"

#include "ui/fonts/fonts.h"
#include "ui/ui_theme.h"

#include <stdio.h>
#include <string.h>

static lv_obj_t *root;
static lv_obj_t *roller_hour;
static lv_obj_t *roller_min;
static lv_obj_t *roller_sec;
static lv_obj_t *roller_day;
static lv_obj_t *roller_month;
static lv_obj_t *roller_year;

#define YEAR_BASE  2024
#define YEAR_COUNT 12

static char opts_hh[24 * 3 + 1];
static char opts_mm[60 * 3 + 1];
static char opts_dd[31 * 3 + 1];
static char opts_yy[YEAR_COUNT * 5 + 1];

// =========================
// BUILD OPTIONS
// =========================
static void build_nn_opts(char *buf, int first, int count)
{
    buf[0] = '\0';
    for (int i = 0; i < count; i++)
    {
        char tmp[8];
        snprintf(tmp, sizeof(tmp), i < count - 1 ? "%02d\n" : "%02d", first + i);
        strcat(buf, tmp);
    }
}

static void build_year_opts(char *buf)
{
    buf[0] = '\0';
    for (int i = 0; i < YEAR_COUNT; i++)
    {
        char tmp[8];
        snprintf(tmp, sizeof(tmp), i < YEAR_COUNT - 1 ? "%d\n" : "%d", YEAR_BASE + i);
        strcat(buf, tmp);
    }
}

// =========================
// BOTÓN ACTUALIZAR
// =========================
static void btn_save_cb(lv_event_t *e)
{
    (void)e;
    datetime_t t;
    t.hour  = (uint8_t) lv_roller_get_selected(roller_hour);
    t.min   = (uint8_t) lv_roller_get_selected(roller_min);
    t.sec   = (uint8_t) lv_roller_get_selected(roller_sec);
    t.day   = (uint8_t)(lv_roller_get_selected(roller_day) + 1);
    t.month = (uint8_t)(lv_roller_get_selected(roller_month) + 1);
    t.year  = (uint16_t)(lv_roller_get_selected(roller_year) + YEAR_BASE);
    t.dotw  = 1;

    PCF85063A_Set_All(t);

    printf("RTC actualizado: %04d-%02d-%02d %02d:%02d:%02d\n",
           t.year, t.month, t.day, t.hour, t.min, t.sec);
}

// =========================
// HELPERS UI
// =========================
static lv_obj_t *make_roller(lv_obj_t *parent, const char *opts,
                              uint32_t initial, lv_roller_mode_t mode)
{
    const ui_theme_t *th = ui_theme_get();

    lv_obj_t *r = lv_roller_create(parent);
    lv_roller_set_options(r, opts, mode);
    lv_roller_set_visible_row_count(r, 2);
    lv_roller_set_selected(r, initial, LV_ANIM_OFF);
    lv_obj_set_flex_grow(r, 1);
    lv_obj_set_height(r, LV_PCT(100));

    lv_obj_set_style_bg_color(r, th->surface, 0);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(r, th->border, 0);
    lv_obj_set_style_border_width(r, 1, 0);
    lv_obj_set_style_radius(r, 8, 0);
    lv_obj_set_style_shadow_width(r, 0, 0);
    lv_obj_set_style_text_color(r, th->subtle, 0);
    lv_obj_set_style_text_font(r, FONT_MEDIUM, 0);
    lv_obj_set_style_text_line_space(r, 4, 0);
    lv_obj_set_style_pad_ver(r, 4, 0);

    lv_obj_set_style_bg_color(r, th->blue, LV_PART_SELECTED);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, LV_PART_SELECTED);
    lv_obj_set_style_text_color(r, lv_color_white(), LV_PART_SELECTED);
    lv_obj_set_style_text_font(r, FONT_MEDIUM, LV_PART_SELECTED);
    lv_obj_set_style_radius(r, 6, LV_PART_SELECTED);
    lv_obj_set_style_border_width(r, 0, LV_PART_SELECTED);

    return r;
}

static lv_obj_t *make_roller_row(lv_obj_t *parent)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_flex_grow(row, 1);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_shadow_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row, 10, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    return row;
}

static void make_sep(lv_obj_t *parent, const char *txt)
{
    const ui_theme_t *th = ui_theme_get();
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_text_color(lbl, th->muted, 0);
    lv_obj_set_style_text_font(lbl, FONT_MEDIUM, 0);
}

static void make_section_label(lv_obj_t *parent, const char *txt)
{
    const ui_theme_t *th = ui_theme_get();
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_text_font(lbl, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl, th->muted, 0);
}

// =========================
// CREATE
// =========================
lv_obj_t *screen_config_datetime_create(lv_obj_t *parent)
{
    const ui_theme_t *th = ui_theme_get();

    build_nn_opts(opts_hh, 0, 24);
    build_nn_opts(opts_mm, 0, 60);
    build_nn_opts(opts_dd, 1, 31);
    build_year_opts(opts_yy);

    datetime_t now = {0};
    rtc_get_datetime(&now);

    uint32_t init_hour  = now.hour  < 24 ? now.hour : 0;
    uint32_t init_min   = now.min   < 60 ? now.min  : 0;
    uint32_t init_sec   = now.sec   < 60 ? now.sec  : 0;
    uint32_t init_day   = (now.day  >= 1 && now.day  <= 31) ? now.day  - 1 : 0;
    uint32_t init_month = (now.month >= 1 && now.month <= 12) ? now.month - 1 : 0;
    uint32_t init_year  = (now.year >= YEAR_BASE && now.year < YEAR_BASE + YEAR_COUNT)
                          ? now.year - YEAR_BASE : 0;

    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, th->bg, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_hor(root, 24, 0);
    lv_obj_set_style_pad_ver(root, 16, 0);
    lv_obj_set_style_pad_gap(root, 8, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    // ── FECHA ─────────────────────────────────────────────
    make_section_label(root, "FECHA");

    lv_obj_t *row_date = make_roller_row(root);
    roller_day   = make_roller(row_date, opts_dd, init_day, LV_ROLLER_MODE_INFINITE);
    make_sep(row_date, "/");
    roller_month = make_roller(row_date,
                               "Ene\nFeb\nMar\nAbr\nMay\nJun"
                               "\nJul\nAgo\nSep\nOct\nNov\nDic",
                               init_month, LV_ROLLER_MODE_INFINITE);
    make_sep(row_date, "/");
    roller_year  = make_roller(row_date, opts_yy, init_year, LV_ROLLER_MODE_NORMAL);

    // ── HORA ──────────────────────────────────────────────
    make_section_label(root, "HORA");

    lv_obj_t *row_time = make_roller_row(root);
    roller_hour = make_roller(row_time, opts_hh, init_hour, LV_ROLLER_MODE_INFINITE);
    make_sep(row_time, ":");
    roller_min  = make_roller(row_time, opts_mm, init_min,  LV_ROLLER_MODE_INFINITE);
    make_sep(row_time, ":");
    roller_sec  = make_roller(row_time, opts_mm, init_sec,  LV_ROLLER_MODE_INFINITE);

    // ── Botón Actualizar ──────────────────────────────────
    lv_obj_t *btn = lv_btn_create(root);
    lv_obj_set_width(btn, LV_PCT(100));
    lv_obj_set_height(btn, 44);
    lv_obj_add_event_cb(btn, btn_save_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_bg_color(btn, th->blue, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

    lv_obj_t *lbl_btn = lv_label_create(btn);
    lv_label_set_text(lbl_btn, "Actualizar");
    lv_obj_set_style_text_font(lbl_btn, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl_btn, lv_color_white(), 0);
    lv_obj_center(lbl_btn);

    return root;
}

// =========================
// UPDATE
// =========================
void screen_config_datetime_update(void)
{
}
