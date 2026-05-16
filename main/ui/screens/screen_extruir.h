#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *screen_extruir_create(lv_obj_t *parent);
void screen_extruir_update(void);
void screen_extruir_refresh_profile(void);
bool screen_extruir_is_alarm_active(void);

#ifdef __cplusplus
}
#endif