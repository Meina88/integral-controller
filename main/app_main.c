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
#include "logic/extrusion.h"

// 🔥 DECLARACIÓN
void rtc_set_manual_time(void);

void app_main(void)
{
    // =========================
    // NVS (SIEMPRE PRIMERO)
    // =========================
    ESP_ERROR_CHECK(nvs_flash_init());

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

    // 🔥 SETEO MANUAL (SOLO UNA VEZ)
    // rtc_set_manual_time();
    // ⚠️ después de probar → COMENTAR ESTA LÍNEA

    // =========================
    // STORAGE
    // =========================
    printf("Inicializando SD...\n");

    if (sdcard_init() == ESP_OK)
    {
        printf("SD montada correctamente\n");
        sdcard_test();
    }
    else
    {
        printf("Error montando SD\n");
    }

    // =========================
    // UI (LVGL)
    // =========================
    if (lvgl_port_lock(-1))
    {
        ui_start();
        lvgl_port_unlock();
    }

    extrusion_start();

    // =========================
    // COMMS
    // =========================
    wifi_init_sta();

    vTaskDelay(pdMS_TO_TICKS(2000));

    start_http_server();

    // =========================
    // LOOP
    // =========================
    while (1)
    {
        // DEBUG
        printf("loop\n");

        // 🔥 VERIFICACIÓN RTC
        char dt[32];
        rtc_get_datetime_string(dt);
        printf("RTC: %s\n", dt);

        // lógica de extrusión
        extrusion_process_tick();

        // actualizar UI
        if (lvgl_port_lock(-1))
        {
            ui_update();
            lvgl_port_unlock();
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}