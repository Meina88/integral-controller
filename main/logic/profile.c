#include "profile.h"
#include "storage/sdcard/sd_files.h"

#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "logic/active_profile.h"

#define PROFILE_DIR "/sdcard/profiles"
#define PROFILE_JSON_SIZE 1024

static void remove_json_extension(char *name)
{
    char *dot = strstr(name, ".json");
    if (dot)
        *dot = '\0';
}

bool profile_get_by_code(const char *code, profile_t *out)
{
    char path[128];
    char json[PROFILE_JSON_SIZE];

    snprintf(path, sizeof(path), "%s/%s.json", PROFILE_DIR, code);

    if (sd_read_file_path(path, json, sizeof(json)) <= 0)
        return false;

    cJSON *root = cJSON_Parse(json);
    if (!root)
        return false;

    memset(out, 0, sizeof(profile_t));

    // =========================
    // ID
    // =========================
    strncpy(out->id, code, sizeof(out->id) - 1);
    out->id[sizeof(out->id) - 1] = '\0';

    // =========================
    // GENERAL
    // =========================
    cJSON *general = cJSON_GetObjectItem(root, "general");
    if (general)
    {
        cJSON *j_code = cJSON_GetObjectItem(general, "code");
        cJSON *j_name = cJSON_GetObjectItem(general, "commercial_name");

        if (cJSON_IsString(j_code))
        {
            strncpy(out->code, j_code->valuestring, sizeof(out->code) - 1);
            out->code[sizeof(out->code) - 1] = '\0';
        }

        if (cJSON_IsString(j_name))
        {
            strncpy(out->commercial_name, j_name->valuestring, sizeof(out->commercial_name) - 1);
            out->commercial_name[sizeof(out->commercial_name) - 1] = '\0';
        }
    }

    // =========================
    // GEOMETRY
    // =========================
    cJSON *geometry = cJSON_GetObjectItem(root, "geometry");
    if (geometry)
    {
        cJSON *j_matrix = cJSON_GetObjectItem(geometry, "matrix");
        cJSON *j_bocas = cJSON_GetObjectItem(geometry, "bocas");
        cJSON *j_area = cJSON_GetObjectItem(geometry, "area_mm2");

        if (cJSON_IsString(j_matrix))
        {
            strncpy(out->matrix, j_matrix->valuestring, sizeof(out->matrix) - 1);
            out->matrix[sizeof(out->matrix) - 1] = '\0';
        }

        if (cJSON_IsNumber(j_bocas))
            out->bocas = j_bocas->valueint;

        if (cJSON_IsNumber(j_area))
            out->area_mm2 = j_area->valuedouble;
    }

    // =========================
    // PRODUCTION
    // =========================
    cJSON *prod = cJSON_GetObjectItem(root, "production");
    if (prod)
    {
        cJSON *opts = cJSON_GetObjectItem(prod, "cut_options_m");

        if (cJSON_IsArray(opts))
        {
            int n = cJSON_GetArraySize(opts);
            out->cut_options_count = (n < MAX_CUT_OPTIONS) ? n : MAX_CUT_OPTIONS;

            for (int i = 0; i < out->cut_options_count; i++)
            {
                cJSON *item = cJSON_GetArrayItem(opts, i);
                if (cJSON_IsNumber(item))
                    out->cut_options[i] = item->valuedouble;
            }
        }

        cJSON *j_def = cJSON_GetObjectItem(prod, "default_cut_m");
        if (cJSON_IsNumber(j_def))
            out->default_cut = j_def->valuedouble;

        cJSON *j_custom = cJSON_GetObjectItem(prod, "allow_custom");
        if (cJSON_IsBool(j_custom))
            out->allow_custom = cJSON_IsTrue(j_custom);
    }

    // =========================
    // PROCESS
    // =========================
    cJSON *process = cJSON_GetObjectItem(root, "process");
    if (process)
    {
        cJSON *j_screw = cJSON_GetObjectItem(process, "screw");
        cJSON *j_vfd = cJSON_GetObjectItem(process, "target_speed_vfd_rpm");
        cJSON *j_belt = cJSON_GetObjectItem(process, "target_speed_belt_m_min");

        if (cJSON_IsNumber(j_screw))
            out->screw = j_screw->valueint;

        if (cJSON_IsNumber(j_vfd))
            out->vfd_rpm = j_vfd->valueint;

        if (cJSON_IsNumber(j_belt))
            out->belt_speed = j_belt->valuedouble;
    }

    // =========================
    // ENGINEERING
    // =========================
    cJSON *eng = cJSON_GetObjectItem(root, "engineering");
    if (eng)
    {
        cJSON *j_t = cJSON_GetObjectItem(eng, "theoretical_density_gr_m");
        cJSON *j_r = cJSON_GetObjectItem(eng, "real_density_gr_m");

        if (cJSON_IsNumber(j_t))
            out->theoretical_density = j_t->valuedouble;

        if (cJSON_IsNumber(j_r))
            out->real_density = j_r->valuedouble;
    }

    // =========================
    // FILES
    // =========================
    cJSON *files = cJSON_GetObjectItem(root, "files");
    if (files)
    {
        cJSON *j_img = cJSON_GetObjectItem(files, "image");

        if (cJSON_IsString(j_img))
        {
            strncpy(out->image, j_img->valuestring, sizeof(out->image) - 1);
            out->image[sizeof(out->image) - 1] = '\0';
        }
    }

    cJSON_Delete(root);
    return true;
}

#define MAX_FILES 30
#define MAX_RESULTS 10

int profile_search(const char *filter, char results[][32], int max)
{
    char files[MAX_FILES][64];

    int count = sd_list_files_in_dir(PROFILE_DIR, files, MAX_FILES);

    if (count <= 0)
        return 0;

    int found = 0;

    for (int i = 0; i < count && found < max && found < MAX_RESULTS; i++)
    {
        if (!strstr(files[i], ".json"))
            continue;

        char code[32];
        strncpy(code, files[i], sizeof(code) - 1);
        code[sizeof(code) - 1] = '\0';

        remove_json_extension(code);

        if (filter == NULL || strlen(filter) == 0)
        {
            strncpy(results[found], code, 31);
            results[found][31] = '\0';
            found++;
            continue;
        }

        if (strstr(code, filter))
        {
            strncpy(results[found], code, 31);
            results[found][31] = '\0';
            found++;
        }
    }

    return found;
}

bool profile_exists(const char *code)
{
    char path[128];
    snprintf(path, sizeof(path), "%s/%s.json", PROFILE_DIR, code);

    FILE *f = fopen(path, "r");
    if (f)
    {
        fclose(f);
        return true;
    }
    return false;
}

bool profile_duplicate(const char *source_code, const char *new_code)
{
    printf("Duplicado deshabilitado en nueva versión\n");
    return false;
}

#include "logic/active_profile.h"

bool profile_delete(const char *code)
{
    const char *active = active_profile_get();

    if (active && strcmp(active, code) == 0)
    {
        printf("ERROR: no se puede eliminar perfil activo\n");
        return false;
    }

    char path[128];
    snprintf(path, sizeof(path), "%s/%s.json", PROFILE_DIR, code);

    if (remove(path) == 0)
    {
        printf("Perfil eliminado OK\n");
        return true;
    }

    return false;
}
