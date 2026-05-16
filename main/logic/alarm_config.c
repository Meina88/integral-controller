#include "alarm_config.h"
#include "storage/nvs/storage_nvs.h"

static bool s_enabled   = false;
static int  s_threshold = 10;

void alarm_config_init(void)
{
    storage_nvs_load_alarm(&s_enabled, &s_threshold);
    if (s_threshold < 1)  s_threshold = 1;
    if (s_threshold > 99) s_threshold = 99;
}

bool alarm_config_is_enabled(void)    { return s_enabled; }
int  alarm_config_get_threshold(void) { return s_threshold; }

void alarm_config_set(bool enabled, int threshold)
{
    s_enabled   = enabled;
    s_threshold = threshold;
    storage_nvs_save_alarm(enabled, threshold);
}
