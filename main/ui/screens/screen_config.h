#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *screen_config_create(lv_obj_t *parent);
void screen_config_update(void);
void screen_config_show_machine(void);

#ifdef __cplusplus
}
#endif