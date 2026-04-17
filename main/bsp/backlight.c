#include "backlight.h"
#include "waveshare_rgb_lcd_port.h"

void backlight_on(void)
{
    wavesahre_rgb_lcd_bl_on();
}

void backlight_off(void)
{
    wavesahre_rgb_lcd_bl_off();
}