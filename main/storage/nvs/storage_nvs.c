#include "storage_nvs.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdio.h>

#define NAMESPACE "storage"
#define KEY_WIFI_SSID       "wifi_ssid"
#define KEY_WIFI_PASS       "wifi_pass"
#define KEY_PROFILES        "profiles_json"
#define KEY_THEME           "ui_theme"
#define KEY_ALARM_ENABLED   "alarm_en"
#define KEY_ALARM_THRESHOLD "alarm_thr"
#define KEY_PRECUT_ENABLED  "precut_en"
#define KEY_PRECUT_SECONDS  "precut_sec"

// =========================
// INIT NVS
// =========================
esp_err_t storage_nvs_init(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    return err;
}

// =========================
// SAVE JSON
// =========================
esp_err_t storage_nvs_save_profiles(const char *json)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_str(handle, KEY_PROFILES, json);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    return err;
}

// =========================
// LOAD JSON
// =========================
esp_err_t storage_nvs_load_profiles(char *buffer, size_t max_len)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    size_t required_size = 0;

    // primero obtener tamaño
    err = nvs_get_str(handle, KEY_PROFILES, NULL, &required_size);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }

    if (required_size > max_len) {
        nvs_close(handle);
        return ESP_ERR_NO_MEM;
    }

    // luego leer
    err = nvs_get_str(handle, KEY_PROFILES, buffer, &required_size);

    nvs_close(handle);
    return err;
}

// =========================
// CHECK EXIST
// =========================
bool storage_nvs_profiles_exist(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return false;

    size_t required_size = 0;
    err = nvs_get_str(handle, KEY_PROFILES, NULL, &required_size);

    nvs_close(handle);

    return (err == ESP_OK);
}

