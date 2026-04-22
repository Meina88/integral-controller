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

void app_main(void)
{
    // =========================
    // NVS (primero)
    // =========================
    nvs_flash_init();

    // =========================
    // DISPLAY
    // =========================
    display_init();
    backlight_on();

    // =========================
    // DRIVERS
    // =========================
    digital_outputs_init();   // 🔥 CAMBIO CLAVE
    rtc_hw_init();

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
    // UI
    // =========================
    if (lvgl_port_lock(-1))
    {
        ui_start();
        lvgl_port_unlock();
    }

    // =========================
    // COMMS
    // =========================
    wifi_init_sta();

    vTaskDelay(pdMS_TO_TICKS(5000));

    start_http_server();

    // =========================
    // LOOP
    // =========================
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}