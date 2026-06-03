#pragma once
#include <stdint.h>
#include <SDL2/SDL.h>

typedef uint32_t TickType_t;
typedef void *TaskHandle_t;
typedef void *QueueHandle_t;
typedef void *SemaphoreHandle_t;

#define portTICK_PERIOD_MS    1U
#define portMAX_DELAY         0xFFFFFFFFU
#define pdMS_TO_TICKS(ms)     ((TickType_t)(ms))
#define pdTRUE                1
#define pdFALSE               0
#define pdPASS                1
#define pdFAIL                0
#define configMINIMAL_STACK_SIZE 128
