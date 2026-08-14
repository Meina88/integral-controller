#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FW_VERSION "1.1.1"
#define OTA_MANIFEST_URL "https://github.com/Meina88/integral-controller/releases/latest/download/manifest.json"

typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_CHECKING,
    OTA_STATE_DOWNLOADING,
    OTA_STATE_SUCCESS,
    OTA_STATE_ERROR
} ota_state_t;

typedef struct {
    ota_state_t state;
    char current_version[32];
    char available_version[32];
    char firmware_url[256];
    int progress;
    bool update_available;
    bool busy;
    char last_error[128];
} ota_status_t;

void ota_manager_init(void);
esp_err_t ota_check_for_update(void);
esp_err_t ota_start_update(void);
void ota_get_status(ota_status_t *out_status);
esp_err_t ota_mark_running_image_validity(bool diagnostics_ok);

#ifdef __cplusplus
}
#endif

