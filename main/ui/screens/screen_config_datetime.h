#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *screen_config_datetime_create(lv_obj_t *parent);
void screen_config_datetime_update(void);

#ifdef __cplusplus
}
#endif