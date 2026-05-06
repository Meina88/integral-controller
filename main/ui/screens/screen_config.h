#pragma once
#include "drivers/rtc/rtc.h"
#include "drivers/rtc/rtc_pcf85063a.h"

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *screen_config_create(lv_obj_t *parent);
void screen_config_update(void);

#ifdef __cplusplus
}
#endif