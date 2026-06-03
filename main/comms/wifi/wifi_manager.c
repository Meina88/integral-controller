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
static bool connection_error_reported = false;
static char ip_string[32] = "0.0.0.0";
static char wifi_last_error[64] = "";
static uint8_t connect_attempt = 0;
static wifi_state_t    s_wifi_state = WIFI_STATE_IDLE;
static volatile bool   s_scanning   = false;
static volatile bool   s_scan_done  = false;
#define MAX_WIFI_SCAN_RESULTS 20
#define WIFI_ERROR_MIN_ATTEMPTS 3

static wifi_ap_record_t scan_results[MAX_WIFI_SCAN_RESULTS];

static uint16_t scan_count = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START)
    {
        connect_attempt = 0;
        if (strlen(wifi_ssid) > 0)
        {
            s_wifi_state = WIFI_STATE_CONNECTING;
            esp_wifi_connect();
        }
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_event_sta_disconnected_t *disconn =
            (wifi_event_sta_disconnected_t *)event_data;

        wifi_connected = false;
        strcpy(ip_string, "0.0.0.0");
        connect_attempt++;

        if (reconnect_enabled)
        {
            // Durante un escaneo el stack WiFi desconecta brevemente; ignorar el
            // cambio de estado y no reconectar para no abortar el escaneo en curso.
            if (!s_scanning)
            {
                // Reportar error solo tras WIFI_ERROR_MIN_ATTEMPTS fallos consecutivos
                // para no mostrar falsos positivos durante el proceso normal de asociación.
                if (!connection_error_reported &&
                    connect_attempt >= WIFI_ERROR_MIN_ATTEMPTS)
                {
                    connection_error_reported = true;

                    switch (disconn->reason)
                    {
                    case WIFI_REASON_AUTH_FAIL:
                    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                    case WIFI_REASON_HANDSHAKE_TIMEOUT:
                        s_wifi_state = WIFI_STATE_AUTH_FAIL;
                        snprintf(wifi_last_error, sizeof(wifi_last_error),
                                 "Contrasena incorrecta");
                        break;

                    case WIFI_REASON_NO_AP_FOUND:
                        s_wifi_state = WIFI_STATE_NO_AP;
                        snprintf(wifi_last_error, sizeof(wifi_last_error),
                                 "Red no encontrada");
                        break;

                    default:
                        s_wifi_state = WIFI_STATE_ERROR;
                        snprintf(wifi_last_error, sizeof(wifi_last_error),
                                 "Error WiFi (%d)", disconn->reason);
                        break;
                    }

                    ESP_LOGW(TAG, "WiFi error after %d attempts: %s",
                             connect_attempt, wifi_last_error);
                }
                else if (!connection_error_reported)
                {
                    s_wifi_state = WIFI_STATE_CONNECTING;
                }

                esp_wifi_connect();
            }
        }
        else
        {
            // Desconexión manual: no reconectar, marcar como desconectado.
            s_wifi_state = WIFI_STATE_DISCONNECTED;
        }
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_SCAN_DONE)
    {
        s_scanning  = false;
        scan_count  = MAX_WIFI_SCAN_RESULTS;
        esp_wifi_scan_get_ap_records(&scan_count, scan_results);
        esp_wifi_clear_ap_list();
        s_scan_done = true;
        ESP_LOGI(TAG, "WiFi scan complete: %d networks", scan_count);
    }
    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        wifi_connected = true;
        connect_attempt = 0;
        connection_error_reported = false;
        s_wifi_state = WIFI_STATE_CONNECTED;
        wifi_clear_last_error();

        snprintf(ip_string,
                 sizeof(ip_string),
                 IPSTR,
                 IP2STR(&event->ip_info.ip));

        ESP_LOGI(TAG, "Connected — IP: %s", ip_string);
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

    bool has_credentials = storage_nvs_wifi_exists();

    if (has_credentials)
    {
        s_wifi_state = WIFI_STATE_CONNECTING;
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
    connect_attempt = 0;
    connection_error_reported = false;
    s_wifi_state = WIFI_STATE_CONNECTING;
    wifi_clear_last_error();

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
    wifi_connected = false;
    s_wifi_state = WIFI_STATE_DISCONNECTED;
    strcpy(ip_string, "0.0.0.0");
    wifi_clear_last_error();

    esp_wifi_disconnect();
}

// =========================
// STATUS
// =========================
bool wifi_is_connected(void)
{
    return wifi_connected;
}

wifi_state_t wifi_get_state(void)
{
    return s_wifi_state;
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
// START SCAN (non-blocking)
// Results ready when wifi_is_scan_done() returns true.
// WIFI_EVENT_SCAN_DONE fires in the event task and fills scan_results.
// =========================
void wifi_start_scan(void)
{
    wifi_scan_config_t scan_config = {
        .show_hidden = false};

    scan_count  = 0;
    s_scan_done = false;
    s_scanning  = true;

    esp_err_t err =
        esp_wifi_scan_start(&scan_config, false);

    if (err != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "WiFi scan skipped: %s",
                 esp_err_to_name(err));
        s_scanning  = false;
        s_scan_done = true;
        return;
    }
}

// =========================
// SCAN DONE
// =========================
bool wifi_is_scan_done(void)
{
    return s_scan_done;
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

const char *wifi_get_last_error(void)
{
    return wifi_last_error;
}

void wifi_clear_last_error(void)
{
    wifi_last_error[0] = 0;
}