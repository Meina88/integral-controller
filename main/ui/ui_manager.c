#include "ui_events.h"
#include "ui.h"   // importante

void ui_start(void)
{
    ui_init();

    // 🔥 enganchar evento al botón
    lv_obj_add_event_cb(ui_BTN_Cancel_Top, btn_toggle_relay_cb, LV_EVENT_CLICKED, NULL);
}