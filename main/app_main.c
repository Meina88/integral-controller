#include "bsp/display.h"
#include "bsp/backlight.h"
#include "ui/ui_manager.h"
#include "lvgl_port.h"

#include "DEV_Config.h"
#include "CH422G.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

void app_main(void)
{
    display_init();   // LCD + TOUCH + LVGL
    backlight_on();

    // Inicializa el CH422G
    if (DEV_Module_Init() != 0) {
        printf("Error: DEV_Module_Init fallo\n");
    } else {
        printf("CH422G inicializado OK\n");
    }

    if (lvgl_port_lock(-1))
    {
        ui_init();
        lvgl_port_unlock();
    }

    // Test: parpadeo DO0
while (1)
{
    printf("DO0 ON\n");
    CH422G_od_output(0x01);

    vTaskDelay(pdMS_TO_TICKS(200));  // 🔥 pequeño delay

    uint8_t di = CH422G_io_input(0x01);  // DI0
    printf("DI0 = %d\n", di);

    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("DO0 OFF\n");
    CH422G_od_output(0x00);

    vTaskDelay(pdMS_TO_TICKS(200));  // 🔥 pequeño delay

    di = CH422G_io_input(0x01);
    printf("DI0 = %d\n", di);

    vTaskDelay(pdMS_TO_TICKS(1000));
}
}