#include "comms/ota/ota_manager.h"
#include <string.h>

void      ota_manager_init(void) {}
esp_err_t ota_check_for_update(void) { return ESP_FAIL; }
esp_err_t ota_start_update(void)     { return ESP_FAIL; }

void ota_get_status(ota_status_t *out)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->state = OTA_STATE_IDLE;
    strncpy(out->current_version, FW_VERSION, sizeof(out->current_version) - 1);
}

esp_err_t ota_mark_running_image_validity(bool ok) { (void)ok; return ESP_OK; }
