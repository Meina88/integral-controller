#include "waveshare_rgb_lcd_port.h"
#include "ui/ui_manager.h"

void app_main(void)
{
    waveshare_esp32_s3_rgb_lcd_init();

    ui_init();
}