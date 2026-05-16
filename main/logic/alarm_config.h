#pragma once

#include <stdbool.h>

void alarm_config_init(void);
bool alarm_config_is_enabled(void);
int  alarm_config_get_threshold(void);
void alarm_config_set(bool enabled, int threshold);
