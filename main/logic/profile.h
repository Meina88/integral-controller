#pragma once
#include <stdbool.h>

typedef struct {
    char code[20];
    char matrix[10];
    int screw;
    int vfd_speed;
    float extrusion_speed;
    float density;
    float cut_length;
} profile_t;

bool profile_get_by_code(const char *code, profile_t *out);
int profile_search(const char *filter, char results[][32], int max);
bool profile_update(const profile_t *p);