#include "extrusion.h"
#include "drivers/io/CH422G.h"
#include "drivers/digital_outputs.h"
#include "lvgl.h"

// =========================
// ESTADO
// =========================
static bool running = false;

static int pulse_count = 0;
static int total_count = 0;

static bool last_sensor_state = false;

// relay 1
static bool relay_active = false;
static uint32_t relay_start_time = 0;

// relay 2
static bool relay_2_pending = false;
static bool relay_2_active = false;
static uint32_t relay_2_delay_start = 0;
static uint32_t relay_2_on_time = 0;

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
}

bool extrusion_is_running(void)
{
    return running;
}

// =========================
// GETTERS
// =========================
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
    bool current_state = !di;

    // =========================
    // DETECCIÓN DE PULSOS
    // =========================
    if (running && current_state && !last_sensor_state)
    {
        pulse_count++;

        if (pulse_count % 5 == 0)
        {
            total_count++;

            relay_1_on();
            relay_active = true;
            relay_start_time = lv_tick_get();
        }
    }

    last_sensor_state = current_state;

    uint32_t now = lv_tick_get();

    // =========================
    // RELAY 1
    // =========================
    if (relay_active && (now - relay_start_time >= 1000))
    {
        relay_1_off();

        relay_2_pending = true;
        relay_2_delay_start = now;

        relay_active = false;
    }

    // =========================
    // RELAY 2 DELAY
    // =========================
    if (relay_2_pending && (now - relay_2_delay_start >= 500))
    {
        relay_2_on();
        relay_2_on_time = now;

        relay_2_pending = false;
        relay_2_active = true;
    }

    // =========================
    // RELAY 2 DURACIÓN
    // =========================
    if (relay_2_active && (now - relay_2_on_time >= 1000))
    {
        relay_2_off();
        relay_2_active = false;
    }
}