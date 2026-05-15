#include "screen_config_wifi.h"
#include "comms/wifi/wifi_manager.h"
#include "storage/nvs/storage_nvs.h"
#include "ui/fonts/fonts.h"
#include "ui/ui_theme.h"
#include <stdbool.h>
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

#define C_BG      (ui_theme_get()->bg)
#define C_SURFACE (ui_theme_get()->surface)
#define C_SURFACE2 (ui_theme_get()->surface2)
#define C_PREVIEW (ui_theme_get()->preview)
#define C_BORDER  (ui_theme_get()->border)
#define C_PRESSED (ui_theme_get()->pressed)
#define C_MUTED   (ui_theme_get()->muted)
#define C_SUBTLE  (ui_theme_get()->subtle)
#define C_BLUE    (ui_theme_get()->blue)
#define C_GREEN   (ui_theme_get()->green)
#define C_RED     (ui_theme_get()->red)

static lv_obj_t *root;
static lv_obj_t *kb_panel;
static lv_obj_t *kb;
static lv_obj_t *preview_name_lbl;
static lv_obj_t *preview_value_lbl;
static lv_obj_t *btn_connect;
static lv_obj_t *label_btn_connect;
static lv_obj_t *ta_ssid;
static lv_obj_t *ta_password;
static lv_obj_t *label_status;
static lv_obj_t *label_ip;
static lv_obj_t *wifi_container;
static lv_obj_t *status_dot;

static char ssid_buffer[33] = {0};
static char pass_buffer[65] = {0};
static bool wifi_scan_loaded = false;
static bool wifi_error_visible = false;

static void load_wifi_list(void);

// =========================
// KEYBOARD
// =========================

// Fired when kb_panel is deleted (e.g. during ui_rebuild).
// Nulls all overlay statics so dangling-pointer callbacks bail out safely.
static void kb_panel_delete_cb(lv_event_t *e)
{
    kb_panel          = NULL;
    kb                = NULL;
    preview_name_lbl  = NULL;
    preview_value_lbl = NULL;
}

static void kb_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL)
    {
        if (!kb_panel || !lv_obj_is_valid(kb_panel))
            return;
        lv_obj_add_flag(kb_panel, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(kb, NULL);
    }
}

static void kb_overlay_click_cb(lv_event_t *e)
{
    if (!kb_panel || !lv_obj_is_valid(kb_panel))
        return;
    if (lv_event_get_target(e) == kb_panel)
    {
        lv_obj_add_flag(kb_panel, LV_OBJ_FLAG_HIDDEN);
        if (kb && lv_obj_is_valid(kb))
            lv_keyboard_set_textarea(kb, NULL);
    }
}

static void ta_changed_cb(lv_event_t *e)
{
    if (!kb_panel || !lv_obj_is_valid(kb_panel))
        return;
    if (!preview_value_lbl || !lv_obj_is_valid(preview_value_lbl))
        return;
    if (lv_obj_has_flag(kb_panel, LV_OBJ_FLAG_HIDDEN))
        return;

    lv_obj_t *ta = lv_event_get_target(e);
    lv_label_set_text(preview_value_lbl, lv_textarea_get_text(ta));
}

static void ta_click_cb(lv_event_t *e)
{
    if (!kb_panel          || !lv_obj_is_valid(kb_panel))
        return;
    if (!kb                || !lv_obj_is_valid(kb))
        return;
    if (!preview_name_lbl  || !lv_obj_is_valid(preview_name_lbl))
        return;
    if (!preview_value_lbl || !lv_obj_is_valid(preview_value_lbl))
        return;

    lv_obj_t *ta = lv_event_get_target(e);
    const char *name = (const char *)lv_event_get_user_data(e);

    lv_keyboard_set_textarea(kb, ta);
    lv_label_set_text(preview_name_lbl, name ? name : "");
    lv_label_set_text(preview_value_lbl, lv_textarea_get_text(ta));
    lv_obj_clear_flag(kb_panel, LV_OBJ_FLAG_HIDDEN);
}

// =========================
// WIFI EVENTS
// =========================
static void wifi_msgbox_event_cb(lv_event_t *e)
{
    wifi_error_visible = false;
}

