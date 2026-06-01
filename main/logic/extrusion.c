#include "extrusion.h"
#include "drivers/io/CH422G.h"
#include "drivers/digital_outputs.h"
#include "drivers/rtc/rtc.h"
#include "logic/alarm.h"
#include "logic/alarm_config.h"
#include "logic/calibration.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

#include "services/production_log.h"
#include "logic/active_profile.h"

// =========================
// PARÁMETROS SISTEMA
// =========================

// Diámetro físico del disco sensado.
// No participa del cálculo si el disco gira solidario 1:1 con la polea de cinta.
#define SENSOR_DIAMETER_MM 200.0f
#define BELT_DIAMETER_MM 300.0f
#define HOLES_COUNT 22.0f
#define SPEED_TIMEOUT_MS 1000
#define MM_PER_PULSE ((3.14159265f * BELT_DIAMETER_MM) / HOLES_COUNT)

// =========================
// ESTADO
// =========================
static bool running = false;
static bool recording = false;

static int pulse_count = 0;
static int total_count = 0;

static float total_mm = 0.0f;
static float speed_m_min = 0.0f;

// promedio velocidad
static float speed_sum = 0.0f;
static int speed_samples = 0;

static bool     last_sensor_state    = false;
static uint32_t last_pulse_time      = 0;
static uint32_t last_pulse_interval  = 0; // tiempo entre los dos últimos pulsos

// tiempos
static char start_time_str[32];
static char end_time_str[32];

// relay 1
static bool relay_active = false;
static bool relay_fired_since_check = false;
static uint32_t relay_start_time = 0;

static bool pre_cut_alarm_armed = false;

static float last_cut_mm = 0.0f;
static float cut_distance_mm = 0.0f;

static int target_count = 0;
static bool target_reached = false;

// =========================
// PROMEDIO
// =========================
float extrusion_get_avg_speed(void)
{
    if (speed_samples == 0)
        return 0.0f;

    return speed_sum / speed_samples;
}

// =========================
// CONTROL
// =========================
void extrusion_start(void)
{
    running = true;
}

void extrusion_stop(void)
{
    running = false;
    speed_m_min = 0.0f;
}

// =========================
// GRABACIÓN
// =========================
void recording_start(void)
{
    recording = true;

    total_mm = 0;
    total_count = 0;
    pulse_count = 0;

    last_cut_mm = 0;

    target_reached = false;
    pre_cut_alarm_armed = false;

    speed_sum = 0;
    speed_samples = 0;

    rtc_get_datetime_string(start_time_str);

    // =========================
    // MARCA INICIAL
    // =========================
    if (alarm_config_pre_cut_is_enabled())
        alarm_trigger_immediate();

    if (alarm_config_marking_relay_is_enabled())
    {
        relay_1_on();
        relay_active = true;
        relay_fired_since_check = true;
        relay_start_time = lv_tick_get();
        alarm_config_spray_shots_decrement();
    }
}

// =========================
void recording_stop(void)
{
    recording = false;
    pre_cut_alarm_armed = false;
    rtc_get_datetime_string(end_time_str);
}

// =========================
bool extrusion_is_running(void)
{
    return running;
}

// =========================
// GETTERS
// =========================
float extrusion_get_total_mm(void)
{
    return total_mm;
}

float extrusion_get_speed_m_min(void)
{
    return speed_m_min;
}

int extrusion_get_pulse_count(void)
{
    return pulse_count;
}

int extrusion_get_total_count(void)
{
    return total_count;
}

// =========================
// DISTANCIA DE CORTE
// =========================
void extrusion_set_cut_distance_m(float meters)
{
    if (meters <= 0.0f)
    {
        cut_distance_mm = 0.0f;
        return;
    }

    cut_distance_mm = meters * 1000.0f;

    printf("Distancia de corte configurada: %.2f m\n", meters);
}

float extrusion_get_cut_distance_m(void)
{
    return cut_distance_mm / 1000.0f;
}

// =========================
// TARGET PRODUCCIÓN
// =========================
void extrusion_set_target_count(int count)
{
    if (count < 0)
        count = 0;

    target_count = count;

    printf("Cantidad objetivo configurada: %d\n", target_count);
}

