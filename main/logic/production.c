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

    // 🔥 LEER DATOS ANTES DE APAGAR
    float meters = extrusion_get_total_mm() / 1000.0f;
    float speed = extrusion_get_speed_m_min();

    char time_str[32];
    rtc_get_time_string(time_str);

    // 🔥 AHORA SÍ PARAR TODO
    extrusion_stop();
    recording_stop();

    // 🔥 GUARDAR
    FILE *f = fopen("/sdcard/logs/production.log", "a");
    if (!f)
    {
        printf("ERROR guardando log\n");
        return;
    }

    fprintf(f,
            "{\"profile\":\"%s\",\"time\":\"%s\",\"meters\":%.2f,\"speed\":%.2f}\n",
            profile,
            time_str,
            meters,
            speed);
    fflush(f);
    fclose(f);

    printf("LOG GUARDADO OK\n");
}

bool production_is_running(void)
{
    return running;
}