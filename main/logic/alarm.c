#include "alarm.h"
#include "drivers/digital_outputs.h"
#include "lvgl.h"

#define BEEP_ON_MS  500
#define BEEP_OFF_MS 300

typedef enum {
    ALARM_IDLE = 0,
    ALARM_ON,
    ALARM_OFF,
} alarm_state_t;

static alarm_state_t state        = ALARM_IDLE;
static uint8_t       beeps_total  = 0;
static uint8_t       beeps_done   = 0;
static uint32_t      phase_start  = 0;

// Flags set by callers, consumed by alarm_tick().
// volatile: written from main-loop task, read from extrusion task.
static volatile bool s_pending_speed     = false;
static volatile bool s_pending_immediate = false;

// =========================
// INIT
// =========================
void alarm_init(void)
{
    relay_2_off();
}

// =========================
// TRIGGER SPEED (3 beeps, overrides)
// =========================
void alarm_trigger_speed(void)
{
    s_pending_speed = true;
}

// =========================
// TRIGGER IMMEDIATE (1 beep, only if idle)
// =========================
void alarm_trigger_immediate(void)
{
    s_pending_immediate = true;
}

// =========================
// START BEEP SEQUENCE
// =========================
static void start_sequence(uint8_t count)
{
    beeps_total = count;
    beeps_done  = 0;
    phase_start = lv_tick_get();
    state       = ALARM_ON;
    relay_2_on();
}

// =========================
// TICK (called from extrusion_task every 5 ms)
// =========================
void alarm_tick(void)
{
    uint32_t now = lv_tick_get();

    // Speed alarm: highest priority, always overrides.
    if (s_pending_speed)
    {
        s_pending_speed     = false;
        s_pending_immediate = false;
        relay_2_on();
        start_sequence(3);
        return;
    }

    // Immediate beep: only when the sequencer is idle.
    if (s_pending_immediate)
    {
        s_pending_immediate = false;
        if (state == ALARM_IDLE)
        {
            start_sequence(1);
            return;
        }
    }

    // Run the state machine.
    switch (state)
    {
    case ALARM_IDLE:
        break;

    case ALARM_ON:
        if ((now - phase_start) >= BEEP_ON_MS)
        {
            relay_2_off();
            beeps_done++;
            if (beeps_done >= beeps_total)
            {
                state = ALARM_IDLE;
            }
            else
            {
                phase_start = now;
                state       = ALARM_OFF;
            }
        }
        break;

    case ALARM_OFF:
        if ((now - phase_start) >= BEEP_OFF_MS)
        {
            relay_2_on();
            phase_start = now;
            state       = ALARM_ON;
        }
        break;
    }
}