static void wifi_network_event_cb(lv_event_t *e)
{
    const char *ssid = (const char *)lv_event_get_user_data(e);
    if (!ssid)
        return;
    strncpy(ssid_buffer, ssid, sizeof(ssid_buffer));
    lv_textarea_set_text(ta_ssid, ssid_buffer);
}

static void btn_connect_event_cb(lv_event_t *e)
{
    if (wifi_is_connected())
    {
        wifi_disconnect();
    }
    else
    {
        strncpy(ssid_buffer, lv_textarea_get_text(ta_ssid), sizeof(ssid_buffer));
        strncpy(pass_buffer, lv_textarea_get_text(ta_password), sizeof(pass_buffer));
        wifi_connect(ssid_buffer, pass_buffer);
    }
}

// =========================
// SCAN BUTTON
// =========================
static void scan_btn_event_cb(lv_event_t *e)
{
    wifi_scan_loaded = false;
    load_wifi_list();
    wifi_scan_loaded = true;
}

// =========================
// HELPER: input field
// =========================
static lv_obj_t *make_field(lv_obj_t *parent, const char *label_text, bool password)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, label_text);
    lv_obj_set_style_text_font(lbl, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl, C_SUBTLE, 0);
    lv_obj_set_width(lbl, LV_PCT(100));

    lv_obj_t *ta = lv_textarea_create(parent);
    lv_obj_set_width(ta, LV_PCT(100));
    lv_obj_set_height(ta, 48);
    lv_textarea_set_one_line(ta, true);
    if (password)
        lv_textarea_set_password_mode(ta, true);
    lv_obj_set_style_bg_color(ta, C_SURFACE, 0);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ta, C_BORDER, 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_radius(ta, 8, 0);
    lv_obj_set_style_text_font(ta, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(ta, ui_theme_get()->text, 0);
    lv_obj_set_style_pad_left(ta, 12, 0);

    lv_obj_add_event_cb(ta, ta_click_cb, LV_EVENT_CLICKED, (void *)label_text);
    lv_obj_add_event_cb(ta, ta_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
    return ta;
}

// =========================
// LOAD WIFI LIST
// =========================
static void load_wifi_list(void)
{
    if (!wifi_container)
        return;
    lv_obj_clean(wifi_container);
    wifi_start_scan();

    int count = wifi_get_scan_count();
    printf("Redes encontradas: %d\n", count);

    if (count == 0)
    {
        lv_obj_t *hint = lv_label_create(wifi_container);
        lv_label_set_text(hint, "Sin resultados.\nToca el boton Buscar.");
        lv_obj_set_style_text_font(hint, FONT_SMALL, 0);
        lv_obj_set_style_text_color(hint, C_MUTED, 0);
        lv_obj_set_width(hint, LV_PCT(100));
        return;
    }

    for (int i = 0; i < count; i++)
    {
        const char *ssid = wifi_get_scan_ssid(i);
        if (!ssid || strlen(ssid) == 0)
            continue;

        lv_obj_t *card = lv_obj_create(wifi_container);
        lv_obj_set_width(card, LV_PCT(100));
        lv_obj_set_height(card, 44);
        lv_obj_set_style_bg_color(card, C_SURFACE, 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(card, C_PRESSED, LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_STATE_PRESSED);
        lv_obj_set_style_border_color(card, C_BORDER, 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 6, 0);
        lv_obj_set_style_pad_hor(card, 12, 0);
        lv_obj_set_style_pad_ver(card, 0, 0);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl = lv_label_create(card);
        lv_label_set_text(lbl, ssid);
        lv_obj_set_style_text_font(lbl, FONT_SMALL, 0);
        lv_obj_set_style_text_color(lbl, ui_theme_get()->text, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_add_event_cb(card, wifi_network_event_cb, LV_EVENT_CLICKED, (void *)ssid);
    }
}

// =========================
// CREATE
// =========================
lv_obj_t *screen_config_wifi_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, C_BG, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    // =========================
    // LEFT: network list
    // =========================
    lv_obj_t *left = lv_obj_create(root);
    lv_obj_set_size(left, 220, LV_PCT(100));
    lv_obj_set_style_bg_color(left, C_SURFACE2, 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(left, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_border_color(left, C_BORDER, 0);
    lv_obj_set_style_border_width(left, 1, 0);
    lv_obj_set_style_radius(left, 0, 0);
    lv_obj_set_style_pad_all(left, 12, 0);
    lv_obj_set_style_pad_gap(left, 8, 0);
    lv_obj_set_layout(left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    // Encabezado: label + botón escanear
    lv_obj_t *net_header = lv_obj_create(left);
    lv_obj_set_width(net_header, LV_PCT(100));
    lv_obj_set_height(net_header, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(net_header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(net_header, 0, 0);
    lv_obj_set_style_pad_all(net_header, 0, 0);
    lv_obj_set_style_pad_gap(net_header, 6, 0);
    lv_obj_set_flex_flow(net_header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(net_header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(net_header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *net_lbl = lv_label_create(net_header);
    lv_label_set_text(net_lbl, "Redes disponibles");
    lv_obj_set_style_text_font(net_lbl, FONT_SMALL, 0);
    lv_obj_set_style_text_color(net_lbl, C_MUTED, 0);
    lv_obj_set_flex_grow(net_lbl, 1);

    lv_obj_t *btn_scan = lv_btn_create(net_header);
    lv_obj_set_size(btn_scan, 32, 28);
    lv_obj_set_style_bg_color(btn_scan, C_SURFACE, 0);
    lv_obj_set_style_bg_opa(btn_scan, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(btn_scan, C_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn_scan, C_BORDER, 0);
    lv_obj_set_style_border_width(btn_scan, 1, 0);
    lv_obj_set_style_radius(btn_scan, 6, 0);
    lv_obj_set_style_shadow_width(btn_scan, 0, 0);
    lv_obj_set_style_pad_all(btn_scan, 0, 0);
    lv_obj_add_event_cb(btn_scan, scan_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *scan_icon = lv_label_create(btn_scan);
    lv_label_set_text(scan_icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(scan_icon, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(scan_icon, C_SUBTLE, 0);
    lv_obj_center(scan_icon);

    wifi_container = lv_obj_create(left);
    lv_obj_set_width(wifi_container, LV_PCT(100));
    lv_obj_set_flex_grow(wifi_container, 1);
    lv_obj_set_style_bg_opa(wifi_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wifi_container, 0, 0);
    lv_obj_set_style_pad_all(wifi_container, 0, 0);
    lv_obj_set_style_pad_right(wifi_container, 12, 0);
    lv_obj_set_style_pad_gap(wifi_container, 6, 0);
    lv_obj_set_layout(wifi_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(wifi_container, LV_FLEX_FLOW_COLUMN);

    // =========================
    // RIGHT: credentials form
    // =========================
    lv_obj_t *right = lv_obj_create(root);
    lv_obj_set_flex_grow(right, 1);
    lv_obj_set_height(right, LV_PCT(100));
    lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right, 0, 0);
    lv_obj_set_style_radius(right, 0, 0);
    lv_obj_set_style_pad_all(right, 20, 0);
    lv_obj_set_style_pad_gap(right, 10, 0);
    lv_obj_set_layout(right, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cred_lbl = lv_label_create(right);
    lv_label_set_text(cred_lbl, "Credenciales");
    lv_obj_set_style_text_font(cred_lbl, FONT_SMALL, 0);
    lv_obj_set_style_text_color(cred_lbl, C_MUTED, 0);

    storage_nvs_load_wifi(ssid_buffer, sizeof(ssid_buffer),
                          pass_buffer, sizeof(pass_buffer));

    ta_ssid = make_field(right, "Red (SSID)", false);
    lv_textarea_set_text(ta_ssid, ssid_buffer);

    ta_password = make_field(right, "Contrasena", true);
    lv_textarea_set_text(ta_password, pass_buffer);

    btn_connect = lv_btn_create(right);
    lv_obj_set_width(btn_connect, LV_PCT(100));
    lv_obj_set_height(btn_connect, 52);
    lv_obj_set_style_bg_color(btn_connect, C_BLUE, 0);
    lv_obj_set_style_bg_opa(btn_connect, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(btn_connect, C_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn_connect, 0, 0);
    lv_obj_set_style_shadow_width(btn_connect, 0, 0);
    lv_obj_set_style_radius(btn_connect, 8, 0);
    lv_obj_add_event_cb(btn_connect, btn_connect_event_cb, LV_EVENT_CLICKED, NULL);

    label_btn_connect = lv_label_create(btn_connect);
    lv_label_set_text(label_btn_connect, "Conectar");
    lv_obj_set_style_text_font(label_btn_connect, FONT_MEDIUM, 0);
    lv_obj_center(label_btn_connect);

    // Spacer
    lv_obj_t *spacer = lv_obj_create(right);
    lv_obj_set_width(spacer, LV_PCT(100));
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_style_pad_all(spacer, 0, 0);

    // Status row
    lv_obj_t *status_row = lv_obj_create(right);
    lv_obj_set_width(status_row, LV_PCT(100));
    lv_obj_set_height(status_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(status_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status_row, 0, 0);
    lv_obj_set_style_pad_all(status_row, 0, 0);
    lv_obj_set_style_pad_gap(status_row, 8, 0);
    lv_obj_set_flex_flow(status_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(status_row, LV_OBJ_FLAG_SCROLLABLE);

    status_dot = lv_obj_create(status_row);
    lv_obj_set_size(status_dot, 10, 10);
    lv_obj_set_style_bg_color(status_dot, C_RED, 0);
    lv_obj_set_style_bg_opa(status_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(status_dot, 0, 0);
    lv_obj_set_style_radius(status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(status_dot, 0, 0);
    lv_obj_clear_flag(status_dot, LV_OBJ_FLAG_SCROLLABLE);

    label_status = lv_label_create(status_row);
    lv_label_set_text(label_status, "Desconectado");
    lv_obj_set_style_text_font(label_status, FONT_SMALL, 0);
    lv_obj_set_style_text_color(label_status, C_SUBTLE, 0);

    label_ip = lv_label_create(right);
    lv_label_set_text(label_ip, "IP: 0.0.0.0");
    lv_obj_set_style_text_font(label_ip, FONT_SMALL, 0);
    lv_obj_set_style_text_color(label_ip, C_MUTED, 0);

    // =========================
    // KB PANEL: overlay full-screen con flex-column + spacer → preview + teclado al fondo
    // =========================
    kb_panel = lv_obj_create(lv_layer_top());
    lv_obj_move_foreground(kb_panel);
    lv_obj_add_flag(kb_panel, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_CLICKABLE);
    // Null out statics before LVGL frees child objects (prevents dangling-ptr callbacks)
    lv_obj_add_event_cb(kb_panel, kb_panel_delete_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_size(kb_panel, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(kb_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(kb_panel, 0, 0);
    lv_obj_set_style_radius(kb_panel, 0, 0);
    lv_obj_set_style_pad_all(kb_panel, 0, 0);
    lv_obj_set_style_pad_gap(kb_panel, 0, 0);
    lv_obj_set_layout(kb_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(kb_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(kb_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(kb_panel, kb_overlay_click_cb, LV_EVENT_CLICKED, NULL);

    // Spacer: no clickeable → los toques en el area vacia llegan a kb_panel y cierran el teclado
    lv_obj_t *kb_spacer = lv_obj_create(kb_panel);
    lv_obj_set_width(kb_spacer, LV_PCT(100));
    lv_obj_set_height(kb_spacer, 10);
    lv_obj_set_flex_grow(kb_spacer, 1);
    lv_obj_set_style_bg_opa(kb_spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(kb_spacer, 0, 0);
    lv_obj_set_style_pad_all(kb_spacer, 0, 0);
    lv_obj_clear_flag(kb_spacer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(kb_spacer, LV_OBJ_FLAG_CLICKABLE);

    // Preview bar
    lv_obj_t *preview = lv_obj_create(kb_panel);
    lv_obj_set_width(preview, LV_PCT(100));
    lv_obj_set_height(preview, 52);
    lv_obj_set_style_bg_color(preview, C_PREVIEW, 0);
    lv_obj_set_style_bg_opa(preview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(preview, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(preview, C_BORDER, 0);
    lv_obj_set_style_border_width(preview, 1, 0);
    lv_obj_set_style_radius(preview, 0, 0);
    lv_obj_set_style_pad_hor(preview, 16, 0);
    lv_obj_set_style_pad_ver(preview, 0, 0);
    lv_obj_set_style_pad_gap(preview, 12, 0);
    lv_obj_set_flex_flow(preview, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(preview, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(preview, LV_OBJ_FLAG_SCROLLABLE);

    preview_name_lbl = lv_label_create(preview);
    lv_label_set_text(preview_name_lbl, "");
    lv_obj_set_width(preview_name_lbl, 120);
    lv_obj_set_style_text_font(preview_name_lbl, FONT_SMALL, 0);
    lv_obj_set_style_text_color(preview_name_lbl, C_MUTED, 0);

    lv_obj_t *vsep = lv_obj_create(preview);
    lv_obj_set_size(vsep, 1, 28);
    lv_obj_set_style_bg_color(vsep, C_BORDER, 0);
    lv_obj_set_style_bg_opa(vsep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(vsep, 0, 0);
    lv_obj_set_style_radius(vsep, 0, 0);
    lv_obj_set_style_pad_all(vsep, 0, 0);
    lv_obj_clear_flag(vsep, LV_OBJ_FLAG_SCROLLABLE);

    preview_value_lbl = lv_label_create(preview);
    lv_label_set_text(preview_value_lbl, "");
    lv_obj_set_flex_grow(preview_value_lbl, 1);
    lv_obj_set_style_text_font(preview_value_lbl, FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(preview_value_lbl, ui_theme_get()->text, 0);

    // Teclado
    kb = lv_keyboard_create(kb_panel);
    lv_obj_set_size(kb, LV_PCT(100), 220);
    lv_obj_set_style_text_font(
        kb,
        &lv_font_montserrat_20,
        LV_PART_ITEMS);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_popovers(kb, true);

    lv_obj_set_style_bg_color(kb, ui_theme_get()->surface2, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(kb, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(kb, 0, LV_PART_MAIN);

    lv_obj_set_style_bg_color(kb, C_SURFACE, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(kb, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_text_color(kb, ui_theme_get()->text, LV_PART_ITEMS);
    lv_obj_set_style_border_color(kb, C_BORDER, LV_PART_ITEMS);
    lv_obj_set_style_border_width(kb, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(kb, 4, LV_PART_ITEMS);

    lv_obj_set_style_bg_color(kb, C_BLUE, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(kb, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_PRESSED);

    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, NULL);

    return root;
}

// =========================
// UPDATE
// =========================
void screen_config_wifi_update(void)
{
    if (!wifi_scan_loaded)
    {
        wifi_scan_loaded = true;
        load_wifi_list();
    }

    // Guard: objects may be temporarily NULL between lv_obj_clean() and screen recreate
    if (!label_btn_connect || !lv_obj_is_valid(label_btn_connect))
        return;

    bool connected = wifi_is_connected();

    lv_label_set_text(label_btn_connect, connected ? "Desconectar" : "Conectar");
    lv_label_set_text(label_status, connected ? "Conectado" : "Desconectado");
    lv_obj_set_style_bg_color(status_dot, connected ? C_GREEN : C_RED, 0);
    lv_obj_set_style_text_color(label_status, connected ? C_GREEN : C_SUBTLE, 0);

    char ip_buf[32];
    snprintf(ip_buf, sizeof(ip_buf), "IP: %s", wifi_get_ip_string());
    lv_label_set_text(label_ip, ip_buf);

    const char *err = wifi_get_last_error();
    if (!wifi_error_visible && err && strlen(err) > 0)
    {
        wifi_error_visible = true;
        lv_obj_t *mbox = lv_msgbox_create(NULL);
        lv_msgbox_add_title(mbox, "WiFi");
        lv_msgbox_add_text(mbox, err);
        lv_msgbox_add_close_button(mbox);
        lv_obj_center(mbox);
        lv_obj_set_style_shadow_width(mbox, 0, 0);
        lv_obj_set_style_radius(mbox, 8, 0);
        wifi_clear_last_error();
        lv_obj_add_event_cb(mbox, wifi_msgbox_event_cb, LV_EVENT_DELETE, NULL);
    }
}
