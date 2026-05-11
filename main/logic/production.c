#include "production.h"
#include "logic/extrusion.h"
#include "logic/active_profile.h"
#include "drivers/rtc/rtc.h"
#include "logic/profile.h"
#include "services/production_log.h"
#include "drivers/digital_outputs.h"
#include "lvgl.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static bool running = false;

void production_start(void)
{
    extrusion_start();
    recording_start();

    running = true;
}

void production_finish(production_finish_reason_t reason)
{
    if (!running)
        return;

    running = false;

    const char *profile = active_profile_get();

    if (!profile || strlen(profile) == 0)
        return;

    // =========================
    // MARCA FINAL MANUAL
    // =========================
    if (reason == PRODUCTION_FINISH_MANUAL)
    {
        relay_1_on();

        lv_delay_ms(500);

        relay_1_off();
    }

    // =========================
    // DATOS
    // =========================
    float meters = extrusion_get_total_mm() / 1000.0f;

    float speed = extrusion_get_avg_speed();

    int cuts = extrusion_get_total_count();

    // =========================
    // PERFIL
    // =========================
    profile_t p;

    // =========================
    // PARAR
    // =========================
    extrusion_stop();

    recording_stop();

    // =========================
    // TIEMPOS
    // =========================
    const char *start = extrusion_get_start_time();

    const char *end = extrusion_get_end_time();

    // =========================
    // FINISH REASON
    // =========================
    const char *finish_reason = "manual";

    if (reason == PRODUCTION_FINISH_TARGET)
    {
        finish_reason = "target";
    }

    // =========================
    // LOG
    // =========================
    production_log_append(
        start,
        end,
        profile,
        meters,
        speed,
        cuts,
        finish_reason);

    printf("LOG CSV GUARDADO OK\n");
}

bool production_is_running(void)
{
    return running;
}