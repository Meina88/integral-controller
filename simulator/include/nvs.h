#pragma once
/* NVS shim — storage_nvs.c is replaced by a mock; this header exists only
   so any file that includes nvs.h compiles without errors. */
#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

typedef uint32_t nvs_handle_t;
typedef enum { NVS_READONLY = 0, NVS_READWRITE = 1 } nvs_open_mode_t;

static inline esp_err_t nvs_open(const char *n, nvs_open_mode_t m, nvs_handle_t *h) { (void)n;(void)m;*h=0; return ESP_OK; }
static inline void      nvs_close(nvs_handle_t h) { (void)h; }
static inline esp_err_t nvs_commit(nvs_handle_t h) { (void)h; return ESP_OK; }
static inline esp_err_t nvs_set_i32(nvs_handle_t h, const char *k, int32_t v) { (void)h;(void)k;(void)v; return ESP_OK; }
static inline esp_err_t nvs_get_i32(nvs_handle_t h, const char *k, int32_t *v) { (void)h;(void)k;(void)v; return ESP_FAIL; }
static inline esp_err_t nvs_set_str(nvs_handle_t h, const char *k, const char *v) { (void)h;(void)k;(void)v; return ESP_OK; }
static inline esp_err_t nvs_get_str(nvs_handle_t h, const char *k, char *v, size_t *l) { (void)h;(void)k;(void)v;(void)l; return ESP_FAIL; }
