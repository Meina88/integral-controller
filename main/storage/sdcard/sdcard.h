#ifndef _SDCARD_H_
#define _SDCARD_H_

#include "esp_err.h"

// =========================
// API
// =========================
esp_err_t sdcard_init(void);
void sdcard_test(void);

#endif