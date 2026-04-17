#include "relay.h"
#include "CH422G.h"

static bool relay_state = false;

void relay_init(void)
{
    // Estado inicial: OFF (línea liberada)
    CH422G_od_output(0x01);
    relay_state = false;
}

void relay_set(bool on)
{
    relay_state = on;

    if (on)
    {
        // 🔥 LOW → activa relay
        CH422G_od_output(0x00);
    }
    else
    {
        // 🔥 flotante → desactiva
        CH422G_od_output(0x01);
    }
}

bool relay_get(void)
{
    return relay_state;
}