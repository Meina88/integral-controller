#pragma once
/* GPIO shim — not used in simulator */
#include <stdint.h>
typedef int gpio_num_t;
typedef void (*gpio_isr_t)(void *);
typedef struct { uint64_t pin_bit_mask; int mode; int pull_up_en; int pull_down_en; int intr_type; } gpio_config_t;
static inline int gpio_config(const gpio_config_t *c) { (void)c; return 0; }
static inline int gpio_set_level(gpio_num_t g, uint32_t l) { (void)g;(void)l; return 0; }
static inline int gpio_get_level(gpio_num_t g) { (void)g; return 0; }
static inline int gpio_install_isr_service(int f) { (void)f; return 0; }
static inline int gpio_isr_handler_add(gpio_num_t g, gpio_isr_t h, void *a) { (void)g;(void)h;(void)a; return 0; }
