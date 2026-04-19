#ifndef SDCARD_H
#define SDCARD_H

#include "esp_err.h"

// =========================
// API
// =========================
esp_err_t sdcard_init(void);
void sdcard_test(void);

#endif