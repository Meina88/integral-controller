#pragma once

#include <stdbool.h>

// ── Speed alarm ───────────────────────────────────────────────────
void alarm_config_init(void);
bool alarm_config_is_enabled(void);
int  alarm_config_get_threshold(void);
void alarm_config_set(bool enabled, int threshold);

// ── Pre-cut alarm ─────────────────────────────────────────────────
bool alarm_config_pre_cut_is_enabled(void);
int  alarm_config_pre_cut_get_seconds(void);
void alarm_config_pre_cut_set(bool enabled, int seconds);
