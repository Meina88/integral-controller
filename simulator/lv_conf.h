/**
 * lv_conf.h — Configuración LVGL 9.3.0 para simulador PC con SDL2
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 16

/*====================
   MEMORY SETTINGS
 *====================*/
#define LV_MEM_CUSTOM 0
#if LV_MEM_CUSTOM == 0
    #define LV_MEM_SIZE (256 * 1024U)
    #define LV_MEM_ADR  0U
    #define LV_MEM_POOL_INCLUDE <stdlib.h>
    #define LV_MEM_POOL_ALLOC   malloc
    #define LV_MEM_POOL_FREE    free
#endif

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DEF_REFR_PERIOD  33

/* Tick via SDL2 */
#define LV_TICK_CUSTOM              1
#define LV_TICK_CUSTOM_INCLUDE      <SDL2/SDL.h>
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (SDL_GetTicks())

#define LV_DPI_DEF  130

/*====================
   DRAW CONFIGURATION
 *====================*/
#define LV_DRAW_BUF_STRIDE_ALIGN    1
#define LV_DRAW_BUF_ALIGN           4
/* Deshabilitar ensamblador ARM (Helium/NEON) — no válido en x86 */
#define LV_DRAW_SW_ASM              0

/*====================
   FEATURE CONFIGURATION
 *====================*/
#define LV_USE_FLEX     1
#define LV_USE_GRID     1
#define LV_USE_ANIM     1

/*====================
   LOGGING
 *====================*/
#define LV_USE_LOG          1
#define LV_LOG_LEVEL        LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF       1
#define LV_LOG_TIMESTAMP_FORMAT LV_LOG_TIMESTAMP_FORMAT_NONE

/*====================
   ASSERTS
 *====================*/
#define LV_USE_ASSERT_NULL           1
#define LV_USE_ASSERT_MALLOC         1
#define LV_USE_ASSERT_STYLE          0
#define LV_USE_ASSERT_MEM_INTEGRITY  0
#define LV_USE_ASSERT_OBJ            0
#define LV_ASSERT_HANDLER_INCLUDE    <stdint.h>
#define LV_ASSERT_HANDLER            while(1);

/*====================
   FONTS
 *====================*/
#define LV_USE_FONT_COMPRESSED  1   /* Las fuentes Inter del proyecto usan formato comprimido */
#define LV_USE_FONT_PLACEHOLDER 1
/* Font predeterminada (el proyecto usa inter_18/24/28 via LV_FONT_DECLARE) */
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_DEFAULT         &lv_font_montserrat_14

/*====================
   WIDGETS (todos habilitados para que los ejemplos internos de LVGL compilen)
 *====================*/
#define LV_USE_ARC          1
#define LV_USE_BAR          1
#define LV_USE_BTN          1
#define LV_USE_BTNMATRIX    1
#define LV_USE_CANVAS       1
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1
#define LV_USE_IMG          1
#define LV_USE_LABEL        1
#define LV_USE_LINE         1
#define LV_USE_ROLLER       1
#define LV_USE_SLIDER       1
#define LV_USE_SWITCH       1
#define LV_USE_TEXTAREA     1
#define LV_USE_TABLE        1
#define LV_USE_KEYBOARD     1
#define LV_USE_LIST         1
#define LV_USE_SPINNER      1
#define LV_USE_TABVIEW      1
#define LV_USE_LED          1
#define LV_USE_MENU         1
#define LV_USE_MSGBOX       1
#define LV_USE_TILEVIEW     1
#define LV_USE_WIN          1
#define LV_USE_CALENDAR     1
#define LV_USE_CHART        1
#define LV_USE_COLORWHEEL   1
#define LV_USE_IMGBTN       1
#define LV_USE_METER        1
#define LV_USE_SPINBOX      1
#define LV_USE_CALENDAR     0
#define LV_USE_CHART        0
#define LV_USE_COLORWHEEL   0
#define LV_USE_IMGBTN       0
#define LV_USE_METER        0
#define LV_USE_SPINBOX      0

/*====================
   EXTRA FEATURES
 *====================*/
#define LV_USE_OBSERVER     1
#define LV_USE_SNAPSHOT     1
#define LV_USE_XML          0

/*====================
   THEMES
 *====================*/
#define LV_USE_THEME_DEFAULT    1
#define LV_THEME_DEFAULT_DARK   0
#define LV_USE_THEME_SIMPLE     1
#define LV_USE_THEME_MONO       1

/*====================
   MISC
 *====================*/
#define LV_USE_USER_DATA    1
#define LV_USE_SPRINTF      1
#define LV_SPRINTF_USE_FLOAT 1

/*====================
   SDL2 DRIVER
 *====================*/
#define LV_USE_SDL              1
#define LV_SDL_INCLUDE_PATH     <SDL2/SDL.h>
#define LV_SDL_RENDER_MODE      LV_DISPLAY_RENDER_MODE_PARTIAL
#define LV_SDL_BUF_COUNT        2
#define LV_SDL_FULLSCREEN       0
#define LV_SDL_DIRECT_EXIT      1
#define LV_SDL_MOUSEWHEEL_MODE  LV_SDL_MOUSEWHEEL_MODE_ENCODER

/*====================
   FILE SYSTEM (no usado)
 *====================*/
#define LV_USE_FS_STDIO     0
#define LV_USE_FS_POSIX     0

/*====================
   IMAGE DECODERS
 *====================*/
#define LV_USE_BMP   0
#define LV_USE_PNG   0
#define LV_USE_GIF   0

/*====================
   DEMOS (no incluidos)
 *====================*/
#define LV_USE_DEMO_WIDGETS     0
#define LV_USE_DEMO_BENCHMARK   0
#define LV_USE_DEMO_STRESS      0
#define LV_USE_DEMO_MUSIC       0

#endif /* LV_CONF_H */
