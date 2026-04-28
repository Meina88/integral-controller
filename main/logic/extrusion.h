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
// DATOS EXISTENTES
// =========================
int extrusion_get_pulse_count(void);
int extrusion_get_total_count(void);

// =========================
// NUEVOS DATOS
// =========================
float extrusion_get_total_mm(void);
float extrusion_get_speed_m_min(void);
float extrusion_get_avg_speed(void);

// =========================
// GRABACIÓN
// =========================
void recording_start(void);
void recording_stop(void);


#pragma once

#include <stdbool.h>
#include <stdint.h>

// =========================
// CONTROL GENERAL
// =========================
void extrusion_start(void);
void extrusion_stop(void);
bool extrusion_is_running(void);

// =========================
// PROCESO (loop principal)
// =========================
void extrusion_process_tick(void);

// =========================
// CONTADORES
// =========================
int extrusion_get_pulse_count(void);
int extrusion_get_total_count(void);

// =========================
// MEDICIONES
// =========================
float extrusion_get_total_mm(void);
float extrusion_get_speed_m_min(void);
float extrusion_get_avg_speed(void);

// =========================
// GRABACIÓN DE CICLO
// =========================
void recording_start(void);
void recording_stop(void);

// =========================
// 🔥 OPCIONAL (RECOMENDADO)
// =========================
// Para futura expansión (UI / API / debug)
const char* extrusion_get_start_time(void);
const char* extrusion_get_end_time(void);