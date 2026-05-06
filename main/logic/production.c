#include "production.h"
#include "logic/extrusion.h"
#include "logic/active_profile.h"
#include "drivers/rtc/rtc.h"
#include "logic/profile.h"
#include "services/production_log.h"
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

void production_stop(void)
{
    if (!running)
        return;

    running = false;

    const char *profile = active_profile_get();

    if (!profile || strlen(profile) == 0)
        return;

    // 🔥 DATOS
    float meters = extrusion_get_total_mm() / 1000.0f;
    float speed = extrusion_get_avg_speed();
    int cuts = extrusion_get_total_count();

    // 🔥 PERFIL
    profile_t p;
    bool ok = profile_get_by_code(profile, &p);

    // 🔥 PARAR (esto fija end_time)
    extrusion_stop();
    recording_stop();

    // 🔥 TIEMPOS CORRECTOS
    const char *start = extrusion_get_start_time();
    const char *end = extrusion_get_end_time();

    // 🔥 LOG
production_log_append(
    start,
    end,
    profile,
    meters,
    speed,
    cuts);

    printf("LOG CSV GUARDADO OK\n");
}

bool production_is_running(void)
{
    return running;
}