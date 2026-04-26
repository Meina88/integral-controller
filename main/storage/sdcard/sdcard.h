#ifndef SDCARD_H
#define SDCARD_H

#include "esp_err.h"
#include <stdbool.h> 

// =========================
// API
// =========================
esp_err_t sdcard_init(void);
void sdcard_test(void);
bool sdcard_is_ready(void);
void sdcard_create_dirs(void);

#endif