#pragma once

#include <stdbool.h>

typedef enum
{
    PRODUCTION_FINISH_MANUAL = 0,
    PRODUCTION_FINISH_TARGET = 1
} production_finish_reason_t;

void production_start(void);

void production_finish(production_finish_reason_t reason);

bool production_is_running(void);