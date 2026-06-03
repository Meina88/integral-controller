#pragma once
#include "FreeRTOS.h"
#include <SDL2/SDL.h>

typedef void (*TaskFunction_t)(void *);

static inline void vTaskDelay(TickType_t ticks) { SDL_Delay(ticks * portTICK_PERIOD_MS); }
static inline TickType_t xTaskGetTickCount(void) { return (TickType_t)SDL_GetTicks(); }

static inline int xTaskCreate(TaskFunction_t fn, const char *name, uint32_t stack,
                               void *param, uint32_t prio, TaskHandle_t *handle)
{
    (void)fn; (void)name; (void)stack; (void)param; (void)prio; (void)handle;
    return pdPASS;
}

static inline void vTaskDelete(TaskHandle_t h) { (void)h; }
