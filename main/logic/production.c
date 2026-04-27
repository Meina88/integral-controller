#include "production.h"
#include "logic/extrusion.h"
#include "logic/active_profile.h"
#include "drivers/rtc/rtc.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static bool running = false;

void production_start(void)
{
    recording_start();
    running = true;
}

void production_stop(void)
{
    if (!running)
        return;

    running = false;

    extrusion_stop();

    const char *profile = active_profile_get();

    if (!profile || strlen(profile) == 0)
        return;

    float meters = extrusion_get_total_mm() / 1000.0f;
    float speed  = extrusion_get_speed_m_min();

    // 🔥 obtener hora
    char time_str[32];
    rtc_get_time_string(time_str);

    FILE *f = fopen("/sdcard/logs/production.log", "a");
    if (!f)
        return;

    // 🔥 log con timestamp
    fprintf(f,
        "{\"time\":\"%s\",\"profile\":\"%s\",\"meters\":%.2f,\"speed\":%.2f}\n",
        time_str,
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