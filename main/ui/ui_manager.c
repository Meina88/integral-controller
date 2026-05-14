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

// ─── Paleta ────────────────────────────────────────────────────
#define C_BG_DARK      lv_color_hex(0x0D1117)  // fondo sidebar / statusbar
#define C_SEPARATOR    lv_color_hex(0x1E293B)  // líneas separadoras
#define C_TAB_ACTIVE   lv_color_hex(0x1D4ED8)  // fondo pestaña activa
#define C_TAB_ACCENT   lv_color_hex(0x93C5FD)  // borde derecho pestaña activa
#define C_TAB_TEXT_OFF lv_color_hex(0x6B7280)  // texto pestaña inactiva
#define C_STATUS_OK    lv_color_hex(0x16A34A)  // badge verde "Listo"
#define C_STATUS_REC   lv_color_hex(0xDC2626)  // badge rojo "Grabando"
#define C_TIME_TEXT    lv_color_hex(0xD1D5DB)  // texto hora (gris suave)

static lv_obj_t *content;
static lv_obj_t *main_area;

static lv_obj_t *screen_extruir;
static lv_obj_t *screen_setup;
static lv_obj_t *screen_config;
static lv_obj_t *screen_historicos;

static lv_obj_t *sidebar;
static lv_obj_t *status_bar;
static lv_obj_t *tab_buttons[4];
static lv_obj_t *tab_labels[4];

static lv_obj_t *status_badge;
static lv_obj_t *label_status;
static lv_obj_t *label_profile;
static lv_obj_t *btn_clear_profile;
static lv_obj_t *label_time;

static bool last_production_state = false;

