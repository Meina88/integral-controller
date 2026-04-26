#include "production.h"
#include "logic/extrusion.h"
#include "logic/active_profile.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static bool running = false;

void production_start(void)
{
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

    // 🔥 CORRECCIÓN CLAVE
    float meters = extrusion_get_total_mm() / 1000.0f;
    float speed  = extrusion_get_speed_m_min();

    FILE *f = fopen("/sdcard/logs/production.log", "a");
    if (!f)
        return;

    fprintf(f,
        "{\"profile\":\"%s\",\"meters\":%.2f,\"speed\":%.2f}\n",
        profile,
        meters,
        speed
    );

    fclose(f);
}

bool production_is_running(void)
{
    return running;
}