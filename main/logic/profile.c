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

    const cJSON *j_code = cJSON_GetObjectItem(root, "code");
    const cJSON *j_matrix = cJSON_GetObjectItem(root, "matrix");
    const cJSON *j_screw = cJSON_GetObjectItem(root, "screw");
    const cJSON *j_vfd = cJSON_GetObjectItem(root, "vfd_speed");
    const cJSON *j_ext = cJSON_GetObjectItem(root, "extrusion_speed");
    const cJSON *j_density = cJSON_GetObjectItem(root, "density");
    const cJSON *j_length = cJSON_GetObjectItem(root, "cut_length");

    if (!cJSON_IsString(j_code) || !cJSON_IsString(j_matrix))
    {
        cJSON_Delete(root);
        return false;
    }

    strncpy(out->code, j_code->valuestring, sizeof(out->code) - 1);
    out->code[sizeof(out->code) - 1] = '\0';

    strncpy(out->matrix, j_matrix->valuestring, sizeof(out->matrix) - 1);
    out->matrix[sizeof(out->matrix) - 1] = '\0';

    out->screw = j_screw ? j_screw->valueint : 0;
    out->vfd_speed = j_vfd ? j_vfd->valueint : 0;
    out->extrusion_speed = j_ext ? j_ext->valuedouble : 0;
    out->density = j_density ? j_density->valuedouble : 0;
    out->cut_length = j_length ? j_length->valuedouble : 0;

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
    profile_t p;

    if (!profile_get_by_code(source_code, &p))
        return false;

    // cambiar código
    strncpy(p.code, new_code, sizeof(p.code) - 1);
    p.code[sizeof(p.code) - 1] = '\0';

    return profile_update(&p);
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

bool profile_update(const profile_t *p)
{
    char path[128];

    snprintf(path, sizeof(path), "%s/%s.json", PROFILE_DIR, p->code);

    FILE *f = fopen(path, "w");
    if (!f)
        return false;

    fprintf(f,
            "{"
            "\"code\":\"%s\","
            "\"matrix\":\"%s\","
            "\"screw\":%d,"
            "\"vfd_speed\":%d,"
            "\"extrusion_speed\":%.2f,"
            "\"density\":%.2f,"
            "\"cut_length\":%.2f"
            "}",
            p->code,
            p->matrix,
            p->screw,
            p->vfd_speed,
            p->extrusion_speed,
            p->density,
            p->cut_length);

    fclose(f);
    return true;
}