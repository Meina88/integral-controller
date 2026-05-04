#pragma once
#include <stdbool.h>

#define MAX_CUT_OPTIONS 5

typedef struct
{
    char id[8];

    // general
    char code[16];
    char commercial_name[32];

    // geometry
    char matrix[16];
    int bocas;
    float area_mm2;

    // production
    float cut_options[MAX_CUT_OPTIONS];
    int cut_options_count;
    float default_cut;
    bool allow_custom;

    // process
    int screw;
    int vfd_rpm;
    float belt_speed;

    // engineering
    float theoretical_density;
    float real_density;

    // files
    char image[32];

} profile_t;


// API
bool profile_get_by_code(const char *code, profile_t *out);
int profile_search(const char *filter, char results[][32], int max);
bool profile_exists(const char *code);
bool profile_delete(const char *code);

// ⚠️ desactivar temporalmente
// bool profile_update(const profile_t *p);
// bool profile_duplicate(...)