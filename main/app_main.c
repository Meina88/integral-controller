#include "bsp/display.h"
#include "bsp/backlight.h"
#include "ui/ui_manager.h"
#include "lvgl_port.h"

#include "DEV_Config.h"
#include "CH422G.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#include "drivers/relay.h"
#include "drivers/rtc/rtc.h"
#include "storage/sdcard/sdcard.h"

// =========================
// MAIN
// =========================
void app_main(void)
{
    // =========================
    // DISPLAY (🔥 CREA I2C)
    // =========================
    display_init();
    backlight_on();

    // =========================
    // SD DESPUÉS (usa ese I2C)
    // =========================
    printf("Inicializando SD...\n");

    if (sdcard_init() == ESP_OK)
    {
        printf("SD montada correctamente\n");
        // sdcard_test(); // opcional
    }
    else
    {
        printf("Error montando SD\n");
    }

    // =========================
    // RELAY
    // =========================
    relay_init();

    // =========================
    // RTC
    // =========================
    rtc_hw_init();
    printf("RTC inicializado\n");

    // =========================
    // UI (LVGL)
    // =========================
    if (lvgl_port_lock(-1))
    {
        ui_init();
        lvgl_port_unlock();
    }

    // =========================
    // LOOP PRINCIPAL
    // =========================
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}