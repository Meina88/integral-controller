#pragma once
#include <stdint.h>
#include <SDL2/SDL.h>

static inline void esp_rom_delay_us(uint32_t us) { SDL_Delay((us + 999) / 1000); }
