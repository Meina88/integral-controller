#pragma once

void production_log_init(void);

void production_log_write_row(
    const char *start_time,
    const char *end_time,
    const char *profile,
    float length_m,
    float speed_avg
);