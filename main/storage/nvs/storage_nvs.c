#include "storage_nvs.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdio.h>

#define NAMESPACE "storage"
#define KEY_PROFILES "profiles_json"

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
    "{"
    "\"code\":\"1234.5678\","
    "\"matrix\":\"5678\","
    "\"screw\":25,"
    "\"vfd_speed\":500,"
    "\"extrusion_speed\":11,"
    "\"density\":0.27,"
    "\"cut_length\":100"
    "},"
    "{"
    "\"code\":\"1239.5679\","
    "\"matrix\":\"5679\","
    "\"screw\":28,"
    "\"vfd_speed\":550,"
    "\"extrusion_speed\":12,"
    "\"density\":0.29,"
    "\"cut_length\":120"
    "}"
    "]"
    "}";

    return storage_nvs_save_profiles(default_json);
}