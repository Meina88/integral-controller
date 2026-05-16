#pragma once

#include "rtc_pcf85063a.h"

void rtc_hw_init(void);
void rtc_set_default_time(void);
void rtc_get_time_string(char *buffer);
void rtc_get_datetime_string(char *buffer);
void rtc_set_manual_time(void);
void rtc_get_date_filename_string(char *buffer);
void rtc_get_datetime(datetime_t *t);