#include "ui_theme.h"
#include "storage/nvs/storage_nvs.h"

static ui_theme_t current;

static void apply_dark(void)
{
    // Base surfaces — slate-900 / slate-800 / deep navy
    current.bg          = lv_color_hex(0x0F1724);  // fondo principal
    current.surface     = lv_color_hex(0x1A2436);  // cards / paneles
    current.surface2    = lv_color_hex(0x0A1020);  // acento profundo / sidebar
    current.preview     = lv_color_hex(0x1A2535);  // barra de preview

    // Interactivos
    current.border      = lv_color_hex(0x2D4060);  // bordes sutiles (más azul)
    current.pressed     = lv_color_hex(0x243247);  // fondo al presionar
    current.btn_grey    = lv_color_hex(0x2D3F55);  // botones neutros

    // Texto — contrastes WCAG AA (≥ 4.5:1 sobre bg)
    current.text        = lv_color_hex(0xF0F4F8);  // texto primario  ~15:1
    current.muted       = lv_color_hex(0x94A3B8);  // texto secundario ~7:1
    current.subtle      = lv_color_hex(0xCBD5E1);  // texto terciario ~11:1

    // Acento
    current.blue        = lv_color_hex(0x2563EB);  // azul principal
    current.blue_accent = lv_color_hex(0x93C5FD);  // azul claro (highlights)
    current.green       = lv_color_hex(0x22C55E);  // verde (éxito)
    current.red         = lv_color_hex(0xEF4444);  // rojo (error/stop)

    current.id = UI_THEME_DARK;
}

static void apply_light(void)
{
    // Base surfaces — tonos fríos claros
    current.bg          = lv_color_hex(0xF1F5F9);  // slate-100
    current.surface     = lv_color_hex(0xFFFFFF);  // blanco puro para cards
    current.surface2    = lv_color_hex(0xE2EAF4);  // slate-200 levemente azul
    current.preview     = lv_color_hex(0xDDE8F5);  // preview teclado

    // Interactivos
    current.border      = lv_color_hex(0xC5D5E8);  // borde sutil frío
    current.pressed     = lv_color_hex(0xD8E6F3);  // presionado
    current.btn_grey    = lv_color_hex(0x4A5568);  // botones neutros oscuros

    // Texto — contrastes WCAG AA sobre bg claro
    current.text        = lv_color_hex(0x0F1724);  // slate-900  ~14:1
    current.muted       = lv_color_hex(0x3D5066);  // slate-700  ~7:1
    current.subtle      = lv_color_hex(0x526070);  // slate-600  ~5:1

    // Acento (mismo en ambos temas)
    current.blue        = lv_color_hex(0x1D4ED8);  // blue-700
    current.blue_accent = lv_color_hex(0x3B82F6);  // blue-500
    current.green       = lv_color_hex(0x16A34A);  // green-700
    current.red         = lv_color_hex(0xDC2626);  // red-600

    current.id = UI_THEME_LIGHT;
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
