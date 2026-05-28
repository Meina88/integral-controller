#include "alarm_config.h"
#include "storage/nvs/storage_nvs.h"

static bool s_enabled   = false;
static int  s_threshold = 10;

static bool s_pre_cut_enabled = false;
static int  s_pre_cut_seconds = 3;
static bool s_marking_relay_enabled = true;

void alarm_config_init(void)
{
    storage_nvs_load_alarm(&s_enabled, &s_threshold);
    if (s_threshold < 1)  s_threshold = 1;
    if (s_threshold > 99) s_threshold = 99;

    storage_nvs_load_pre_cut_alarm(&s_pre_cut_enabled, &s_pre_cut_seconds);
    if (s_pre_cut_seconds < 1)  s_pre_cut_seconds = 1;
    if (s_pre_cut_seconds > 30) s_pre_cut_seconds = 30;

    storage_nvs_load_marking_relay_enabled(&s_marking_relay_enabled);
}

bool alarm_config_is_enabled(void)    { return s_enabled; }
int  alarm_config_get_threshold(void) { return s_threshold; }

void alarm_config_set(bool enabled, int threshold)
{
    s_enabled   = enabled;
    s_threshold = threshold;
    storage_nvs_save_alarm(enabled, threshold);
}

bool alarm_config_pre_cut_is_enabled(void)   { return s_pre_cut_enabled; }
int  alarm_config_pre_cut_get_seconds(void)  { return s_pre_cut_seconds; }

void alarm_config_pre_cut_set(bool enabled, int seconds)
{
    s_pre_cut_enabled = enabled;
    s_pre_cut_seconds = seconds;
    storage_nvs_save_pre_cut_alarm(enabled, seconds);
}

bool alarm_config_marking_relay_is_enabled(void)
{
    return s_marking_relay_enabled;
}

void alarm_config_marking_relay_set(bool enabled)
{
    s_marking_relay_enabled = enabled;
    storage_nvs_save_marking_relay_enabled(enabled);
}
