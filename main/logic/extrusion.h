#pragma once

#include <stdbool.h>
#include <stdint.h>

// =========================
// CONTROL
// =========================
void extrusion_start(void);
void extrusion_stop(void);
bool extrusion_is_running(void);

// =========================
// PROCESO
// =========================
void extrusion_process_tick(void);

// =========================
// DATOS
// =========================
int extrusion_get_pulse_count(void);
int extrusion_get_total_count(void);