#include "ui_manager.h"
#include "screens/screen_extruir.h"
#include "screens/screen_setup.h"
#include "screens/screen_config.h"
#include "screens/screen_historicos.h"

static lv_obj_t *content;

static lv_obj_t *screen_extruir;
static lv_obj_t *screen_setup;
static lv_obj_t *screen_config;
static lv_obj_t *screen_historicos;

// botones
static lv_obj_t *sidebar;

// =========================
// SWITCH TAB
// =========================
static void switch_tab(int tab)
{
    lv_obj_add_flag(screen_extruir, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(screen_setup, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(screen_config, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(screen_historicos, LV_OBJ_FLAG_HIDDEN);

    if (tab == 0)
        lv_obj_clear_flag(screen_extruir, LV_OBJ_FLAG_HIDDEN);
    if (tab == 1)
        lv_obj_clear_flag(screen_setup, LV_OBJ_FLAG_HIDDEN);
    if (tab == 2)
        lv_obj_clear_flag(screen_config, LV_OBJ_FLAG_HIDDEN);
    if (tab == 3)
        lv_obj_clear_flag(screen_historicos, LV_OBJ_FLAG_HIDDEN);
}

// =========================
// EVENTOS BOTONES
// =========================
static void tab_extruir_cb(lv_event_t *e) { switch_tab(0); }
static void tab_setup_cb(lv_event_t *e) { switch_tab(1); }
static void tab_config_cb(lv_event_t *e) { switch_tab(2); }
static void tab_historicos_cb(lv_event_t *e) { switch_tab(3); }
// =========================
// UI START
// =========================
void ui_start(void)
{
    lv_obj_t *scr = lv_scr_act();

    // =========================
    // SIDEBAR
    // =========================
    sidebar = lv_obj_create(scr);
    lv_obj_set_size(sidebar, 120, LV_PCT(100));
    lv_obj_align(sidebar, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_set_style_bg_color(sidebar, lv_color_hex(0x101722), 0);
    lv_obj_set_style_border_width(sidebar, 0, 0);
    lv_obj_set_style_radius(sidebar, 0, 0);

    lv_obj_set_flex_flow(sidebar, LV_FLEX_FLOW_COLUMN);

    // =========================
    // BOTONES
    // =========================
    lv_obj_t *btn1 = lv_btn_create(sidebar);
    lv_obj_set_size(btn1, LV_PCT(100), 80);
    lv_obj_add_event_cb(btn1, tab_extruir_cb, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(btn1), "Extruir");

    lv_obj_t *btn2 = lv_btn_create(sidebar);
    lv_obj_set_size(btn2, LV_PCT(100), 80);
    lv_obj_add_event_cb(btn2, tab_setup_cb, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(btn2), "Setup");

    lv_obj_t *btn3 = lv_btn_create(sidebar);
    lv_obj_set_size(btn3, LV_PCT(100), 80);
    lv_obj_add_event_cb(btn3, tab_config_cb, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(btn3), "Config");

    lv_obj_t *btn4 = lv_btn_create(sidebar);
    lv_obj_set_size(btn4, LV_PCT(100), 80);
    lv_obj_add_event_cb(btn4, tab_historicos_cb, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(btn4), "Historicos");

    // =========================
    // CONTENT
    // =========================
    content = lv_obj_create(scr);
    lv_obj_set_size(content, 680, 480);
    lv_obj_align(content, LV_ALIGN_TOP_LEFT, 120, 0);

    lv_obj_set_style_border_width(content, 0, 0);

    // =========================
    // SCREENS
    // =========================
    screen_extruir = screen_extruir_create(content);
    screen_setup = screen_setup_create(content);
    screen_config = screen_config_create(content);
    screen_historicos = screen_historicos_create(content);

    switch_tab(0);
}

// =========================
// UPDATE
// =========================
void ui_update(void)
{
    screen_extruir_update();
}