#include "screen_config_wifi.h"

#include "comms/wifi/wifi_manager.h"
#include "components/keyboard.h"
#include "storage/nvs/storage_nvs.h"
#include <stdbool.h>
#include "lvgl.h"

#include <stdio.h>
#include <string.h>

static lv_obj_t *root;
static lv_obj_t *btn_connect;
static lv_obj_t *label_btn_connect;

static lv_obj_t *ta_ssid;
static lv_obj_t *ta_password;

static lv_obj_t *label_status;
static lv_obj_t *label_ip;
static lv_obj_t *wifi_container;

static char ssid_buffer[33] = {0};
static char pass_buffer[65] = {0};
static bool wifi_scan_loaded = false;
static bool wifi_error_visible = false;


static void wifi_msgbox_event_cb(lv_event_t *e)
{
    wifi_error_visible = false;
}

// =========================
// SSID CLICK
// =========================
static void ta_ssid_event_cb(lv_event_t *e)
{
    keyboard_open(ta_ssid);
}

// =========================
// PASS CLICK
// =========================
static void ta_pass_event_cb(lv_event_t *e)
{
    keyboard_open(ta_password);
}

// =========================
// WIFI NETWORK CLICK
// =========================
static void wifi_network_event_cb(lv_event_t *e)
{
    const char *ssid =
        (const char *)lv_event_get_user_data(e);

    if (!ssid)
        return;

    strncpy(ssid_buffer,
            ssid,
            sizeof(ssid_buffer));

    lv_textarea_set_text(
        ta_ssid,
        ssid_buffer);

    printf("SSID seleccionado: %s\n",
           ssid);
}

// =========================
// CONNECT BUTTON
// =========================
static void btn_connect_event_cb(lv_event_t *e)
{
    if (wifi_is_connected())
    {
        wifi_disconnect();
    }
    else
    {
        strncpy(
            ssid_buffer,
            lv_textarea_get_text(ta_ssid),
            sizeof(ssid_buffer));

        strncpy(
            pass_buffer,
            lv_textarea_get_text(ta_password),
            sizeof(pass_buffer));

        wifi_connect(
            ssid_buffer,
            pass_buffer);
    }
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

    for (int i = 0; i < count; i++)
    {
        const char *ssid =
            wifi_get_scan_ssid(i);

        if (!ssid || strlen(ssid) == 0)
            continue;

        printf("SSID[%d]: %s\n",
               i,
               ssid);

        lv_obj_t *btn =
            lv_btn_create(
                wifi_container);

        lv_obj_set_width(
            btn,
            LV_PCT(100));

        lv_obj_set_style_bg_color(
            btn,
            lv_palette_lighten(
                LV_PALETTE_BLUE,
                2),
            0);

        lv_obj_t *label =
            lv_label_create(btn);

        char txt[64];

        snprintf(txt,
                 sizeof(txt),
                 "%s %s",
                 LV_SYMBOL_WIFI,
                 ssid);

        lv_label_set_text(
            label,
            txt);

        lv_obj_center(label);

        lv_obj_add_event_cb(
            btn,
            wifi_network_event_cb,
            LV_EVENT_CLICKED,
            (void *)ssid);
    }
}

