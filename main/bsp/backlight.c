#include "backlight.h"
#include "waveshare_rgb_lcd_port.h"

void backlight_on(void)
{
    waveshare_rgb_lcd_bl_on();
}

void backlight_off(void)
{
    waveshare_rgb_lcd_bl_off();
}