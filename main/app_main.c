#include "bsp/display.h"
#include "bsp/backlight.h"
#include "ui/ui_manager.h"
#include "lvgl_port.h"

void app_main(void)
{
    display_init();   // LCD + TOUCH + LVGL
    backlight_on();

    if (lvgl_port_lock(-1))   // 🔥 CLAVE
    {
        ui_init();            // crear UI
        lvgl_port_unlock();
    }
}