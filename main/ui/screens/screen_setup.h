#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *screen_setup_create(lv_obj_t *parent);
void screen_setup_update(void);

#ifdef __cplusplus
}
#endif