lv_obj_t *screen_config_wifi_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);

    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(root, LV_LAYOUT_FLEX);

    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    lv_obj_set_style_pad_all(root, 20, 0);

    lv_obj_set_style_pad_gap(root, 15, 0);

    // =========================
    // TITLE
    // =========================
    lv_obj_t *title = lv_label_create(root);

    lv_label_set_text(title, "WiFi");

    // =========================
    // WIFI LIST TITLE
    // =========================
    lv_obj_t *label_list =
        lv_label_create(root);

    lv_label_set_text(
        label_list,
        "Redes disponibles");

    // =========================
    // WIFI LIST
    // =========================
    wifi_container = lv_obj_create(root);

    lv_obj_set_width(
        wifi_container,
        LV_PCT(100));

    lv_obj_set_layout(
        wifi_container,
        LV_LAYOUT_FLEX);

    lv_obj_set_flex_flow(
        wifi_container,
        LV_FLEX_FLOW_COLUMN);

    lv_obj_set_style_pad_all(
        wifi_container,
        4,
        0);

    lv_obj_set_style_pad_gap(
        wifi_container,
        4,
        0);

    // =========================
    // SSID
    // =========================
    lv_obj_t *label_ssid = lv_label_create(root);

    lv_label_set_text(label_ssid, "SSID");

    ta_ssid = lv_textarea_create(root);

    lv_obj_set_width(ta_ssid, 280);

    lv_textarea_set_one_line(ta_ssid, true);

    storage_nvs_load_wifi(
        ssid_buffer,
        sizeof(ssid_buffer),
        pass_buffer,
        sizeof(pass_buffer));

    lv_textarea_set_text(
        ta_ssid,
        ssid_buffer);

    lv_obj_add_event_cb(
        ta_ssid,
        ta_ssid_event_cb,
        LV_EVENT_CLICKED,
        NULL);

    // =========================
    // PASSWORD
    // =========================
    lv_obj_t *label_pass = lv_label_create(root);

    lv_label_set_text(label_pass, "Password");

    ta_password = lv_textarea_create(root);

    lv_obj_set_width(ta_password, 280);

    lv_textarea_set_password_mode(
        ta_password,
        true);

    lv_textarea_set_one_line(
        ta_password,
        true);

    lv_textarea_set_text(
        ta_password,
        pass_buffer);

    lv_obj_add_event_cb(
        ta_password,
        ta_pass_event_cb,
        LV_EVENT_CLICKED,
        NULL);

    // =========================
    // CONNECT BUTTON
    // =========================
    btn_connect =
        lv_btn_create(root);

    lv_obj_set_width(btn_connect, 220);

    lv_obj_add_event_cb(
        btn_connect,
        btn_connect_event_cb,
        LV_EVENT_CLICKED,
        NULL);

    label_btn_connect =
        lv_label_create(btn_connect);

    lv_label_set_text(
        label_btn_connect,
        "CONECTAR");

    lv_obj_center(label_btn_connect);

    // =========================
    // STATUS
    // =========================
    label_status =
        lv_label_create(root);

    lv_label_set_text(
        label_status,
        "Estado: Desconectado");

    label_ip =
        lv_label_create(root);

    lv_label_set_text(
        label_ip,
        "IP: 0.0.0.0");

    return root;
}

// =========================
// UPDATE
// =========================
void screen_config_wifi_update(void)
{
    // =========================
    // LOAD WIFI LIST ONCE
    // =========================
    if (!wifi_scan_loaded)
    {
        wifi_scan_loaded = true;

        load_wifi_list();
    }

    // =========================
    // STATUS
    // =========================

    if (wifi_is_connected())
    {
        lv_label_set_text(
            label_btn_connect,
            "DESCONECTAR");
    }
    else
    {
        lv_label_set_text(
            label_btn_connect,
            "CONECTAR");
    }
    if (wifi_is_connected())
    {
        lv_label_set_text(
            label_status,
            "Estado: Conectado");
    }
    else
    {
        lv_label_set_text(
            label_status,
            "Estado: Desconectado");
    }

    char ip_buf[64];

    snprintf(ip_buf,
             sizeof(ip_buf),
             "IP: %s",
             wifi_get_ip_string());

    lv_label_set_text(
        label_ip,
        ip_buf);

const char *err =
    wifi_get_last_error();

if (!wifi_error_visible &&
    err &&
    strlen(err) > 0)
{
    wifi_error_visible = true;

    lv_obj_t *mbox =
        lv_msgbox_create(NULL);

    lv_msgbox_add_title(
        mbox,
        "WiFi");

    lv_msgbox_add_text(
        mbox,
        err);

    lv_msgbox_add_close_button(
        mbox);

    lv_obj_center(mbox);

    // 🔥 quitar sombras pesadas
    lv_obj_set_style_shadow_width(
        mbox,
        0,
        0);

    lv_obj_set_style_radius(
        mbox,
        8,
        0);

    wifi_clear_last_error();

    lv_obj_add_event_cb(
        mbox,
        wifi_msgbox_event_cb,
        LV_EVENT_DELETE,
        NULL);
}
}