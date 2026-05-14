#include "ui_manager.h"
#include "screens/screen_extruir.h"
#include "screens/screen_setup.h"
#include "screens/screen_config.h"
#include "screens/screen_historicos.h"
#include <string.h>
#include <stdio.h>
#include "logic/production.h"
#include "drivers/rtc/rtc.h"
#include "logic/active_profile.h"
#include "screens/screen_config_wifi.h"
#include "ui/fonts/fonts.h"

static lv_obj_t *content;
static lv_obj_t *main_area;

static lv_obj_t *screen_extruir;
static lv_obj_t *screen_setup;
static lv_obj_t *screen_config;
static lv_obj_t *screen_historicos;

// contenedores
static lv_obj_t *sidebar;
static lv_obj_t *status_bar;

// labels status bar
static lv_obj_t *label_status;
static lv_obj_t *label_profile;
static lv_obj_t *btn_clear_profile;
static lv_obj_t *label_time;

// =========================
// SET PROFILE (🔥 usada globalmente)
// =========================
void ui_set_active_profile(const char *code)
{
    char buf[64];

    if (code && strlen(code) > 0)
    {
        snprintf(buf, sizeof(buf), "Perfil: %s", code);

        lv_obj_clear_flag(
            btn_clear_profile,
            LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        snprintf(buf, sizeof(buf), "Perfil: ninguno");

        lv_obj_add_flag(
            btn_clear_profile,
            LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text(label_profile, buf);
}

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
static void tab_extruir_cb(lv_event_t *e)
{
    screen_extruir_refresh_profile();
    switch_tab(0);
}
static void tab_setup_cb(lv_event_t *e)
{
    if (production_is_running())
    {
        printf("SETUP BLOQUEADO: producción en curso\n");
        return;
    }

    switch_tab(1);
}
static void tab_config_cb(lv_event_t *e) { switch_tab(2); }
static void tab_historicos_cb(lv_event_t *e) { switch_tab(3); }

// =========================
// CLEAR PROFILE
// =========================
static void clear_profile_cb(lv_event_t *e)
{
    if (production_is_running())
    {
        printf("No se puede liberar perfil durante grabacion\n");
        return;
    }

    active_profile_set("");

    ui_set_active_profile(NULL);

    screen_extruir_refresh_profile();

    printf("Perfil liberado\n");
}

// =========================
// UI START
// =========================
void ui_start(void)
{
    lv_obj_t *scr = lv_scr_act();

    // =========================
    // THEME + FONT DEFAULT
    // =========================
    lv_theme_t *theme = lv_theme_default_init(
        lv_display_get_default(),
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        false,
        FONT_SMALL);

    lv_display_set_theme(
        lv_display_get_default(),
        theme);

    // 🔥 layout raíz
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);

    // =========================
    // STATUS BAR
    // =========================
    status_bar = lv_obj_create(scr);
    lv_obj_set_size(status_bar, LV_PCT(100), 40);

    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_pad_all(status_bar, 10, 0);

    // 🔥 FLEX HORIZONTAL
    lv_obj_set_layout(status_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    // izquierda
    label_status = lv_label_create(status_bar);
    lv_label_set_text(label_status, "Listo");
    lv_obj_set_style_text_color(label_status, lv_color_white(), 0);

    // =========================
    // PROFILE CONTAINER
    // =========================
    lv_obj_t *profile_container = lv_obj_create(status_bar);

    lv_obj_set_size(
        profile_container,
        LV_SIZE_CONTENT,
        LV_SIZE_CONTENT);

    lv_obj_set_style_bg_opa(profile_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(profile_container, 0, 0);
    lv_obj_set_style_pad_all(profile_container, 0, 0);
    lv_obj_set_style_pad_gap(profile_container, 6, 0);

    lv_obj_set_layout(profile_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(profile_container, LV_FLEX_FLOW_ROW);

    lv_obj_clear_flag(profile_container, LV_OBJ_FLAG_SCROLLABLE);

    // label perfil
    label_profile = lv_label_create(profile_container);
    lv_label_set_text(label_profile, "Perfil: ninguno");
    lv_obj_set_style_text_color(label_profile, lv_color_white(), 0);

    // botón liberar
    btn_clear_profile = lv_btn_create(profile_container);

    lv_obj_set_size(btn_clear_profile, 28, 28);

    lv_obj_add_event_cb(
        btn_clear_profile,
        clear_profile_cb,
        LV_EVENT_CLICKED,
        NULL);

    lv_obj_t *lbl_x = lv_label_create(btn_clear_profile);
    lv_label_set_text(lbl_x, "X");
    lv_obj_center(lbl_x);

    // ocultar inicialmente
    lv_obj_add_flag(btn_clear_profile, LV_OBJ_FLAG_HIDDEN);

    // derecha: hora actual
    label_time = lv_label_create(status_bar);
    lv_label_set_text(label_time, "--:--:--");
    lv_obj_set_style_text_color(label_time, lv_color_white(), 0);

    // =========================
    // MAIN AREA
    // =========================
    main_area = lv_obj_create(scr);
    lv_obj_set_width(main_area, LV_PCT(100));
    lv_obj_set_flex_grow(main_area, 1);

    lv_obj_set_style_border_width(main_area, 0, 0);
    lv_obj_set_style_pad_all(main_area, 0, 0);

    lv_obj_set_layout(main_area, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_area, LV_FLEX_FLOW_ROW);

    lv_obj_clear_flag(main_area, LV_OBJ_FLAG_SCROLLABLE);

    // =========================
    // SIDEBAR
    // =========================
    sidebar = lv_obj_create(main_area);
    lv_obj_set_width(sidebar, 110);
    lv_obj_set_height(sidebar, LV_PCT(100));

    lv_obj_set_style_bg_opa(sidebar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sidebar, 0, 0);
    lv_obj_set_style_pad_all(sidebar, 0, 0);
    lv_obj_set_style_pad_gap(sidebar, 0, 0);

    lv_obj_set_layout(sidebar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sidebar, LV_FLEX_FLOW_COLUMN);

    lv_obj_clear_flag(sidebar, LV_OBJ_FLAG_SCROLLABLE);

    // botones
    const char *names[] = {"Extruir", "Perfiles", "Setup", "Archivos"};
    lv_event_cb_t callbacks[] = {tab_extruir_cb, tab_setup_cb, tab_config_cb, tab_historicos_cb};

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *btn = lv_btn_create(sidebar);
        lv_obj_set_width(btn, LV_PCT(100));
        lv_obj_set_flex_grow(btn, 1);
        lv_obj_add_event_cb(btn, callbacks[i], LV_EVENT_CLICKED, NULL);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, names[i]);
        lv_obj_center(lbl);
    }

    // =========================
    // CONTENT
    // =========================
    content = lv_obj_create(main_area);
    lv_obj_set_height(content, LV_PCT(100));
    lv_obj_set_flex_grow(content, 1);

    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);

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
// UPDATE TIME
// =========================

static void update_time_label(void)
{
    char buf[16];
    rtc_get_time_string(buf);

    lv_label_set_text(label_time, buf);
}

// =========================
// UPDATE
// =========================
void ui_update(void)
{
    screen_extruir_update();
    screen_config_wifi_update();

    update_time_label();
}
