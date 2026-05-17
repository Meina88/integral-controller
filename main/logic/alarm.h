#pragma once

#include <stdbool.h>

void alarm_init(void);
void alarm_tick(void);

// 3 × 500 ms beeps — speed out of range. Always overrides current beep.
void alarm_trigger_speed(void);

// 1 × 500 ms beep — pre-cut warning. Only fires if alarm is idle.
void alarm_trigger_immediate(void);
