#include "ota_manager.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "esp_crt_bundle.h"
#include "esp_system.h"

#include "cJSON.h"

static const char *TAG = "ota_manager";

#define OTA_HTTP_BUF_RX_MANIFEST 8192
#define OTA_HTTP_BUF_RX_FIRMWARE 8192
#define OTA_HTTP_BUF_TX          2048
#define OTA_HTTP_TIMEOUT_MANIFEST_MS 20000
#define OTA_HTTP_TIMEOUT_FIRMWARE_MS 30000

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} http_buffer_t;

static ota_status_t s_status;
static SemaphoreHandle_t s_status_mutex;

static void status_lock(void)
{
    xSemaphoreTake(s_status_mutex, portMAX_DELAY);
}

static void status_unlock(void)
{
    xSemaphoreGive(s_status_mutex);
}

static void set_error_locked(const char *msg)
{
    s_status.state = OTA_STATE_ERROR;
    s_status.busy = false;
    s_status.progress = 0;
    snprintf(s_status.last_error, sizeof(s_status.last_error), "%s", msg ? msg : "unknown");
}

static int parse_version_triplet(const char *v, int out[3])
{
    if (!v || !out) return -1;
    int major = 0, minor = 0, patch = 0;
    if (sscanf(v, "%d.%d.%d", &major, &minor, &patch) < 1) return -1;
    out[0] = major;
    out[1] = minor;
    out[2] = patch;
    return 0;
}

static int compare_versions(const char *a, const char *b)
{
    int va[3] = {0}, vb[3] = {0};
    if (parse_version_triplet(a, va) != 0 || parse_version_triplet(b, vb) != 0) {
        return strcmp(a ? a : "", b ? b : "");
    }

    for (int i = 0; i < 3; i++) {
        if (va[i] > vb[i]) return 1;
        if (va[i] < vb[i]) return -1;
    }
    return 0;
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_buffer_t *buf = (http_buffer_t *)evt->user_data;
    if (!buf) return ESP_OK;

    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data && evt->data_len > 0) {
        size_t needed = buf->len + (size_t)evt->data_len + 1;
        if (needed > buf->cap) {
            size_t new_cap = buf->cap == 0 ? 512 : buf->cap * 2;
            while (new_cap < needed) new_cap *= 2;
            char *new_data = realloc(buf->data, new_cap);
            if (!new_data) return ESP_ERR_NO_MEM;
            buf->data = new_data;
            buf->cap = new_cap;
        }
        memcpy(buf->data + buf->len, evt->data, (size_t)evt->data_len);
        buf->len += (size_t)evt->data_len;
        buf->data[buf->len] = '\0';
    }
    return ESP_OK;
}

static esp_err_t download_manifest(char **out_json)
{
    if (!out_json) return ESP_ERR_INVALID_ARG;
    *out_json = NULL;

    http_buffer_t buffer = {0};
    esp_http_client_config_t cfg = {
        .url = OTA_MANIFEST_URL,
        .method = HTTP_METHOD_GET,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = http_event_handler,
        .user_data = &buffer,
        .timeout_ms = OTA_HTTP_TIMEOUT_MANIFEST_MS,
        .buffer_size = OTA_HTTP_BUF_RX_MANIFEST,
        .buffer_size_tx = OTA_HTTP_BUF_TX,
        .user_agent = "integral-controller-ota/1.0",
        .keep_alive_enable = false,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return ESP_FAIL;

    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "manifest perform failed: %s", esp_err_to_name(err));
        free(buffer.data);
        return err;
    }
    if (status_code != 200 || !buffer.data) {
        ESP_LOGE(TAG, "manifest invalid response: status=%d len=%u",
                 status_code, (unsigned)buffer.len);
        free(buffer.data);
        return ESP_FAIL;
    }

    *out_json = buffer.data;
    return ESP_OK;
}

