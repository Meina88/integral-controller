#include "ui_manager.h"
#include "screens/screen_extruir.h"
#include "screens/screen_setup.h"
#include "screens/screen_config.h"
#include "screens/screen_historicos.h"
#include <string.h>
#include <stdio.h>
#include "logic/production.h"
#include "logic/alarm.h"
#include "drivers/rtc/rtc.h"
#include "logic/active_profile.h"
#include "screens/screen_config_wifi.h"
#include "ui/fonts/fonts.h"
#include "ui/fonts/fa_18.h"
#include "ui/ui_theme.h"
#include "comms/wifi/wifi_manager.h"

// ─── Paleta ────────────────────────────────────────────────────
#define C_BG_DARK      lv_color_hex(0x080E18)  // fondo sidebar / statusbar (más profundo)
#define C_SEPARATOR    lv_color_hex(0x1A2D45)  // líneas separadoras
#define C_TAB_ACTIVE   lv_color_hex(0x2563EB)  // fondo pestaña activa
#define C_TAB_ACCENT   lv_color_hex(0x93C5FD)  // borde derecho pestaña activa
#define C_TAB_TEXT_OFF lv_color_hex(0x94A3B8)  // texto pestaña inactiva (WCAG AA)
#define C_STATUS_OK    lv_color_hex(0x16A34A)  // badge verde "Listo"
#define C_STATUS_REC   lv_color_hex(0xDC2626)  // badge rojo "Grabando"
#define C_STATUS_ALARM lv_color_hex(0xD97706)  // badge ambar "Alarma"
#define C_TIME_TEXT    lv_color_hex(0xE2E8F0)  // texto hora (más legible)

typedef enum { STATUS_READY, STATUS_RECORDING, STATUS_ALARM } status_t;

// Íconos WiFi (FontAwesome, fuente fa_18)
#define WIFI_ICON_OK      "\xEF\x87\xAB"  // U+F1EB  fa-wifi
#define WIFI_ICON_LOCK    "\xEF\x80\xA3"  // U+F023  fa-lock     (contraseña incorrecta)
#define WIFI_ICON_EXCLAIM "\xEF\x81\xAA"  // U+F06A  fa-exclamation-circle  (sin AP)
#define WIFI_ICON_WARN    "\xEF\x81\xB1"  // U+F071  fa-warning  (error genérico)
#define WIFI_ICON_REFRESH "\xEF\x80\xA1"  // U+F021  fa-refresh  (reconectando)
#define WIFI_ICON_OFF     "\xEF\x80\x91"  // U+F011  fa-power-off (sin config)

typedef enum {
    WIFI_UI_NONE,       // sin SSID configurado
    WIFI_UI_CONNECTING, // tiene SSID, intentando conectar
    WIFI_UI_AUTH_ERROR, // contraseña incorrecta
    WIFI_UI_NO_AP,      // red no encontrada
    WIFI_UI_ERROR,      // otro error
    WIFI_UI_CONNECTED,  // conectado
} wifi_ui_state_t;

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
static lv_obj_t *time_digits[8];  // ranuras fijas: [H][H][:][M][M][:][S][S]
static lv_obj_t *label_wifi;

static status_t s_status              = STATUS_READY;
static int      s_wifi_state          = -1;
static uint32_t s_alarm_phase_start   = 0;
static bool     s_alarm_show_warning  = false;
static int      active_tab            = 0;

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
        snprintf(buf, sizeof(buf), "Seleccione un perfil");
        lv_obj_add_flag(btn_clear_profile, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text(label_profile, buf);
}