// =========================
// LOAD DEFAULT
// =========================
esp_err_t storage_nvs_load_defaults(void)
{
const char *default_json =
"{"
"\"profiles\":["
"{\"code\":\"1000.5000\",\"matrix\":\"5000\",\"screw\":20,\"vfd_speed\":400,\"extrusion_speed\":8.0,\"density\":0.20,\"cut_length\":80},"
"{\"code\":\"1001.5001\",\"matrix\":\"5001\",\"screw\":21,\"vfd_speed\":410,\"extrusion_speed\":8.2,\"density\":0.21,\"cut_length\":82},"
"{\"code\":\"1002.5002\",\"matrix\":\"5002\",\"screw\":22,\"vfd_speed\":420,\"extrusion_speed\":8.4,\"density\":0.22,\"cut_length\":84},"
"{\"code\":\"1003.5003\",\"matrix\":\"5003\",\"screw\":23,\"vfd_speed\":430,\"extrusion_speed\":8.6,\"density\":0.23,\"cut_length\":86},"
"{\"code\":\"1004.5004\",\"matrix\":\"5004\",\"screw\":24,\"vfd_speed\":440,\"extrusion_speed\":8.8,\"density\":0.24,\"cut_length\":88},"
"{\"code\":\"1005.5005\",\"matrix\":\"5005\",\"screw\":25,\"vfd_speed\":450,\"extrusion_speed\":9.0,\"density\":0.25,\"cut_length\":90},"
"{\"code\":\"1006.5006\",\"matrix\":\"5006\",\"screw\":26,\"vfd_speed\":460,\"extrusion_speed\":9.2,\"density\":0.26,\"cut_length\":92},"
"{\"code\":\"1007.5007\",\"matrix\":\"5007\",\"screw\":27,\"vfd_speed\":470,\"extrusion_speed\":9.4,\"density\":0.27,\"cut_length\":94},"
"{\"code\":\"1008.5008\",\"matrix\":\"5008\",\"screw\":28,\"vfd_speed\":480,\"extrusion_speed\":9.6,\"density\":0.28,\"cut_length\":96},"
"{\"code\":\"1009.5009\",\"matrix\":\"5009\",\"screw\":29,\"vfd_speed\":490,\"extrusion_speed\":9.8,\"density\":0.29,\"cut_length\":98},"

"{\"code\":\"1010.5010\",\"matrix\":\"5010\",\"screw\":20,\"vfd_speed\":500,\"extrusion_speed\":10.0,\"density\":0.30,\"cut_length\":100},"
"{\"code\":\"1011.5011\",\"matrix\":\"5011\",\"screw\":21,\"vfd_speed\":510,\"extrusion_speed\":10.2,\"density\":0.31,\"cut_length\":102},"
"{\"code\":\"1012.5012\",\"matrix\":\"5012\",\"screw\":22,\"vfd_speed\":520,\"extrusion_speed\":10.4,\"density\":0.32,\"cut_length\":104},"
"{\"code\":\"1013.5013\",\"matrix\":\"5013\",\"screw\":23,\"vfd_speed\":530,\"extrusion_speed\":10.6,\"density\":0.33,\"cut_length\":106},"
"{\"code\":\"1014.5014\",\"matrix\":\"5014\",\"screw\":24,\"vfd_speed\":540,\"extrusion_speed\":10.8,\"density\":0.34,\"cut_length\":108},"
"{\"code\":\"1015.5015\",\"matrix\":\"5015\",\"screw\":25,\"vfd_speed\":550,\"extrusion_speed\":11.0,\"density\":0.35,\"cut_length\":110},"
"{\"code\":\"1016.5016\",\"matrix\":\"5016\",\"screw\":26,\"vfd_speed\":560,\"extrusion_speed\":11.2,\"density\":0.36,\"cut_length\":112},"
"{\"code\":\"1017.5017\",\"matrix\":\"5017\",\"screw\":27,\"vfd_speed\":570,\"extrusion_speed\":11.4,\"density\":0.37,\"cut_length\":114},"
"{\"code\":\"1018.5018\",\"matrix\":\"5018\",\"screw\":28,\"vfd_speed\":580,\"extrusion_speed\":11.6,\"density\":0.38,\"cut_length\":116},"
"{\"code\":\"1019.5019\",\"matrix\":\"5019\",\"screw\":29,\"vfd_speed\":590,\"extrusion_speed\":11.8,\"density\":0.39,\"cut_length\":118},"

"{\"code\":\"1020.5020\",\"matrix\":\"5020\",\"screw\":20,\"vfd_speed\":600,\"extrusion_speed\":12.0,\"density\":0.40,\"cut_length\":120},"
"{\"code\":\"1021.5021\",\"matrix\":\"5021\",\"screw\":21,\"vfd_speed\":610,\"extrusion_speed\":12.2,\"density\":0.41,\"cut_length\":122},"
"{\"code\":\"1022.5022\",\"matrix\":\"5022\",\"screw\":22,\"vfd_speed\":620,\"extrusion_speed\":12.4,\"density\":0.42,\"cut_length\":124},"
"{\"code\":\"1023.5023\",\"matrix\":\"5023\",\"screw\":23,\"vfd_speed\":630,\"extrusion_speed\":12.6,\"density\":0.43,\"cut_length\":126},"
"{\"code\":\"1024.5024\",\"matrix\":\"5024\",\"screw\":24,\"vfd_speed\":640,\"extrusion_speed\":12.8,\"density\":0.44,\"cut_length\":128},"
"{\"code\":\"1025.5025\",\"matrix\":\"5025\",\"screw\":25,\"vfd_speed\":650,\"extrusion_speed\":13.0,\"density\":0.45,\"cut_length\":130},"
"{\"code\":\"1026.5026\",\"matrix\":\"5026\",\"screw\":26,\"vfd_speed\":660,\"extrusion_speed\":13.2,\"density\":0.46,\"cut_length\":132},"
"{\"code\":\"1027.5027\",\"matrix\":\"5027\",\"screw\":27,\"vfd_speed\":670,\"extrusion_speed\":13.4,\"density\":0.47,\"cut_length\":134},"
"{\"code\":\"1028.5028\",\"matrix\":\"5028\",\"screw\":28,\"vfd_speed\":680,\"extrusion_speed\":13.6,\"density\":0.48,\"cut_length\":136},"
"{\"code\":\"1029.5029\",\"matrix\":\"5029\",\"screw\":29,\"vfd_speed\":690,\"extrusion_speed\":13.8,\"density\":0.49,\"cut_length\":138},"

"{\"code\":\"1030.5030\",\"matrix\":\"5030\",\"screw\":20,\"vfd_speed\":700,\"extrusion_speed\":14.0,\"density\":0.50,\"cut_length\":140},"
"{\"code\":\"1031.5031\",\"matrix\":\"5031\",\"screw\":21,\"vfd_speed\":710,\"extrusion_speed\":14.2,\"density\":0.51,\"cut_length\":142},"
"{\"code\":\"1032.5032\",\"matrix\":\"5032\",\"screw\":22,\"vfd_speed\":720,\"extrusion_speed\":14.4,\"density\":0.52,\"cut_length\":144},"
"{\"code\":\"1033.5033\",\"matrix\":\"5033\",\"screw\":23,\"vfd_speed\":730,\"extrusion_speed\":14.6,\"density\":0.53,\"cut_length\":146},"
"{\"code\":\"1034.5034\",\"matrix\":\"5034\",\"screw\":24,\"vfd_speed\":740,\"extrusion_speed\":14.8,\"density\":0.54,\"cut_length\":148},"
"{\"code\":\"1035.5035\",\"matrix\":\"5035\",\"screw\":25,\"vfd_speed\":750,\"extrusion_speed\":15.0,\"density\":0.55,\"cut_length\":150},"
"{\"code\":\"1036.5036\",\"matrix\":\"5036\",\"screw\":26,\"vfd_speed\":760,\"extrusion_speed\":15.2,\"density\":0.56,\"cut_length\":152},"
"{\"code\":\"1037.5037\",\"matrix\":\"5037\",\"screw\":27,\"vfd_speed\":770,\"extrusion_speed\":15.4,\"density\":0.57,\"cut_length\":154},"
"{\"code\":\"1038.5038\",\"matrix\":\"5038\",\"screw\":28,\"vfd_speed\":780,\"extrusion_speed\":15.6,\"density\":0.58,\"cut_length\":156},"
"{\"code\":\"1039.5039\",\"matrix\":\"5039\",\"screw\":29,\"vfd_speed\":790,\"extrusion_speed\":15.8,\"density\":0.59,\"cut_length\":158},"

"{\"code\":\"1040.5040\",\"matrix\":\"5040\",\"screw\":20,\"vfd_speed\":800,\"extrusion_speed\":16.0,\"density\":0.60,\"cut_length\":160},"
"{\"code\":\"1041.5041\",\"matrix\":\"5041\",\"screw\":21,\"vfd_speed\":810,\"extrusion_speed\":16.2,\"density\":0.61,\"cut_length\":162},"
"{\"code\":\"1042.5042\",\"matrix\":\"5042\",\"screw\":22,\"vfd_speed\":820,\"extrusion_speed\":16.4,\"density\":0.62,\"cut_length\":164},"
"{\"code\":\"1043.5043\",\"matrix\":\"5043\",\"screw\":23,\"vfd_speed\":830,\"extrusion_speed\":16.6,\"density\":0.63,\"cut_length\":166},"
"{\"code\":\"1044.5044\",\"matrix\":\"5044\",\"screw\":24,\"vfd_speed\":840,\"extrusion_speed\":16.8,\"density\":0.64,\"cut_length\":168},"
"{\"code\":\"1045.5045\",\"matrix\":\"5045\",\"screw\":25,\"vfd_speed\":850,\"extrusion_speed\":17.0,\"density\":0.65,\"cut_length\":170},"
"{\"code\":\"1046.5046\",\"matrix\":\"5046\",\"screw\":26,\"vfd_speed\":860,\"extrusion_speed\":17.2,\"density\":0.66,\"cut_length\":172},"
"{\"code\":\"1047.5047\",\"matrix\":\"5047\",\"screw\":27,\"vfd_speed\":870,\"extrusion_speed\":17.4,\"density\":0.67,\"cut_length\":174},"
"{\"code\":\"1048.5048\",\"matrix\":\"5048\",\"screw\":28,\"vfd_speed\":880,\"extrusion_speed\":17.6,\"density\":0.68,\"cut_length\":176},"
"{\"code\":\"1049.5049\",\"matrix\":\"5049\",\"screw\":29,\"vfd_speed\":890,\"extrusion_speed\":17.8,\"density\":0.69,\"cut_length\":178}"
"]"
"}";

    return storage_nvs_save_profiles(default_json);
}

