#include "screen_config.h"

#include "screen_config_datetime.h"
#include "screen_config_wifi.h"
#include "screen_config_machine.h"
#include "ui/fonts/fonts.h"
#include "lvgl.h"

// =========================
// OBJETOS
// =========================
static lv_obj_t *root;

static lv_obj_t *content;

static lv_obj_t *screen_datetime;
static lv_obj_t *screen_wifi;
static lv_obj_t *screen_machine;

// =========================
// SWITCH SCREEN
// =========================
static void switch_screen(int id)
{
    lv_obj_add_flag(screen_datetime, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(screen_wifi, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(screen_machine, LV_OBJ_FLAG_HIDDEN);

    if (id == 0)
        lv_obj_clear_flag(screen_datetime, LV_OBJ_FLAG_HIDDEN);

    if (id == 1)
        lv_obj_clear_flag(screen_wifi, LV_OBJ_FLAG_HIDDEN);

    if (id == 2)
        lv_obj_clear_flag(screen_machine, LV_OBJ_FLAG_HIDDEN);
}

// =========================
// EVENTS
// =========================
static void btn_datetime_cb(lv_event_t *e)
{
    switch_screen(0);
}

static void btn_wifi_cb(lv_event_t *e)
{
    switch_screen(1);
}

static void btn_machine_cb(lv_event_t *e)
{
    switch_screen(2);
}

// =========================
// CREATE
// =========================
lv_obj_t *screen_config_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);

    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));

    lv_obj_set_style_border_width(root, 0, 0);

    lv_obj_set_style_pad_all(root, 0, 0);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);

    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);

    // =========================
    // SIDEBAR
    // =========================
    lv_obj_t *sidebar = lv_obj_create(root);

    lv_obj_set_width(sidebar, 180);

    lv_obj_set_height(sidebar, LV_PCT(100));

    lv_obj_set_style_radius(sidebar, 0, 0);

    lv_obj_set_style_border_width(sidebar, 0, 0);

    lv_obj_set_style_bg_color(
        sidebar,
        lv_color_hex(0x161B20),
        0);

    lv_obj_set_style_pad_all(sidebar, 15, 0);

    lv_obj_set_style_pad_gap(sidebar, 12, 0);

    lv_obj_set_layout(sidebar, LV_LAYOUT_FLEX);

    lv_obj_set_flex_flow(sidebar, LV_FLEX_FLOW_COLUMN);

    // =========================
    // TITULO
    // =========================
    lv_obj_t *title = lv_label_create(sidebar);

    lv_label_set_text(title, "CONFIG");

    lv_obj_set_style_text_font(
        title,
        FONT_MEDIUM,
        0);

    lv_obj_set_style_text_color(
        title,
        lv_color_white(),
        0);

    // =========================
    // BOTONES
    // =========================

    // Fecha y Hora
    lv_obj_t *btn_datetime = lv_btn_create(sidebar);

    lv_obj_set_width(btn_datetime, LV_PCT(100));

    lv_obj_set_height(btn_datetime, 55);

    lv_obj_add_event_cb(
        btn_datetime,
        btn_datetime_cb,
        LV_EVENT_CLICKED,
        NULL);

    lv_obj_t *lbl_datetime = lv_label_create(btn_datetime);

    lv_label_set_text(lbl_datetime, "Fecha y hora");

    lv_obj_center(lbl_datetime);

    // WiFi
    lv_obj_t *btn_wifi = lv_btn_create(sidebar);

    lv_obj_set_width(btn_wifi, LV_PCT(100));

    lv_obj_set_height(btn_wifi, 55);

    lv_obj_add_event_cb(
        btn_wifi,
        btn_wifi_cb,
        LV_EVENT_CLICKED,
        NULL);

    lv_obj_t *lbl_wifi = lv_label_create(btn_wifi);

    lv_label_set_text(lbl_wifi, "WiFi");

    lv_obj_center(lbl_wifi);

    // Máquina
    lv_obj_t *btn_machine = lv_btn_create(sidebar);

    lv_obj_set_width(btn_machine, LV_PCT(100));

    lv_obj_set_height(btn_machine, 55);

    lv_obj_add_event_cb(
        btn_machine,
        btn_machine_cb,
        LV_EVENT_CLICKED,
        NULL);

    lv_obj_t *lbl_machine = lv_label_create(btn_machine);

    lv_label_set_text(lbl_machine, "Maquina");

    lv_obj_center(lbl_machine);

    // =========================
    // CONTENT
    // =========================
    content = lv_obj_create(root);

    lv_obj_set_flex_grow(content, 1);

    lv_obj_set_height(content, LV_PCT(100));

    lv_obj_set_style_border_width(content, 0, 0);

    lv_obj_set_style_pad_all(content, 0, 0);

    // =========================
    // SUBSCREENS
    // =========================
    screen_datetime = screen_config_datetime_create(content);

    screen_wifi = screen_config_wifi_create(content);

    screen_machine = screen_config_machine_create(content);

    // =========================
    // DEFAULT
    // =========================
    switch_screen(0);

    return root;
}

// =========================
// UPDATE
// =========================
void screen_config_update(void)
{
   // screen_config_datetime_update();
}