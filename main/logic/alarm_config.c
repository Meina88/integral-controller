#include "alarm_config.h"
#include "storage/nvs/storage_nvs.h"

static bool s_enabled   = false;
static int  s_threshold = 10;

static bool s_pre_cut_enabled = false;
static int  s_pre_cut_seconds = 3;
static bool s_marking_relay_enabled = true;
static int  s_marking_relay_duration_ds = 5; // 0.5 s default

static int s_spray_shots_max       = 500;
static int s_spray_shots_remaining = 500;

void alarm_config_init(void)
{
    storage_nvs_load_alarm(&s_enabled, &s_threshold);
    if (s_threshold < 1)  s_threshold = 1;
    if (s_threshold > 99) s_threshold = 99;

    storage_nvs_load_pre_cut_alarm(&s_pre_cut_enabled, &s_pre_cut_seconds);
    if (s_pre_cut_seconds < 1)  s_pre_cut_seconds = 1;
    if (s_pre_cut_seconds > 30) s_pre_cut_seconds = 30;

    storage_nvs_load_marking_relay_enabled(&s_marking_relay_enabled);
    storage_nvs_load_marking_relay_duration_ds(&s_marking_relay_duration_ds);
    if (s_marking_relay_duration_ds < 1)  s_marking_relay_duration_ds = 1;
    if (s_marking_relay_duration_ds > 50) s_marking_relay_duration_ds = 50;

    storage_nvs_load_spray_shots_max(&s_spray_shots_max);
    if (s_spray_shots_max < 1)    s_spray_shots_max = 1;
    if (s_spray_shots_max > 9999) s_spray_shots_max = 9999;

    storage_nvs_load_spray_shots_remaining(&s_spray_shots_remaining);
    if (s_spray_shots_remaining < 0)
        s_spray_shots_remaining = 0;
    if (s_spray_shots_remaining > s_spray_shots_max)
        s_spray_shots_remaining = s_spray_shots_max;
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

int alarm_config_marking_relay_get_duration_ds(void)
{
    return s_marking_relay_duration_ds;
}

void alarm_config_marking_relay_set_duration_ds(int ds)
{
    if (ds < 1)  ds = 1;
    if (ds > 50) ds = 50;
    s_marking_relay_duration_ds = ds;
    storage_nvs_save_marking_relay_duration_ds(ds);
}

int alarm_config_marking_relay_get_duration_ms(void)
{
    return s_marking_relay_duration_ds * 100;
}

int alarm_config_spray_shots_get_max(void)
{
    return s_spray_shots_max;
}

void alarm_config_spray_shots_set_max(int max)
{
    if (max < 1)    max = 1;
    if (max > 9999) max = 9999;
    s_spray_shots_max = max;
    if (s_spray_shots_remaining > s_spray_shots_max) {
        s_spray_shots_remaining = s_spray_shots_max;
        storage_nvs_save_spray_shots_remaining(s_spray_shots_remaining);
    }
    storage_nvs_save_spray_shots_max(max);
}

int alarm_config_spray_shots_get_remaining(void)
{
    return s_spray_shots_remaining;
}

void alarm_config_spray_shots_decrement(void)
{
    if (s_spray_shots_remaining > 0) {
        s_spray_shots_remaining--;
        storage_nvs_save_spray_shots_remaining(s_spray_shots_remaining);
    }
}

void alarm_config_spray_shots_reset(void)
{
    s_spray_shots_remaining = s_spray_shots_max;
    storage_nvs_save_spray_shots_remaining(s_spray_shots_remaining);
}
