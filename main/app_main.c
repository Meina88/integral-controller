#include "bsp/display.h"
#include "bsp/backlight.h"
#include "ui/ui_manager.h"
#include "lvgl_port.h"

#include "DEV_Config.h"
#include "CH422G.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#include "digital_outputs.h"
#include "drivers/rtc/rtc.h"
#include "storage/sdcard/sdcard.h"

#include "wifi_manager.h"
#include "http_server.h"

#include "nvs_flash.h"
#include "storage/nvs/storage_nvs.h"
#include "logic/extrusion.h"

#include "services/production_log.h"

// 🔥 DECLARACIÓN
void rtc_set_manual_time(void);

// =========================
// TASK DEDICADA A EXTRUSIÓN
// =========================
static void extrusion_task(void *arg)
{
    while (1)
    {
        extrusion_process_tick();

        // Lectura rápida del sensor
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void app_main(void)
{
    // =========================
    // NVS BASE (ESP-IDF)
    // =========================
    ESP_ERROR_CHECK(nvs_flash_init());

    // =========================
    // STORAGE NVS (PERFILES)
    // =========================
    ESP_ERROR_CHECK(storage_nvs_init());

    if (!storage_nvs_profiles_exist())
    {
        printf("Cargando perfiles por defecto en NVS...\n");
        ESP_ERROR_CHECK(storage_nvs_load_defaults());
    }
    else
    {
        printf("Perfiles encontrados en NVS\n");
    }

    // =========================
    // LOW LEVEL HW
    // =========================
    DEV_Module_Init();

    // =========================
    // DISPLAY
    // =========================
    display_init();
    backlight_on();

    // =========================
    // DRIVERS
    // =========================
    digital_outputs_init();
    rtc_hw_init();

    // rtc_set_manual_time();

    // =========================
    // STORAGE SD (OPCIONAL)
    // =========================
    printf("Inicializando SD...\n");

    if (sdcard_init() == ESP_OK)
    {
        printf("SD montada correctamente\n");

        production_log_init();
    }
    else
    {
        printf("Error montando SD (no afecta perfiles)\n");
    }

    // =========================
    // COMMS
    // =========================
    wifi_init_sta();

    vTaskDelay(pdMS_TO_TICKS(2000));

    start_http_server();

    // =========================
    // UI (LVGL)
    // =========================
    if (lvgl_port_lock(-1))
    {
        ui_start();
        lvgl_port_unlock();
    }

    // =========================
    // LOGIC
    // =========================
    extrusion_start();

    xTaskCreate(
        extrusion_task,
        "extrusion_task",
        4096,
        NULL,
        5,
        NULL);

    // =========================
    // LOOP PRINCIPAL: SOLO UI
    // =========================
    while (1)
    {
        if (lvgl_port_lock(-1))
        {
            ui_update();
            lvgl_port_unlock();
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}