static esp_err_t parse_manifest_and_update_status(const char *manifest_json)
{
    if (!manifest_json) return ESP_ERR_INVALID_ARG;

    cJSON *root = cJSON_Parse(manifest_json);
    if (!root) return ESP_FAIL;

    const cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
    const cJSON *firmware = cJSON_GetObjectItemCaseSensitive(root, "firmware");
    if (!cJSON_IsString(version) || !cJSON_IsString(firmware) ||
        !version->valuestring || !firmware->valuestring) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    status_lock();
    snprintf(s_status.available_version, sizeof(s_status.available_version), "%s", version->valuestring);
    snprintf(s_status.firmware_url, sizeof(s_status.firmware_url), "%s", firmware->valuestring);
    s_status.update_available = compare_versions(s_status.available_version, s_status.current_version) > 0;
    status_unlock();

    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t refresh_manifest_info(void)
{
    char *manifest = NULL;
    esp_err_t err = download_manifest(&manifest);
    if (err != ESP_OK) return err;

    err = parse_manifest_and_update_status(manifest);
    free(manifest);
    return err;
}

static void ota_check_task(void *arg)
{
    (void)arg;

    status_lock();
    s_status.state = OTA_STATE_CHECKING;
    s_status.busy = true;
    s_status.progress = 0;
    s_status.last_error[0] = '\0';
    status_unlock();

    esp_err_t err = refresh_manifest_info();

    status_lock();
    if (err == ESP_OK) {
        s_status.state = OTA_STATE_IDLE;
        s_status.busy = false;
    } else {
        char msg[96];
        snprintf(msg, sizeof(msg), "manifest fail: %s", esp_err_to_name(err));
        set_error_locked(msg);
    }
    status_unlock();

    vTaskDelete(NULL);
}

static void ota_update_task(void *arg)
{
    (void)arg;

    esp_err_t err = refresh_manifest_info();
    if (err != ESP_OK) {
        status_lock();
        char msg[96];
        snprintf(msg, sizeof(msg), "manifest fail: %s", esp_err_to_name(err));
        set_error_locked(msg);
        status_unlock();
        vTaskDelete(NULL);
        return;
    }

    status_lock();
    if (!s_status.update_available) {
        s_status.state = OTA_STATE_IDLE;
        s_status.busy = false;
        s_status.progress = 100;
        status_unlock();
        vTaskDelete(NULL);
        return;
    }

    s_status.state = OTA_STATE_DOWNLOADING;
    s_status.busy = true;
    s_status.progress = 0;
    s_status.last_error[0] = '\0';
    char firmware_url[sizeof(s_status.firmware_url)];
    snprintf(firmware_url, sizeof(firmware_url), "%s", s_status.firmware_url);
    status_unlock();

    esp_http_client_config_t http_cfg = {
        .url = firmware_url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = OTA_HTTP_TIMEOUT_FIRMWARE_MS,
        .buffer_size = OTA_HTTP_BUF_RX_FIRMWARE,
        .buffer_size_tx = OTA_HTTP_BUF_TX,
        .user_agent = "integral-controller-ota/1.0",
        .keep_alive_enable = false,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    esp_https_ota_handle_t ota_handle = NULL;
    err = esp_https_ota_begin(&ota_cfg, &ota_handle);
    if (err != ESP_OK) {
        status_lock();
        set_error_locked("esp_https_ota_begin failed");
        status_unlock();
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        err = esp_https_ota_perform(ota_handle);
        if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            int image_len = esp_https_ota_get_image_len_read(ota_handle);
            int image_size = esp_https_ota_get_image_size(ota_handle);
            if (image_size > 0) {
                int pct = (image_len * 100) / image_size;
                if (pct > 100) pct = 100;
                status_lock();
                s_status.progress = pct;
                status_unlock();
            }
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        break;
    }

    if (err != ESP_OK) {
        esp_https_ota_abort(ota_handle);
        status_lock();
        set_error_locked("esp_https_ota_perform failed");
        status_unlock();
        vTaskDelete(NULL);
        return;
    }

    err = esp_https_ota_finish(ota_handle);
    if (err != ESP_OK) {
        status_lock();
        set_error_locked("esp_https_ota_finish failed");
        status_unlock();
        vTaskDelete(NULL);
        return;
    }

    status_lock();
    s_status.state = OTA_STATE_SUCCESS;
    s_status.busy = false;
    s_status.progress = 100;
    s_status.last_error[0] = '\0';
    status_unlock();

    ESP_LOGI(TAG, "OTA success, rebooting...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

void ota_manager_init(void)
{
    if (!s_status_mutex) {
        s_status_mutex = xSemaphoreCreateMutex();
    }

    status_lock();
    memset(&s_status, 0, sizeof(s_status));
    s_status.state = OTA_STATE_IDLE;
    s_status.progress = 0;
    s_status.busy = false;
    snprintf(s_status.current_version, sizeof(s_status.current_version), "%s", FW_VERSION);
    snprintf(s_status.available_version, sizeof(s_status.available_version), "%s", s_status.current_version);
    s_status.update_available = false;
    status_unlock();
}

esp_err_t ota_check_for_update(void)
{
    status_lock();
    bool busy = s_status.busy;
    status_unlock();
    if (busy) return ESP_ERR_INVALID_STATE;

    BaseType_t ok = xTaskCreate(ota_check_task, "ota_check_task", 6144, NULL, 4, NULL);
    return ok == pdPASS ? ESP_OK : ESP_FAIL;
}

esp_err_t ota_start_update(void)
{
    status_lock();
    bool busy = s_status.busy;
    status_unlock();
    if (busy) return ESP_ERR_INVALID_STATE;

    status_lock();
    s_status.busy = true;
    s_status.state = OTA_STATE_DOWNLOADING;
    s_status.progress = 0;
    s_status.last_error[0] = '\0';
    status_unlock();

    BaseType_t ok = xTaskCreate(ota_update_task, "ota_update_task", 10240, NULL, 5, NULL);
    if (ok != pdPASS) {
        status_lock();
        set_error_locked("cannot create ota task");
        status_unlock();
        return ESP_FAIL;
    }
    return ESP_OK;
}

void ota_get_status(ota_status_t *out_status)
{
    if (!out_status) return;
    status_lock();
    *out_status = s_status;
    status_unlock();
}

esp_err_t ota_mark_running_image_validity(bool diagnostics_ok)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    esp_err_t err = esp_ota_get_state_partition(running, &state);
    if (err != ESP_OK) return err;

    if (state == ESP_OTA_IMG_PENDING_VERIFY) {
        if (diagnostics_ok) {
            ESP_LOGI(TAG, "Diagnostics OK: marking app valid");
            return esp_ota_mark_app_valid_cancel_rollback();
        }

        ESP_LOGE(TAG, "Diagnostics failed: rolling back");
        return esp_ota_mark_app_invalid_rollback_and_reboot();
    }

    return ESP_OK;
}
