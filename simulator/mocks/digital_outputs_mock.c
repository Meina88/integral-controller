#include "drivers/digital_outputs.h"
#include <stdio.h>

void digital_outputs_init(void) {}
void relay_1_on(void)  { printf("[SIM] Relay 1 ON\n"); }
void relay_1_off(void) { printf("[SIM] Relay 1 OFF\n"); }
void relay_2_on(void)  { printf("[SIM] Relay 2 ON\n"); }
void relay_2_off(void) { printf("[SIM] Relay 2 OFF\n"); }
