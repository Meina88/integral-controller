#include "storage/sdcard/sdcard.h"
#include "esp_err.h"
#include <string.h>

esp_err_t sdcard_init(void)      { return ESP_OK; }
void      sdcard_test(void)      {}
bool      sdcard_is_ready(void)  { return false; }
void      sdcard_create_dirs(void) {}
