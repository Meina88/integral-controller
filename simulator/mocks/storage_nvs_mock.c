/*
 * Mock de storage_nvs — reemplaza storage/nvs/storage_nvs.c en el simulador.
 * Usa variables estáticas en memoria; los valores se pierden al cerrar.
 */
#include "storage/nvs/storage_nvs.h"
#include "esp_err.h"
#include <string.h>
#include <stdio.h>

/* ── Estado en memoria ───────────────────────────────────────────────────── */
static int   s_theme         = 0;
static int   s_alarm_en      = 0;
static int   s_alarm_thr     = 10;
static int   s_precut_en     = 0;
static int   s_precut_sec    = 3;
static int   s_relay_en      = 1;
static int   s_relay_dur_ds  = 5;
static int   s_cal_factor    = 10000; /* × 10000 → 1.0000 */
static int   s_spray_max     = 500;
static int   s_spray_rem     = 500;

static char  s_profiles_json[32768] = "";
static char  s_wifi_ssid[64]        = "";
static char  s_wifi_pass[64]        = "";

/* ── Init ────────────────────────────────────────────────────────────────── */
esp_err_t storage_nvs_init(void) { return ESP_OK; }

/* ── Profiles ────────────────────────────────────────────────────────────── */
esp_err_t storage_nvs_save_profiles(const char *json)
{
    strncpy(s_profiles_json, json, sizeof(s_profiles_json) - 1);
    return ESP_OK;
}

esp_err_t storage_nvs_load_profiles(char *buffer, size_t max_len)
{
    if (s_profiles_json[0] == '\0') return ESP_FAIL;
    strncpy(buffer, s_profiles_json, max_len - 1);
    return ESP_OK;
}

bool storage_nvs_profiles_exist(void) { return s_profiles_json[0] != '\0'; }

esp_err_t storage_nvs_load_defaults(void)
{
    const char *def =
        "{\"profiles\":["
        "{\"code\":\"SIM-001\",\"matrix\":\"5000\",\"screw\":20,"
        "\"vfd_speed\":400,\"extrusion_speed\":8.0,\"density\":0.20,\"cut_length\":80},"
        "{\"code\":\"SIM-002\",\"matrix\":\"5001\",\"screw\":22,"
        "\"vfd_speed\":450,\"extrusion_speed\":9.0,\"density\":0.22,\"cut_length\":90}"
        "]}";
    return storage_nvs_save_profiles(def);
}

/* ── WiFi ────────────────────────────────────────────────────────────────── */
esp_err_t storage_nvs_save_wifi(const char *ssid, const char *password)
{
    strncpy(s_wifi_ssid, ssid     ? ssid     : "", sizeof(s_wifi_ssid) - 1);
    strncpy(s_wifi_pass, password ? password : "", sizeof(s_wifi_pass) - 1);
    return ESP_OK;
}

esp_err_t storage_nvs_load_wifi(char *ssid, size_t ssid_max,
                                char *password, size_t pass_max)
{
    if (ssid)     strncpy(ssid,     s_wifi_ssid, ssid_max - 1);
    if (password) strncpy(password, s_wifi_pass, pass_max - 1);
    return (s_wifi_ssid[0] != '\0') ? ESP_OK : ESP_FAIL;
}

bool storage_nvs_wifi_exists(void) { return s_wifi_ssid[0] != '\0'; }

/* ── Theme ───────────────────────────────────────────────────────────────── */
void storage_nvs_save_theme(int id) { s_theme = id; }
int  storage_nvs_load_theme(void)   { return s_theme; }

/* ── Speed alarm ─────────────────────────────────────────────────────────── */
void storage_nvs_save_alarm(bool enabled, int threshold)
    { s_alarm_en = enabled; s_alarm_thr = threshold; }

void storage_nvs_load_alarm(bool *enabled, int *threshold)
{
    if (enabled)   *enabled   = (bool)s_alarm_en;
    if (threshold) *threshold = s_alarm_thr;
}

/* ── Pre-cut alarm ───────────────────────────────────────────────────────── */
void storage_nvs_save_pre_cut_alarm(bool enabled, int seconds)
    { s_precut_en = enabled; s_precut_sec = seconds; }

void storage_nvs_load_pre_cut_alarm(bool *enabled, int *seconds)
{
    if (enabled) *enabled = (bool)s_precut_en;
    if (seconds) *seconds = s_precut_sec;
}

/* ── Marking relay ───────────────────────────────────────────────────────── */
void storage_nvs_save_marking_relay_enabled(bool enabled) { s_relay_en = enabled; }
void storage_nvs_load_marking_relay_enabled(bool *enabled)
    { if (enabled) *enabled = (bool)s_relay_en; }

void storage_nvs_save_marking_relay_duration_ds(int ds) { s_relay_dur_ds = ds; }
void storage_nvs_load_marking_relay_duration_ds(int *ds)
    { if (ds) *ds = s_relay_dur_ds; }

/* ── Calibration ─────────────────────────────────────────────────────────── */
void storage_nvs_save_cal_factor(float factor)
    { s_cal_factor = (int)(factor * 10000.0f); }

void storage_nvs_load_cal_factor(float *factor)
    { if (factor) *factor = (float)s_cal_factor / 10000.0f; }

/* ── Spray shots ─────────────────────────────────────────────────────────── */
void storage_nvs_save_spray_shots_max(int max) { s_spray_max = max; }
void storage_nvs_load_spray_shots_max(int *max) { if (max) *max = s_spray_max; }

void storage_nvs_save_spray_shots_remaining(int rem) { s_spray_rem = rem; }
void storage_nvs_load_spray_shots_remaining(int *rem) { if (rem) *rem = s_spray_rem; }