// =========================
// SET PROFILE
// =========================
void ui_set_active_profile(const char *code)
{
    char buf[64];

    if (code && strlen(code) > 0)
    {
        snprintf(buf, sizeof(buf), "Perfil: %s", code);
        lv_obj_clear_flag(btn_clear_profile, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        snprintf(buf, sizeof(buf), "Sin perfil");
        lv_obj_add_flag(btn_clear_profile, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text(label_profile, buf);
}

// =========================
// SWITCH TAB
// =========================
static void switch_tab(int tab)
{
    lv_obj_t *screens[] = {
        screen_extruir,
        screen_setup,
        screen_config,
        screen_historicos,
    };

    for (int i = 0; i < 4; i++)
        lv_obj_add_flag(screens[i], LV_OBJ_FLAG_HIDDEN);

    lv_obj_clear_flag(screens[tab], LV_OBJ_FLAG_HIDDEN);

    // visual de pestañas
    for (int i = 0; i < 4; i++)
    {
        if (i == tab)
        {
            lv_obj_add_state(tab_buttons[i], LV_STATE_CHECKED);
            lv_obj_set_style_text_color(tab_labels[i], lv_color_white(), 0);
        }
        else
        {
            lv_obj_clear_state(tab_buttons[i], LV_STATE_CHECKED);
            lv_obj_set_style_text_color(tab_labels[i], C_TAB_TEXT_OFF, 0);
        }
    }
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

static void tab_config_cb(lv_event_t *e)     { switch_tab(2); }
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
// HELPER: contenedor transparente sin bordes
// =========================
static lv_obj_t *make_invis(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

// =========================
// UI START
// =========================
void ui_start(void)
{
    lv_obj_t *scr = lv_scr_act();

    lv_theme_t *theme = lv_theme_default_init(
        lv_display_get_default(),
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        false,
        FONT_SMALL);

    lv_display_set_theme(lv_display_get_default(), theme);

    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(scr, C_BG_DARK, 0);

    // =========================
    // STATUS BAR
    // =========================
    status_bar = lv_obj_create(scr);
    lv_obj_set_size(status_bar, LV_PCT(100), 48);
    lv_obj_set_style_bg_color(status_bar, C_BG_DARK, 0);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_shadow_width(status_bar, 0, 0);
    lv_obj_set_style_pad_hor(status_bar, 14, 0);
    lv_obj_set_style_pad_ver(status_bar, 0, 0);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

    // separador inferior
    lv_obj_set_style_border_side(status_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(status_bar, 1, 0);
    lv_obj_set_style_border_color(status_bar, C_SEPARATOR, 0);

    lv_obj_set_layout(status_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(status_bar, 0, 0);

    // ── izquierda: badge de estado (ancho fijo → no desplaza nada) ──
    status_badge = lv_obj_create(status_bar);
    lv_obj_set_size(status_badge, 120, 30);
    lv_obj_set_style_radius(status_badge, 15, 0);
    lv_obj_set_style_bg_color(status_badge, C_STATUS_OK, 0);
    lv_obj_set_style_bg_opa(status_badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(status_badge, 0, 0);
    lv_obj_set_style_shadow_width(status_badge, 0, 0);
    lv_obj_set_style_pad_all(status_badge, 0, 0);
    lv_obj_clear_flag(status_badge, LV_OBJ_FLAG_SCROLLABLE);

    label_status = lv_label_create(status_badge);
    lv_label_set_text(label_status, "Listo");
    lv_obj_set_style_text_color(label_status, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_status, FONT_SMALL, 0);
    lv_obj_center(label_status);

    // ── centro: perfil + botón X (ocupa el espacio restante) ──
    lv_obj_t *center_cont = make_invis(status_bar);
    lv_obj_set_flex_grow(center_cont, 1);
    lv_obj_set_height(center_cont, LV_PCT(100));
    lv_obj_set_layout(center_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(center_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(center_cont,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(center_cont, 8, 0);

    label_profile = lv_label_create(center_cont);
    lv_label_set_text(label_profile, "Sin perfil");
    lv_obj_set_style_text_color(label_profile, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_profile, FONT_SMALL, 0);

    btn_clear_profile = lv_btn_create(center_cont);
    lv_obj_set_size(btn_clear_profile, 30, 30);
    lv_obj_set_style_radius(btn_clear_profile, 15, 0);
    lv_obj_set_style_bg_color(btn_clear_profile, lv_color_hex(0x374151), 0);
    lv_obj_set_style_shadow_width(btn_clear_profile, 0, 0);
    lv_obj_set_style_pad_all(btn_clear_profile, 0, 0);
    lv_obj_add_event_cb(btn_clear_profile, clear_profile_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_x = lv_label_create(btn_clear_profile);
    lv_label_set_text(lbl_x, "X");
    lv_obj_set_style_text_font(lbl_x, FONT_SMALL, 0);
    lv_obj_center(lbl_x);

    lv_obj_add_flag(btn_clear_profile, LV_OBJ_FLAG_HIDDEN);

    // ── derecha: hora en contenedor ancho fijo → no deforma el layout ──
    lv_obj_t *time_cont = make_invis(status_bar);
    lv_obj_set_size(time_cont, 100, LV_PCT(100));
    lv_obj_set_layout(time_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(time_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_cont,
        LV_FLEX_ALIGN_END,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);

    label_time = lv_label_create(time_cont);
    lv_label_set_text(label_time, "--:--:--");
    lv_obj_set_style_text_color(label_time, C_TIME_TEXT, 0);
    lv_obj_set_style_text_font(label_time, FONT_SMALL, 0);

    // =========================
    // MAIN AREA
    // =========================
    main_area = make_invis(scr);
    lv_obj_set_width(main_area, LV_PCT(100));
    lv_obj_set_flex_grow(main_area, 1);
    lv_obj_set_layout(main_area, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_area, LV_FLEX_FLOW_ROW);

    // =========================
    // SIDEBAR (pestañas)
    // =========================
    sidebar = lv_obj_create(main_area);
    lv_obj_set_width(sidebar, 120);
    lv_obj_set_height(sidebar, LV_PCT(100));
    lv_obj_set_style_bg_color(sidebar, C_BG_DARK, 0);
    lv_obj_set_style_bg_opa(sidebar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(sidebar, 0, 0);
    lv_obj_set_style_border_width(sidebar, 0, 0);
    lv_obj_set_style_shadow_width(sidebar, 0, 0);
    lv_obj_set_style_pad_all(sidebar, 0, 0);
    lv_obj_clear_flag(sidebar, LV_OBJ_FLAG_SCROLLABLE);

    // separador derecho del sidebar
    lv_obj_set_style_border_side(sidebar, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_border_width(sidebar, 1, 0);
    lv_obj_set_style_border_color(sidebar, C_SEPARATOR, 0);

    lv_obj_set_layout(sidebar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sidebar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(sidebar, 0, 0);

    // pestañas
    const char *names[]       = {"Extruir", "Perfiles", "Config", "Archivos"};
    lv_event_cb_t callbacks[] = {
        tab_extruir_cb,
        tab_setup_cb,
        tab_config_cb,
        tab_historicos_cb,
    };

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *btn = lv_btn_create(sidebar);
        tab_buttons[i] = btn;

        lv_obj_set_width(btn, LV_PCT(100));
        lv_obj_set_flex_grow(btn, 1);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);

        // estilo inactivo
        lv_obj_set_style_bg_color(btn, C_BG_DARK, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(btn, 0, 0);

        // estilo activo (LV_STATE_CHECKED)
        lv_obj_set_style_bg_color(btn, C_TAB_ACTIVE, LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_CHECKED);
        lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_RIGHT, LV_STATE_CHECKED);
        lv_obj_set_style_border_width(btn, 4, LV_STATE_CHECKED);
        lv_obj_set_style_border_color(btn, C_TAB_ACCENT, LV_STATE_CHECKED);

        lv_obj_add_event_cb(btn, callbacks[i], LV_EVENT_CLICKED, NULL);

        lv_obj_t *lbl = lv_label_create(btn);
        tab_labels[i] = lbl;
        lv_label_set_text(lbl, names[i]);
        lv_obj_set_style_text_font(lbl, FONT_SMALL, 0);
        lv_obj_set_style_text_color(lbl, C_TAB_TEXT_OFF, 0); // inactivo por defecto
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
    lv_obj_set_style_radius(content, 0, 0);

    // =========================
    // SCREENS
    // =========================
    screen_extruir    = screen_extruir_create(content);
    screen_setup      = screen_setup_create(content);
    screen_config     = screen_config_create(content);
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
// UPDATE STATUS
// =========================
static void update_status_label(void)
{
    bool running = production_is_running();
    if (running == last_production_state)
        return;

    last_production_state = running;

    if (running)
    {
        lv_label_set_text(label_status, "Grabando");
        lv_obj_set_style_bg_color(status_badge, C_STATUS_REC, 0);
    }
    else
    {
        lv_label_set_text(label_status, "Listo");
        lv_obj_set_style_bg_color(status_badge, C_STATUS_OK, 0);
    }
}

// =========================
// UPDATE
// =========================
void ui_update(void)
{
    screen_extruir_update();
    screen_config_wifi_update();
    update_time_label();
    update_status_label();
}
