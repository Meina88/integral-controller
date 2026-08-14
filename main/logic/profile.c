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

// Un "code" termina siendo un nombre de archivo en /sdcard/profiles/, así que
// solo se permiten caracteres seguros para eso: sin espacios, "/", "\" ni "..".
bool profile_code_is_valid(const char *code)
{
    if (!code || !code[0])
        return false;

    size_t len = strlen(code);
    if (len > PROFILE_CODE_MAX_LEN)
        return false;

    if (strcmp(code, ".") == 0 || strcmp(code, "..") == 0)
        return false;

    for (size_t i = 0; i < len; i++)
    {
        char c = code[i];
        bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_';
        if (!ok)
            return false;
    }

    return true;
}

bool profile_get_by_code(const char *code, profile_t *out)
{
    if (!profile_code_is_valid(code))
        return false;

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
        strncpy(
            out->image,
            j_img->valuestring,
            sizeof(out->image) - 1
        );

        out->image[sizeof(out->image) - 1] = '\0';
    }
}

    cJSON_Delete(root);
    return true;
}

#define MAX_FILES 120

int profile_search(const char *filter, char results[][32], int max)
{
    char (*files)[64] = malloc(MAX_FILES * sizeof(*files));
    if (!files)
        return 0;

    int count = sd_list_files_in_dir(PROFILE_DIR, files, MAX_FILES);

    if (count <= 0)
        return 0;

    int found = 0;

    for (int i = 0; i < count && found < max; i++)
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

    free(files);
    return found;
}

bool profile_exists(const char *code)
{
    if (!profile_code_is_valid(code))
        return false;

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
    if (!profile_code_is_valid(code))
        return false;

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

// Reemplaza cada caracter fuera de [A-Za-z0-9._-] por '_' y trunca a
// PROFILE_CODE_MAX_LEN, para reparar códigos guardados antes de que
// profile_code_is_valid() existiera (ver bug de perfiles con espacios/"/").
static void sanitize_code(const char *src, char *dst, size_t dst_size)
{
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < dst_size - 1 && j < PROFILE_CODE_MAX_LEN; i++)
    {
        char c = src[i];
        bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_';
        dst[j++] = ok ? c : '_';
    }
    dst[j] = '\0';

    if (dst[0] == '\0' || strcmp(dst, ".") == 0 || strcmp(dst, "..") == 0)
        strncpy(dst, "perfil", dst_size - 1);
}

// Repara en el arranque perfiles guardados en la SD con nombres de archivo
// inválidos (ej: "Manguera%20Riego%201%2F2.json"), renombrándolos a un
// código seguro para que vuelvan a ser editables/borrables desde la UI.
void profile_sanitize_storage(void)
{
    char (*files)[64] = malloc(MAX_FILES * sizeof(*files));
    if (!files)
        return;

    int count = sd_list_files_in_dir(PROFILE_DIR, files, MAX_FILES);

    for (int i = 0; i < count; i++)
    {
        if (!strstr(files[i], ".json"))
            continue;

        char code[32];
        strncpy(code, files[i], sizeof(code) - 1);
        code[sizeof(code) - 1] = '\0';
        remove_json_extension(code);

        if (profile_code_is_valid(code))
            continue;

        char clean[PROFILE_CODE_MAX_LEN + 1];
        sanitize_code(code, clean, sizeof(clean));

        char newcode[PROFILE_CODE_MAX_LEN + 8];
        strncpy(newcode, clean, sizeof(newcode) - 1);
        newcode[sizeof(newcode) - 1] = '\0';

        for (int suffix = 2; profile_exists(newcode) && suffix < 50; suffix++)
            snprintf(newcode, sizeof(newcode), "%.*s_%d", PROFILE_CODE_MAX_LEN - 3, clean, suffix);

        char oldpath[128], newpath[128];
        snprintf(oldpath, sizeof(oldpath), "%s/%s.json", PROFILE_DIR, code);
        snprintf(newpath, sizeof(newpath), "%s/%s.json", PROFILE_DIR, newcode);

        if (rename(oldpath, newpath) != 0)
        {
            printf("No se pudo reparar perfil corrupto '%s'\n", code);
            continue;
        }

        printf("Perfil corrupto reparado: '%s' -> '%s'\n", code, newcode);

        char oldimg[128], newimg[128];
        snprintf(oldimg, sizeof(oldimg), "%s/%s.png", PROFILE_DIR, code);
        snprintf(newimg, sizeof(newimg), "%s/%s.png", PROFILE_DIR, newcode);
        rename(oldimg, newimg);
    }

    free(files);
}