// =========================
// SWITCH TAB
// =========================
static void switch_tab(int tab)
{
    active_tab = tab;

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

void ui_navigate_to_extruir(void)
{
    switch_tab(0);
}

void ui_navigate_to_profiles(void)
{
    switch_tab(1);
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

static void tab_config_cb(lv_event_t *e)
{
    if (production_is_running())
    {
        printf("CONFIG BLOQUEADO: producción en curso\n");
        return;
    }
    switch_tab(2);
}
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
// UI REBUILD (theme switch)
// =========================
void ui_rebuild(void)
{
    // Re-apply the LVGL built-in widget theme with the updated dark/light flag
    // so that widgets like lv_scale, lv_btn, etc. adapt their inherited colors.
    lv_theme_t *lvgl_theme = lv_theme_default_init(
        lv_display_get_default(),
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        ui_theme_get()->id == UI_THEME_DARK,
        FONT_SMALL);
    lv_display_set_theme(lv_display_get_default(), lvgl_theme);

    lv_obj_clean(lv_layer_top());
    lv_obj_clean(content);

    screen_extruir    = screen_extruir_create(content);
    screen_setup      = screen_setup_create(content);
    screen_config     = screen_config_create(content);
    screen_historicos = screen_historicos_create(content);

    switch_tab(active_tab);

    if (active_tab == 2)
        screen_config_show_machine();
}

// =========================
// UI START
// =========================
void ui_start(void)
{
    ui_theme_init();

    lv_obj_t *scr = lv_scr_act();

    lv_theme_t *theme = lv_theme_default_init(
        lv_display_get_default(),
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        ui_theme_get()->id == UI_THEME_DARK,
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
    lv_obj_set_size(btn_clear_profile, 22, 22);
    lv_obj_set_style_radius(btn_clear_profile, 4, 0);
    lv_obj_set_style_bg_opa(btn_clear_profile, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_color(btn_clear_profile, lv_color_white(), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn_clear_profile, LV_OPA_20, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(btn_clear_profile, 0, 0);
    lv_obj_set_style_border_width(btn_clear_profile, 0, 0);
    lv_obj_set_style_pad_all(btn_clear_profile, 0, 0);
    lv_obj_add_event_cb(btn_clear_profile, clear_profile_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_x = lv_label_create(btn_clear_profile);
    lv_label_set_text(lbl_x, "×");
    lv_obj_set_style_text_font(lbl_x, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_x, ui_theme_get()->subtle, 0);
    lv_obj_center(lbl_x);

    lv_obj_add_flag(btn_clear_profile, LV_OBJ_FLAG_HIDDEN);

    // ── derecha: icono WiFi + hora en contenedor ancho fijo ──
    lv_obj_t *time_cont = make_invis(status_bar);
    lv_obj_set_size(time_cont, 130, LV_PCT(100));
    lv_obj_set_layout(time_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(time_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_cont,
        LV_FLEX_ALIGN_END,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(time_cont, 8, 0);

    // Cada posición del reloj tiene ancho fijo → los dígitos no desplazan el texto adyacente.
    // adv_w en inter_18: '0'=181, '4'=184 (~11.5 px) → ranura 12 px; ':'=79 (~5 px) → ranura 5 px.
    static const int8_t dw[8] = {12, 12, 5, 12, 12, 5, 12, 12};
    lv_obj_t *digit_row = make_invis(time_cont);
    lv_obj_set_size(digit_row, 82, LV_PCT(100));
    lv_obj_set_layout(digit_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(digit_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(digit_row,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(digit_row, 0, 0);
    for (int i = 0; i < 8; i++) {
        time_digits[i] = lv_label_create(digit_row);
        lv_obj_set_width(time_digits[i], dw[i]);
        lv_label_set_long_mode(time_digits[i], LV_LABEL_LONG_CLIP);
        lv_label_set_text(time_digits[i], i == 2 || i == 5 ? ":" : "-");
        lv_obj_set_style_text_color(time_digits[i], C_TIME_TEXT, 0);
        lv_obj_set_style_text_font(time_digits[i], FONT_SMALL, 0);
        lv_obj_set_style_text_align(time_digits[i], LV_TEXT_ALIGN_CENTER, 0);
    }

    label_wifi = lv_label_create(time_cont);
    lv_label_set_text(label_wifi, WIFI_ICON_OFF);
    lv_obj_set_style_text_color(label_wifi, lv_color_hex(0x6B7280), 0);
    lv_obj_set_style_text_font(label_wifi, &fa_18, 0);

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
    lv_obj_set_width(sidebar, 132);
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
    const char *names[]       = {"Extrudir", "Perfiles", "Ajustes", "Archivos"};
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
        // hover / press suave
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A2D45), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_PRESSED);

        // estilo activo (LV_STATE_CHECKED)
        lv_obj_set_style_bg_color(btn, C_TAB_ACTIVE, LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_CHECKED);
        lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_RIGHT, LV_STATE_CHECKED);
        lv_obj_set_style_border_width(btn, 3, LV_STATE_CHECKED);
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
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

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
    static char prev[9] = "--------";
    char buf[16];
    rtc_get_time_string(buf);
    for (int i = 0; i < 8; i++) {
        if (buf[i] != prev[i]) {
            prev[i] = buf[i];
            lv_label_set_text_fmt(time_digits[i], "%c", buf[i]);
        }
    }
}

// =========================
// UPDATE WIFI ICON
// =========================
static void update_wifi_icon(void)
{
    wifi_ui_state_t state;

    switch (wifi_get_state()) {
    case WIFI_STATE_CONNECTED:    state = WIFI_UI_CONNECTED;  break;
    case WIFI_STATE_AUTH_FAIL:    state = WIFI_UI_AUTH_ERROR; break;
    case WIFI_STATE_NO_AP:        state = WIFI_UI_NO_AP;      break;
    case WIFI_STATE_ERROR:        state = WIFI_UI_ERROR;      break;
    case WIFI_STATE_CONNECTING:   state = WIFI_UI_CONNECTING; break;
    case WIFI_STATE_DISCONNECTED: state = WIFI_UI_NONE;       break;
    default:                      state = WIFI_UI_NONE;       break;
    }

    if ((int)state == s_wifi_state) return;
    s_wifi_state = (int)state;

    const char *icon;
    lv_color_t  color;

    switch (state) {
    case WIFI_UI_CONNECTED:
        icon  = WIFI_ICON_OK;
        color = lv_color_white(); 
        break;
    case WIFI_UI_AUTH_ERROR:
        icon  = WIFI_ICON_LOCK;
        color = lv_color_hex(0xF97316);  // naranja (contraseña)
        break;
    case WIFI_UI_NO_AP:
        icon  = WIFI_ICON_EXCLAIM;
        color = lv_color_hex(0xEF4444);  // rojo (red no encontrada)
        break;
    case WIFI_UI_ERROR:
        icon  = WIFI_ICON_WARN;
        color = lv_color_hex(0xEF4444);  // rojo (error genérico)
        break;
    case WIFI_UI_CONNECTING:
        icon  = WIFI_ICON_REFRESH;
        color = lv_color_hex(0xFBBF24);  // amarillo (reconectando)
        break;
    default:
        icon  = WIFI_ICON_OFF;
        color = lv_color_hex(0x6B7280);  // gris (sin config)
        break;
    }

    lv_label_set_text(label_wifi, icon);
    lv_obj_set_style_text_color(label_wifi, color, 0);
}

// =========================
// UPDATE STATUS
// =========================
static void set_profile_label_normal(void)
{
    const char *code = active_profile_get();
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

static void update_status_label(void)
{
    bool running = production_is_running();
    bool alarm   = running && screen_extruir_is_alarm_active();

    status_t target = STATUS_READY;
    if (alarm)        target = STATUS_ALARM;
    else if (running) target = STATUS_RECORDING;

    if (target != s_status)
    {
        s_status = target;
        switch (target)
        {
        case STATUS_READY:
            lv_label_set_text(label_status, "Listo");
            lv_obj_set_style_bg_color(status_badge, C_STATUS_OK, 0);
            set_profile_label_normal();
            break;

        case STATUS_RECORDING:
            lv_label_set_text(label_status, "Grabando");
            lv_obj_set_style_bg_color(status_badge, C_STATUS_REC, 0);
            set_profile_label_normal();
            lv_obj_add_flag(btn_clear_profile, LV_OBJ_FLAG_HIDDEN);
            break;

        case STATUS_ALARM:
            lv_label_set_text(label_status, "Alarma");
            lv_obj_set_style_bg_color(status_badge, C_STATUS_ALARM, 0);
            alarm_trigger_speed();
            s_alarm_phase_start  = lv_tick_get();
            s_alarm_show_warning = true;
            lv_label_set_text(label_profile, "Velocidad fuera de rango");
            lv_obj_add_flag(btn_clear_profile, LV_OBJ_FLAG_HIDDEN);
            break;
        }
    }

    // Alternating center text during alarm (3s warning / 3s profile)
    if (s_status == STATUS_ALARM)
    {
        uint32_t elapsed     = lv_tick_get() - s_alarm_phase_start;
        bool should_warn     = (elapsed % 6000) < 3000;
        if (should_warn != s_alarm_show_warning)
        {
            s_alarm_show_warning = should_warn;
            if (should_warn)
            {
                lv_label_set_text(label_profile, "Velocidad fuera de rango");
                lv_obj_add_flag(btn_clear_profile, LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                set_profile_label_normal();
            }
        }
    }
}

// =========================
// UPDATE
// =========================
void ui_update(void)
{
    screen_extruir_update();
    if (active_tab == 2)
        screen_config_update();
    update_time_label();
    update_status_label();
    update_wifi_icon();
}
