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
// PROCESO
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
// DISTANCIA DE CORTE
// =========================
void extrusion_set_cut_distance_m(float meters);
float extrusion_get_cut_distance_m(void);

// =========================
// GRABACIÓN
// =========================
void recording_start(void);
void recording_stop(void);

// =========================
// TIEMPOS
// =========================
const char* extrusion_get_start_time(void);
const char* extrusion_get_end_time(void);

// =========================
// TARGET PRODUCCIÓN
// =========================
void extrusion_set_target_count(int count);
int extrusion_get_target_count(void);
bool extrusion_is_target_reached(void);