// =========================
// SAVE WIFI
// =========================
esp_err_t storage_nvs_save_wifi(const char *ssid, const char *password)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_set_str(handle, KEY_WIFI_SSID, ssid ? ssid : "");
    if (err != ESP_OK)
    {
        nvs_close(handle);
        return err;
    }

    err = nvs_set_str(handle, KEY_WIFI_PASS, password ? password : "");
    if (err != ESP_OK)
    {
        nvs_close(handle);
        return err;
    }

    err = nvs_commit(handle);

    nvs_close(handle);

    return err;
}

// =========================
// LOAD WIFI
// =========================
esp_err_t storage_nvs_load_wifi(char *ssid, size_t ssid_max_len,
                                char *password, size_t pass_max_len)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK)
        return err;

    size_t ssid_len = ssid_max_len;
    size_t pass_len = pass_max_len;

    err = nvs_get_str(handle, KEY_WIFI_SSID, ssid, &ssid_len);
    if (err != ESP_OK)
    {
        nvs_close(handle);
        return err;
    }

    err = nvs_get_str(handle, KEY_WIFI_PASS, password, &pass_len);

    nvs_close(handle);

    return err;
}

// =========================
// SAVE THEME
// =========================
void storage_nvs_save_theme(int theme_id)
{
    nvs_handle_t handle;
    if (nvs_open(NAMESPACE, NVS_READWRITE, &handle) != ESP_OK)
        return;
    nvs_set_i32(handle, KEY_THEME, (int32_t)theme_id);
    nvs_commit(handle);
    nvs_close(handle);
}

