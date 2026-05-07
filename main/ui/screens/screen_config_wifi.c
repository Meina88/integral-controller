#include "screen_config_wifi.h"

lv_obj_t *screen_config_wifi_create(lv_obj_t *parent)
{
    lv_obj_t *root = lv_obj_create(parent);

    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));

    lv_obj_t *label = lv_label_create(root);

    lv_label_set_text(label, "CONFIG WIFI");

    lv_obj_center(label);

    return root;
}