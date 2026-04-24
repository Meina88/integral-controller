#include "profile.h"
#include "storage/nvs/storage_nvs.h"

#include "cJSON.h"
#include <string.h>
#include <stdio.h>

#define MAX_JSON_SIZE 2048

// =========================
// GET BY CODE
// =========================
bool profile_get_by_code(const char *code, profile_t *out)
{
    char json[MAX_JSON_SIZE];

    if (storage_nvs_load_profiles(json, sizeof(json)) != ESP_OK)
        return false;

    cJSON *root = cJSON_Parse(json);
    if (!root) return false;

    cJSON *profiles = cJSON_GetObjectItem(root, "profiles");
    if (!profiles) {
        cJSON_Delete(root);
        return false;
    }

    int count = cJSON_GetArraySize(profiles);

    for (int i = 0; i < count; i++)
    {
        cJSON *p = cJSON_GetArrayItem(profiles, i);

        const char *p_code = cJSON_GetObjectItem(p, "code")->valuestring;

        if (strcmp(p_code, code) == 0)
        {
            strcpy(out->code, p_code);
            strcpy(out->matrix, cJSON_GetObjectItem(p, "matrix")->valuestring);
            out->screw = cJSON_GetObjectItem(p, "screw")->valueint;
            out->vfd_speed = cJSON_GetObjectItem(p, "vfd_speed")->valueint;
            out->extrusion_speed = cJSON_GetObjectItem(p, "extrusion_speed")->valuedouble;
            out->density = cJSON_GetObjectItem(p, "density")->valuedouble;
            out->cut_length = cJSON_GetObjectItem(p, "cut_length")->valuedouble;

            cJSON_Delete(root);
            return true;
        }
    }

    cJSON_Delete(root);
    return false;
}

// =========================
// SEARCH (filtro por texto)
// =========================
int profile_search(const char *filter, char results[][32], int max)
{
    char json[MAX_JSON_SIZE];

    if (storage_nvs_load_profiles(json, sizeof(json)) != ESP_OK)
        return 0;

    cJSON *root = cJSON_Parse(json);
    if (!root) return 0;

    cJSON *profiles = cJSON_GetObjectItem(root, "profiles");
    if (!profiles) {
        cJSON_Delete(root);
        return 0;
    }

    int count = cJSON_GetArraySize(profiles);
    int found = 0;

    for (int i = 0; i < count && found < max; i++)
    {
        cJSON *p = cJSON_GetArrayItem(profiles, i);
        const char *code = cJSON_GetObjectItem(p, "code")->valuestring;

        if (strstr(code, filter) != NULL)
        {
            strncpy(results[found], code, 31);
            results[found][31] = '\0';
            found++;
        }
    }

    cJSON_Delete(root);
    return found;
}

// =========================
// UPDATE PROFILE
// =========================
bool profile_update(const profile_t *new_p)
{
    char json[MAX_JSON_SIZE];

    if (storage_nvs_load_profiles(json, sizeof(json)) != ESP_OK)
        return false;

    cJSON *root = cJSON_Parse(json);
    if (!root) return false;

    cJSON *profiles = cJSON_GetObjectItem(root, "profiles");
    if (!profiles) {
        cJSON_Delete(root);
        return false;
    }

    int count = cJSON_GetArraySize(profiles);
    bool updated = false;

    for (int i = 0; i < count; i++)
    {
        cJSON *p = cJSON_GetArrayItem(profiles, i);
        const char *code = cJSON_GetObjectItem(p, "code")->valuestring;

        if (strcmp(code, new_p->code) == 0)
        {
            cJSON_ReplaceItemInObject(p, "matrix", cJSON_CreateString(new_p->matrix));
            cJSON_ReplaceItemInObject(p, "screw", cJSON_CreateNumber(new_p->screw));
            cJSON_ReplaceItemInObject(p, "vfd_speed", cJSON_CreateNumber(new_p->vfd_speed));
            cJSON_ReplaceItemInObject(p, "extrusion_speed", cJSON_CreateNumber(new_p->extrusion_speed));
            cJSON_ReplaceItemInObject(p, "density", cJSON_CreateNumber(new_p->density));
            cJSON_ReplaceItemInObject(p, "cut_length", cJSON_CreateNumber(new_p->cut_length));

            updated = true;
            break;
        }
    }

    if (!updated)
    {
        // crear nuevo perfil si no existe
        cJSON *new_item = cJSON_CreateObject();

        cJSON_AddStringToObject(new_item, "code", new_p->code);
        cJSON_AddStringToObject(new_item, "matrix", new_p->matrix);
        cJSON_AddNumberToObject(new_item, "screw", new_p->screw);
        cJSON_AddNumberToObject(new_item, "vfd_speed", new_p->vfd_speed);
        cJSON_AddNumberToObject(new_item, "extrusion_speed", new_p->extrusion_speed);
        cJSON_AddNumberToObject(new_item, "density", new_p->density);
        cJSON_AddNumberToObject(new_item, "cut_length", new_p->cut_length);

        cJSON_AddItemToArray(profiles, new_item);
    }

    // convertir a string
    char *new_json = cJSON_PrintUnformatted(root);

    if (!new_json)
    {
        cJSON_Delete(root);
        return false;
    }

    // guardar en NVS
    bool ok = (storage_nvs_save_profiles(new_json) == ESP_OK);

    // liberar
    cJSON_free(new_json);
    cJSON_Delete(root);

    return ok;
}