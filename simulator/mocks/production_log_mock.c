#include "services/production_log.h"
#include <stdio.h>

void production_log_init(void) {}

void production_log_append(const char *start_time, const char *end_time,
                            const char *profile, float length_m,
                            float speed_avg, int cuts, const char *finish_reason)
{
    printf("[SIM] Log: profile=%s start=%s end=%s len=%.2fm speed=%.1f cuts=%d reason=%s\n",
           profile ? profile : "?",
           start_time ? start_time : "?",
           end_time ? end_time : "?",
           length_m, speed_avg, cuts,
           finish_reason ? finish_reason : "?");
}
