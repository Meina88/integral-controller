#include "extrusion.h"
#include "drivers/io/CH422G.h"
#include "drivers/digital_outputs.h"
#include "drivers/rtc/rtc.h"
#include "lvgl.h"
#include <stdio.h>

// =========================
// PARÁMETROS SISTEMA
// =========================
#define SENSOR_DIAMETER_MM 200.0f
#define BELT_DIAMETER_MM 300.0f
#define HOLES_COUNT 22.0f
#define CUT_DISTANCE_M 1.0f
#define CUT_DISTANCE_MM (CUT_DISTANCE_M * 1000.0f)
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

static bool last_sensor_state = false;
static uint32_t last_pulse_time = 0;

// tiempos (🔥 FIX tamaño)
static char start_time_str[32];
static char end_time_str[32];

// relay 1
static bool relay_active = false;
static uint32_t relay_start_time = 0;

// relay 2 (no usado aún)
static bool relay_2_pending = false;
static bool relay_2_active = false;
static uint32_t relay_2_delay_start = 0;
static uint32_t relay_2_on_time = 0;

static float last_cut_mm = 0.0f;

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

    relay_1_off();
    relay_2_off();

    relay_active = false;
    relay_2_pending = false;
    relay_2_active = false;

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

    speed_sum = 0;
    speed_samples = 0;

    rtc_get_datetime_string(start_time_str);
}

// =========================
void recording_stop(void)
{
    recording = false;

    rtc_get_datetime_string(end_time_str);

    float avg_speed = 0.0f;
    if (speed_samples > 0)
        avg_speed = speed_sum / speed_samples;

    FILE *f = fopen("/sdcard/production.log", "a");

    if (f)
    {
        printf("LOG OK\n");

        fprintf(f,
            "{\"start\":\"%s\",\"end\":\"%s\",\"cuts\":%d,\"meters\":%.2f,\"avg_speed\":%.2f}\n",
            start_time_str,
            end_time_str,
            total_count,
            total_mm / 1000.0f,
            avg_speed
        );

        fflush(f);   // 🔥 asegura escritura inmediata
        fclose(f);
    }
    else
    {
        printf("ERROR fopen production.log\n");
    }
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
// PROCESO PRINCIPAL
// =========================
void extrusion_process_tick(void)
{
    uint8_t di = CH422G_io_input(0x01);
    bool current_state = !(di & 0x01); // activo LOW

    uint32_t now = lv_tick_get();

    // =========================
    // DETECCIÓN DE PULSOS
    // =========================
    if (running && current_state && !last_sensor_state)
    {
        pulse_count++;

        // distancia
        total_mm += MM_PER_PULSE;

        // velocidad instantánea
        if (last_pulse_time > 0)
        {
            float delta_t_sec = (now - last_pulse_time) / 1000.0f;

            if (delta_t_sec > 0.001f)
            {
                float speed_mm_s = MM_PER_PULSE / delta_t_sec;
                speed_m_min = (speed_mm_s / 1000.0f) * 60.0f;
            }
        }

        last_pulse_time = now;

        // promedio velocidad (🔥 ahora incluye pausas)
        if (recording)
        {
            speed_sum += speed_m_min;
            speed_samples++;
        }

        // corte por distancia
        if ((total_mm - last_cut_mm) >= CUT_DISTANCE_MM)
        {
            total_count++;

            relay_1_on();
            relay_active = true;
            relay_start_time = now;

            last_cut_mm = total_mm;
        }
    }

    last_sensor_state = current_state;

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
    if (running && last_pulse_time > 0)
    {
        if ((now - last_pulse_time) > SPEED_TIMEOUT_MS)
        {
            speed_m_min = 0.0f;
        }
    }
}