int extrusion_get_target_count(void)
{
    return target_count;
}

bool extrusion_is_target_reached(void)
{
    return target_reached;
}

// =========================
// PROCESO PRINCIPAL
// =========================
void extrusion_process_tick(void)
{
    uint8_t di = CH422G_io_input(0x01);
    bool sensor = !(di & 0x01); // true = detecta metal

    uint32_t now = lv_tick_get();

    // =========================
    // DETECCIÓN DE PULSO
    // =========================
    if (!sensor && last_sensor_state)
    {
        if ((now - last_pulse_time) > 10)
        {
            pulse_count++;

            // calcular velocidad siempre (no solo al grabar)
            if (last_pulse_time > 0)
            {
                uint32_t interval = now - last_pulse_time;
                last_pulse_interval = interval;

                float delta_t_sec = interval / 1000.0f;
                if (delta_t_sec > 0.001f)
                {
                    float speed_mm_s = (MM_PER_PULSE * calibration_get_factor()) / delta_t_sec;
                    speed_m_min = (speed_mm_s / 1000.0f) * 60.0f;
                }
            }

            last_pulse_time = now;

            // =========================
            // SOLO SI GRABANDO
            // =========================
            if (recording)
            {
                total_mm += MM_PER_PULSE * calibration_get_factor();

                // CORTE
                if (cut_distance_mm > 0.0f &&
                    (total_mm - last_cut_mm) >= cut_distance_mm)
                {
                    total_count++;

                    if (alarm_config_marking_relay_is_enabled())
                    {
                        relay_1_on();
                        relay_active = true;
                        relay_fired_since_check = true;
                        relay_start_time = now;
                        alarm_config_spray_shots_decrement();
                    }

                    // =========================
                    // TARGET ALCANZADO
                    // =========================
                    if (target_count > 0 &&
                        total_count >= target_count)
                    {
                        target_reached = true;

                        printf("TARGET DE PRODUCCION ALCANZADO\n");
                    }

                    last_cut_mm = total_mm;
                    pre_cut_alarm_armed = false;
                }

                // PROMEDIO
                speed_sum += speed_m_min;
                speed_samples++;
            }
        }
    }

    // guardar estado correcto
    last_sensor_state = sensor;

    // =========================
    // RELAY 1
    // =========================
    if (relay_active && (now - relay_start_time >= 500))
    {
        relay_1_off();
        relay_active = false;
    }

    // =========================
    // TIMEOUT VELOCIDAD = 0
    // =========================
    // Sin condición 'running': la velocidad siempre debe volver a cero
    // si no llegan pulsos. El timeout es adaptativo: espera al menos
    // 2× el intervalo del último pulso para no zerear en velocidades bajas.
    if (last_pulse_time > 0)
    {
        uint32_t adaptive_timeout = (last_pulse_interval > 0)
            ? LV_MAX(SPEED_TIMEOUT_MS, last_pulse_interval * 2)
            : SPEED_TIMEOUT_MS;

        if ((now - last_pulse_time) > adaptive_timeout)
        {
            speed_m_min = 0.0f;
        }
    }

    // =========================
    // ALARMA PRE-CORTE
    // =========================
    if (recording && cut_distance_mm > 0.0f && speed_m_min > 0.0f &&
        alarm_config_pre_cut_is_enabled() && !pre_cut_alarm_armed)
    {
        float remaining_mm = cut_distance_mm - (total_mm - last_cut_mm);
        float advance_mm   = (float)alarm_config_pre_cut_get_seconds()
                             * (speed_m_min * 1000.0f / 60.0f);

        if (remaining_mm > 0.0f && remaining_mm <= advance_mm)
        {
            alarm_trigger_immediate();
            pre_cut_alarm_armed = true;
        }
    }

    // =========================
    // BUZZER TICK
    // =========================
    alarm_tick();
}

bool extrusion_get_sensor_state(void)
{
    return last_sensor_state;
}

bool extrusion_get_relay_fired(void)
{
    bool v = relay_fired_since_check;
    relay_fired_since_check = false;
    return v;
}

const char *extrusion_get_start_time(void)
{
    return start_time_str;
}

const char *extrusion_get_end_time(void)
{
    return end_time_str;
}
