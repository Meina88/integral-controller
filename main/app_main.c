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

void app_main(void)
{
    display_init();
    backlight_on();

    if (DEV_Module_Init() != 0) {
        printf("Error: DEV_Module_Init fallo\n");
    } else {
        printf("CH422G inicializado OK\n");
    }

    relay_init();  // 🔥 importante

    if (lvgl_port_lock(-1))
    {
        ui_init();
        lvgl_port_unlock();
    }

    // 🔥 Nada más
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}