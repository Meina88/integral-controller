#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "storage/nvs/storage_nvs.h"
#include <string.h>

static const char *TAG = "wifi";

static char wifi_ssid[33] = {0};
static char wifi_pass[65] = {0};
static bool wifi_connected = false;
static bool reconnect_enabled = true;
static char ip_string[32] = "0.0.0.0";
#define MAX_WIFI_SCAN_RESULTS 20

static wifi_ap_record_t scan_results[MAX_WIFI_SCAN_RESULTS];

static uint16_t scan_count = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START)
    {
        if (strlen(wifi_ssid) > 0)
        {
            esp_wifi_connect();
        }
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_connected = false;

        strcpy(ip_string, "0.0.0.0");

        if (reconnect_enabled)
        {
            ESP_LOGI(TAG, "Reconnecting...");

            esp_wifi_connect();
        }
    }
    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        wifi_connected = true;

        snprintf(ip_string,
                 sizeof(ip_string),
                 IPSTR,
                 IP2STR(&event->ip_info.ip));

        ESP_LOGI(TAG, "IP: %s", ip_string);
    }
}

void wifi_init_sta(void)
{
    esp_netif_init();

    esp_event_loop_create_default();

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL);

    esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        NULL);

    esp_wifi_set_mode(WIFI_MODE_STA);

    bool has_credentials =
        storage_nvs_wifi_exists();

    if (has_credentials)
    {
        esp_err_t err =
            storage_nvs_load_wifi(
                wifi_ssid,
                sizeof(wifi_ssid),
                wifi_pass,
                sizeof(wifi_pass));

        if (err == ESP_OK)
        {
            wifi_config_t wifi_config = {0};

            strncpy(
                (char *)wifi_config.sta.ssid,
                wifi_ssid,
                sizeof(wifi_config.sta.ssid));

            strncpy(
                (char *)wifi_config.sta.password,
                wifi_pass,
                sizeof(wifi_config.sta.password));

            esp_wifi_set_config(
                WIFI_IF_STA,
                &wifi_config);

            ESP_LOGI(TAG,
                     "SSID loaded: %s",
                     wifi_ssid);
        }
    }
    else
    {
        ESP_LOGW(TAG,
                 "No WiFi credentials saved");
    }

    esp_wifi_start();

    ESP_LOGI(TAG,
             "WiFi init...");
}

// =========================
// CONNECT
// =========================
void wifi_connect(const char *ssid,
                  const char *password)
{
    if (!ssid || strlen(ssid) == 0)
        return;

    strncpy(wifi_ssid,
            ssid,
            sizeof(wifi_ssid));

    strncpy(wifi_pass,
            password ? password : "",
            sizeof(wifi_pass));

    storage_nvs_save_wifi(
        wifi_ssid,
        wifi_pass);

    wifi_config_t wifi_config = {0};

    strncpy((char *)wifi_config.sta.ssid,
            wifi_ssid,
            sizeof(wifi_config.sta.ssid));

    strncpy((char *)wifi_config.sta.password,
            wifi_pass,
            sizeof(wifi_config.sta.password));

    reconnect_enabled = true;

    esp_wifi_disconnect();

    esp_wifi_set_config(
        WIFI_IF_STA,
        &wifi_config);

    esp_wifi_connect();

    ESP_LOGI(TAG,
             "Connecting to: %s",
             wifi_ssid);
}

// =========================
// DISCONNECT
// =========================
void wifi_disconnect(void)
{
    reconnect_enabled = false;

    esp_wifi_disconnect();

    wifi_connected = false;

    strcpy(ip_string, "0.0.0.0");
}

// =========================
// STATUS
// =========================
bool wifi_is_connected(void)
{
    return wifi_connected;
}

const char *wifi_get_ip_string(void)
{
    return ip_string;
}

const char *wifi_get_ssid(void)
{
    return wifi_ssid;
}

// =========================
// START SCAN
// =========================
void wifi_start_scan(void)
{
    wifi_scan_config_t scan_config = {
        .show_hidden = false};

    esp_wifi_scan_start(&scan_config, true);

    scan_count = MAX_WIFI_SCAN_RESULTS;

    esp_wifi_scan_get_ap_records(
        &scan_count,
        scan_results);

    esp_wifi_clear_ap_list();

    ESP_LOGI(TAG,
             "WiFi scan complete: %d networks",
             scan_count);
}

// =========================
// GET COUNT
// =========================
int wifi_get_scan_count(void)
{
    return scan_count;
}

// =========================
// GET SSID
// =========================
const char *wifi_get_scan_ssid(int index)
{
    if (index < 0 || index >= scan_count)
        return "";

    return (const char *)scan_results[index].ssid;
}