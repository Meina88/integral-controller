/*
 * Mock de sd_files para el simulador de UI.
 * Devuelve dos perfiles demo en /sdcard/profiles.
 */
#include "storage/sdcard/sd_files.h"
#include <string.h>
#include <stdio.h>

#define PROFILE_DIR "/sdcard/profiles"

static const char *DEMO_CODES[] = { "SIM-001", "SIM-002" };
#define DEMO_COUNT 2

static const char *DEMO_SIM001 =
    "{\"general\":{\"code\":\"SIM-001\",\"commercial_name\":\"Demo A\"},"
    "\"geometry\":{\"matrix\":\"5000\",\"bocas\":1,\"area_mm2\":10.0},"
    "\"production\":{\"cut_options_m\":[0.08,0.10,0.12],\"default_cut_m\":0.08,\"allow_custom\":true},"
    "\"process\":{\"screw\":20,\"target_speed_vfd_rpm\":400,\"target_speed_belt_m_min\":8.0},"
    "\"engineering\":{\"theoretical_density_gr_m\":0.20,\"real_density_gr_m\":0.20}}";

static const char *DEMO_SIM002 =
    "{\"general\":{\"code\":\"SIM-002\",\"commercial_name\":\"Demo B\"},"
    "\"geometry\":{\"matrix\":\"5001\",\"bocas\":2,\"area_mm2\":12.0},"
    "\"production\":{\"cut_options_m\":[0.09,0.10,0.15],\"default_cut_m\":0.09,\"allow_custom\":true},"
    "\"process\":{\"screw\":22,\"target_speed_vfd_rpm\":450,\"target_speed_belt_m_min\":9.0},"
    "\"engineering\":{\"theoretical_density_gr_m\":0.22,\"real_density_gr_m\":0.22}}";

int sd_list_files(char files[][64], int max_files)
    { (void)files; (void)max_files; return 0; }

int sd_list_files_in_dir(const char *dir, char files[][64], int max_files)
{
    if (!dir || strcmp(dir, PROFILE_DIR) != 0)
        return 0;
    int n = (DEMO_COUNT < max_files) ? DEMO_COUNT : max_files;
    for (int i = 0; i < n; i++) {
        snprintf(files[i], 64, "%s.json", DEMO_CODES[i]);
    }
    return n;
}

int sd_read_file(const char *filename, char *buffer, int max_len)
    { (void)filename; (void)buffer; (void)max_len; return -1; }

int sd_read_file_path(const char *path, char *buffer, int max_len)
{
    if (!path || !buffer || max_len <= 0)
        return -1;
    const char *content = NULL;
    char expected[128];
    snprintf(expected, sizeof(expected), "%s/SIM-001.json", PROFILE_DIR);
    if (strcmp(path, expected) == 0) content = DEMO_SIM001;
    snprintf(expected, sizeof(expected), "%s/SIM-002.json", PROFILE_DIR);
    if (strcmp(path, expected) == 0) content = DEMO_SIM002;
    if (!content) return -1;
    int len = (int)strlen(content);
    if (len >= max_len) len = max_len - 1;
    memcpy(buffer, content, len);
    buffer[len] = '\0';
    return len;
}

int sd_read_last_chunk(const char *filename, char *buffer, int max_len)
    { (void)filename; (void)buffer; (void)max_len; return -1; }
