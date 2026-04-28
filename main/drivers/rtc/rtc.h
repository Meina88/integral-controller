#pragma once

void rtc_hw_init(void);
void rtc_set_default_time(void);
void rtc_get_time_string(char *buffer);
void rtc_get_datetime_string(char *buffer);
void rtc_set_manual_time(void);
void rtc_get_date_filename_string(char *buffer);