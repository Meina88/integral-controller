#include "digital_outputs.h"
#include "CH422G.h"

#define RELAY_1_PIN 0
#define RELAY_2_PIN 1

static uint8_t od_state = 0xFF;

// =========================
// APPLY
// =========================
static void apply_outputs(void)
{
    CH422G_od_output(od_state);
}

// =========================
// SET PIN
// =========================
static void set_pin(uint8_t pin, bool on)
{
    if (on)
        od_state &= ~(1 << pin);  // LOW
    else
        od_state |= (1 << pin);   // FLOAT

    apply_outputs();
}

// =========================
// INIT
// =========================
void digital_outputs_init(void)
{
    od_state = 0xFF;
    apply_outputs();
}

// =========================
// RELAY 1 (DO0)
// =========================
void relay_1_on(void)
{
    set_pin(RELAY_1_PIN, true);
}

void relay_1_off(void)
{
    set_pin(RELAY_1_PIN, false);
}

// =========================
// RELAY 2 (DO1)
// =========================
void relay_2_on(void)
{
    set_pin(RELAY_2_PIN, true);
}

void relay_2_off(void)
{
    set_pin(RELAY_2_PIN, false);
}