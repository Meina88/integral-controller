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

// Relay de marcacion (relay 1)
bool alarm_config_marking_relay_is_enabled(void);
void alarm_config_marking_relay_set(bool enabled);
int  alarm_config_marking_relay_get_duration_ds(void);   // deciseconds (1–50 → 0.1–5.0 s)
void alarm_config_marking_relay_set_duration_ds(int ds);
int  alarm_config_marking_relay_get_duration_ms(void);   // milliseconds, for relay driver

// ── Spray shots counter ───────────────────────────────────────────
int  alarm_config_spray_shots_get_max(void);
void alarm_config_spray_shots_set_max(int max);
int  alarm_config_spray_shots_get_remaining(void);
void alarm_config_spray_shots_decrement(void);
void alarm_config_spray_shots_reset(void);