// =========================
// LOAD THEME
// =========================
int storage_nvs_load_theme(void)
{
    nvs_handle_t handle;
    if (nvs_open(NAMESPACE, NVS_READONLY, &handle) != ESP_OK)
        return 0;
    int32_t val = 0;
    nvs_get_i32(handle, KEY_THEME, &val);
    nvs_close(handle);
    return (int)val;
}

// =========================
// SAVE ALARM CONFIG
// =========================
void storage_nvs_save_alarm(bool enabled, int threshold)
{
    nvs_handle_t handle;
    if (nvs_open(NAMESPACE, NVS_READWRITE, &handle) != ESP_OK)
        return;
    nvs_set_i32(handle, KEY_ALARM_ENABLED, (int32_t)enabled);
    nvs_set_i32(handle, KEY_ALARM_THRESHOLD, (int32_t)threshold);
    nvs_commit(handle);
    nvs_close(handle);
}

// =========================
// LOAD ALARM CONFIG
// =========================
void storage_nvs_load_alarm(bool *enabled, int *threshold)
{
    nvs_handle_t handle;
    if (nvs_open(NAMESPACE, NVS_READONLY, &handle) != ESP_OK)
        return;
    int32_t en = 0, thr = 10;
    nvs_get_i32(handle, KEY_ALARM_ENABLED, &en);
    nvs_get_i32(handle, KEY_ALARM_THRESHOLD, &thr);
    nvs_close(handle);
    if (enabled)   *enabled   = (bool)en;
    if (threshold) *threshold = (int)thr;
}

// =========================
// SAVE PRE-CUT ALARM CONFIG
// =========================
void storage_nvs_save_pre_cut_alarm(bool enabled, int seconds)
{
    nvs_handle_t handle;
    if (nvs_open(NAMESPACE, NVS_READWRITE, &handle) != ESP_OK)
        return;
    nvs_set_i32(handle, KEY_PRECUT_ENABLED, (int32_t)enabled);
    nvs_set_i32(handle, KEY_PRECUT_SECONDS, (int32_t)seconds);
    nvs_commit(handle);
    nvs_close(handle);
}

// =========================
// LOAD PRE-CUT ALARM CONFIG
// =========================
void storage_nvs_load_pre_cut_alarm(bool *enabled, int *seconds)
{
    nvs_handle_t handle;
    if (nvs_open(NAMESPACE, NVS_READONLY, &handle) != ESP_OK)
        return;
    int32_t en = 0, sec = 3;
    nvs_get_i32(handle, KEY_PRECUT_ENABLED, &en);
    nvs_get_i32(handle, KEY_PRECUT_SECONDS, &sec);
    nvs_close(handle);
    if (enabled) *enabled = (bool)en;
    if (seconds) *seconds = (int)sec;
}

// =========================
// CHECK WIFI EXISTS
// =========================
bool storage_nvs_wifi_exists(void)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK)
        return false;

    size_t required_size = 0;

    err = nvs_get_str(handle, KEY_WIFI_SSID, NULL, &required_size);

    nvs_close(handle);

    return (err == ESP_OK && required_size > 1);
}