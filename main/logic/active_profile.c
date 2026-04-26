#include "active_profile.h"
#include <string.h>

static char current_profile[32] = "";

const char *active_profile_get(void)
{
    return current_profile;
}

void active_profile_set(const char *code)
{
    if (!code) return;

    strncpy(current_profile, code, sizeof(current_profile) - 1);
    current_profile[sizeof(current_profile) - 1] = '\0';
}