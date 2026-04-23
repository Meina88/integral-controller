#include "drivers/digital_outputs.h"
#include "lvgl.h"

void relay_press_event(lv_event_t *event)
{
    relay_2_on();
}

void relay_release_event(lv_event_t *event)
{
    relay_2_off();
}
