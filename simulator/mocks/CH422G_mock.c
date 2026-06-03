/*
 * Mock del IO expander CH422G.
 * CH422G_io_input() siempre retorna 0 (sensor inactivo).
 * Las escrituras son no-ops.
 */
#include "esp_err.h"
#include <stdint.h>

esp_err_t CH422G_io_output(uint8_t pin) { (void)pin; return ESP_OK; }
uint8_t   CH422G_io_input(uint8_t pin)  { (void)pin; return 0; }
esp_err_t CH422G_od_output(uint8_t pin) { (void)pin; return ESP_OK; }
