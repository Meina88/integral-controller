#include "ui_manager.h"
#include "lvgl.h"
#include "ui/screens/screen_home.h"

void ui_init(void)
{
    // 🔥 limpiar cualquier UI previa
    lv_obj_clean(lv_scr_act());

    // 🔥 crear tu pantalla
    screen_home_create();
}