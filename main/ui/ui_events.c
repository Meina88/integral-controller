#include "ui_events.h"
#include "drivers/digital_outputs.h"

static bool relay_state = false;

void btn_toggle_relay_cb(lv_event_t * e)
{
    relay_state = !relay_state;

    if (relay_state)
    {
        relay_1_on();
    }
    else
    {
        relay_1_off();
    }
}