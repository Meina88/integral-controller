/*
 * sim_main.c — Entry point del simulador de UI para PC.
 *
 * Inicializa LVGL 9.x con backend SDL2 (800×480), carga la lógica y
 * arranca la UI sin FreeRTOS ni hardware real.
 */

#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

/* LVGL 9.x exporta los drivers SDL cuando LV_USE_SDL=1 */
#include "lvgl.h"

#include "logic/alarm_config.h"
#include "logic/calibration.h"
#include "logic/alarm.h"
#include "storage/nvs/storage_nvs.h"
#include "ui/ui_manager.h"

#define SIM_WIDTH  800
#define SIM_HEIGHT 480

int main(int argc, char *argv[])
{
    printf("[SIM] Iniciando simulador de UI — Extrusion Controller\n");

    /* ── LVGL init ────────────────────────────────────────────────────── */
    lv_init();

    /* Ventana SDL2 como display principal */
    lv_display_t *disp = lv_sdl_window_create(SIM_WIDTH, SIM_HEIGHT);
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);

    /* Dispositivos de entrada */
    lv_indev_t *mouse = lv_sdl_mouse_create();
    (void)mouse;

    lv_indev_t *wheel = lv_sdl_mousewheel_create();
    (void)wheel;

    lv_indev_t *kb = lv_sdl_keyboard_create();
    (void)kb;

    /* ── Módulos de lógica ────────────────────────────────────────────── */
    storage_nvs_init();

    if (!storage_nvs_profiles_exist())
        storage_nvs_load_defaults();

    alarm_config_init();
    calibration_init();
    alarm_init();

    /* ── UI ───────────────────────────────────────────────────────────── */
    ui_start();

    printf("[SIM] UI activa. Cierre la ventana SDL2 para salir.\n");

    /* ── Loop principal ───────────────────────────────────────────────── */
    while (1) {
        uint32_t delay_ms = lv_timer_handler();
        if (delay_ms == 0 || delay_ms > 16)
            delay_ms = 5;

        ui_update();
        SDL_Delay(delay_ms);
    }

    return 0;
}
