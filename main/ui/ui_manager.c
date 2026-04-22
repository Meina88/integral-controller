#include "ui_manager.h"
#include "lvgl.h"
#include "ui/screens/screen_home.h"

void ui_init(void)
{
    lv_obj_clean(lv_scr_act());
    screen_home_create();
}