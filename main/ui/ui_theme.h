#pragma once
#include "lvgl.h"
#include <stdbool.h>

typedef enum {
    UI_THEME_DARK  = 0,
    UI_THEME_LIGHT = 1,
} ui_theme_id_t;

typedef struct {
    // Fondos
    lv_color_t bg;         // fondo principal de contenido
    lv_color_t surface;    // cards, paneles
    lv_color_t surface2;   // acento más profundo / sidebar izq
    lv_color_t preview;    // barra de preview (teclado)

    // Interactivos
    lv_color_t border;
    lv_color_t pressed;

    // Texto
    lv_color_t text;       // texto primario
    lv_color_t muted;      // texto secundario
    lv_color_t subtle;     // texto terciario / placeholder

    // Acento (igual en ambos temas)
    lv_color_t blue;
    lv_color_t blue_accent;
    lv_color_t green;
    lv_color_t red;
    lv_color_t btn_grey;

    ui_theme_id_t id;
} ui_theme_t;

void              ui_theme_init(void);
const ui_theme_t *ui_theme_get(void);
void              ui_theme_set(ui_theme_id_t id);
