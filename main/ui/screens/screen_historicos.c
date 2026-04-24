#include "screen_historicos.h"

static lv_obj_t *root;

lv_obj_t *screen_historicos_create(lv_obj_t *parent)
{
    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));

    lv_obj_t *label = lv_label_create(root);
    lv_label_set_text(label, "Historicos (próximamente)");
    lv_obj_center(label);

    return root;
}

void screen_historicos_update(void)
{
    // futuro: mostrar logs SD
}