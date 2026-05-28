#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

esp_err_t storage_nvs_init(void);

esp_err_t storage_nvs_save_profiles(const char *json);
esp_err_t storage_nvs_load_profiles(char *buffer, size_t max_len);

bool storage_nvs_profiles_exist(void);

esp_err_t storage_nvs_load_defaults(void);

// =========================
// WIFI CONFIG
// =========================
esp_err_t storage_nvs_save_wifi(const char *ssid, const char *password);
esp_err_t storage_nvs_load_wifi(char *ssid, size_t ssid_max_len,
                                char *password, size_t pass_max_len);
bool storage_nvs_wifi_exists(void);

// =========================
// UI THEME
// =========================
void storage_nvs_save_theme(int theme_id);
int  storage_nvs_load_theme(void);

// =========================
// ALARM CONFIG
// =========================
void storage_nvs_save_alarm(bool enabled, int threshold);
void storage_nvs_load_alarm(bool *enabled, int *threshold);

// =========================
// PRE-CUT ALARM CONFIG
// =========================
void storage_nvs_save_pre_cut_alarm(bool enabled, int seconds);
void storage_nvs_load_pre_cut_alarm(bool *enabled, int *seconds);

// =========================
// MARKING RELAY CONFIG
// =========================
void storage_nvs_save_marking_relay_enabled(bool enabled);
void storage_nvs_load_marking_relay_enabled(bool *enabled);

// =========================
// LENGTH CALIBRATION
// =========================
void storage_nvs_save_cal_factor(float factor);
void storage_nvs_load_cal_factor(float *factor);
