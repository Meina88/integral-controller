#pragma once
/* I2C driver shim — not used in simulator, exists only so DEV_Config.h/CH422G.h compile */
#include "esp_err.h"
#include <stdint.h>
#define I2C_MASTER_WRITE 0
#define I2C_MASTER_READ  1
typedef int i2c_port_t;
