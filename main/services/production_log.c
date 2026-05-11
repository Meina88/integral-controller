#include "production_log.h"
#include "drivers/rtc/rtc.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define LOG_PATH_MAX 128

static char current_file[LOG_PATH_MAX];

// =========================
// GENERAR NOMBRE ARCHIVO
// =========================
static void build_filename(void)
{
    char date[32];

    rtc_get_date_filename_string(date);

    snprintf(current_file, sizeof(current_file),
             "/sdcard/logs/production_%s.csv",
             date);
}

// =========================
// VERIFICAR EXISTENCIA
// =========================
static bool file_exists(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f)
    {
        fclose(f);
        return true;
    }
    return false;
}

// =========================
// INIT CSV
// =========================
void production_log_init(void)
{
    build_filename();

    if (file_exists(current_file))
    {
        printf("[LOG] CSV ya existe: %s\n", current_file);
        return;
    }

    FILE *f = fopen(current_file, "w");
    if (!f)
    {
        printf("[LOG][ERROR] No se pudo crear CSV\n");
        return;
    }

    // 🔥 HEADER PRO
    fprintf(f,
            "sep=;\n"
            "start_time;end_time;profile;length_m;speed_avg;cuts;finish_reason\n");

    fflush(f);
    fclose(f);

    printf("[LOG] CSV creado: %s\n", current_file);
}

// =========================
// APPEND ROW
// =========================
void production_log_append(
    const char *start_time,
    const char *end_time,
    const char *profile,
    float length_m,
    float speed_avg,
    int cuts,
    const char *finish_reason)
{
    build_filename();

    FILE *f = fopen(current_file, "a");
    if (!f)
    {
        printf("[LOG][ERROR] No se pudo abrir CSV\n");
        return;
    }

    int written = fprintf(f,
                          "%s;%s;%s;%.3f;%.2f;%d;%s\n",
                          start_time ? start_time : "N/A",
                          end_time ? end_time : "N/A",
                          profile ? profile : "N/A",
                          length_m,
                          speed_avg,
                          cuts,
                          finish_reason ? finish_reason : "unknown");

    if (written <= 0)
    {
        printf("[LOG][ERROR] Error escribiendo fila\n");
    }
    else
    {
        printf("[LOG] OK → %s | %.2f m | %.2f m/min | cuts=%d\n",
               profile,
               length_m,
               speed_avg,
               cuts);
    }

    fflush(f);
    fclose(f);
}