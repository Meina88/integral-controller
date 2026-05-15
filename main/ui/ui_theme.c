#include "ui_theme.h"
#include "storage/nvs/storage_nvs.h"

static ui_theme_t current;

static void apply_dark(void)
{
    current.bg          = lv_color_hex(0x111827);
    current.surface     = lv_color_hex(0x1E293B);
    current.surface2    = lv_color_hex(0x0D1117);
    current.preview     = lv_color_hex(0x1A2535);
    current.border      = lv_color_hex(0x334155);
    current.pressed     = lv_color_hex(0x2D3748);
    current.text        = lv_color_hex(0xF1F5F9);
    current.muted       = lv_color_hex(0x64748B);
    current.subtle      = lv_color_hex(0x94A3B8);
    current.blue        = lv_color_hex(0x1D4ED8);
    current.blue_accent = lv_color_hex(0x93C5FD);
    current.green       = lv_color_hex(0x16A34A);
    current.red         = lv_color_hex(0xDC2626);
    current.btn_grey    = lv_color_hex(0x374151);
    current.id          = UI_THEME_DARK;
}

static void apply_light(void)
{
    current.bg          = lv_color_hex(0xF0F4F8);
    current.surface     = lv_color_hex(0xFFFFFF);
    current.surface2    = lv_color_hex(0xE8EEF4);
    current.preview     = lv_color_hex(0xDDE5EF);
    current.border      = lv_color_hex(0xC8D5E3);
    current.pressed     = lv_color_hex(0xD5E0EC);
    current.text        = lv_color_hex(0x0F172A);
    current.muted       = lv_color_hex(0x4A5568);
    current.subtle      = lv_color_hex(0x718096);
    current.blue        = lv_color_hex(0x1D4ED8);
    current.blue_accent = lv_color_hex(0x3B82F6);
    current.green       = lv_color_hex(0x16A34A);
    current.red         = lv_color_hex(0xDC2626);
    current.btn_grey    = lv_color_hex(0x475569);
    current.id          = UI_THEME_LIGHT;
}

void ui_theme_init(void)
{
    int saved = storage_nvs_load_theme();
    if (saved == (int)UI_THEME_LIGHT)
        apply_light();
    else
        apply_dark();
}

const ui_theme_t *ui_theme_get(void)
{
    return &current;
}

void ui_theme_set(ui_theme_id_t id)
{
    if (id == UI_THEME_LIGHT)
        apply_light();
    else
        apply_dark();
    storage_nvs_save_theme((int)id);
}
