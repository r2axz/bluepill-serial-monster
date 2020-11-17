#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stm32f1xx.h>

typedef enum {
    gpio_dir_input,
    gpio_dir_output,
    gpio_dir_unknown,
    gpio_dir_last = gpio_dir_unknown
} __attribute__ ((packed)) gpio_dir_t;

typedef enum {
    gpio_func_general,
    gpio_func_alternate,
    gpio_func_unknown,
    gpio_func_last = gpio_func_unknown
} __attribute__ ((packed)) gpio_func_t;

typedef enum {
    gpio_output_pp,
    gpio_output_od,
    gpio_output_unknown,
    gpio_output_last = gpio_output_unknown
} __attribute__ ((packed)) gpio_output_t;

typedef enum {
    gpio_pull_floating,
    gpio_pull_up,
    gpio_pull_down,
    gpio_pull_unknown,
    gpio_pull_last = gpio_pull_unknown
} __attribute__ ((packed)) gpio_pull_t;

typedef enum {
     gpio_polarity_high,
     gpio_polarity_low,
     gpio_polarity_unknown,
     gpio_polarity_last = gpio_polarity_unknown,
} __attribute__ ((packed)) gpio_polarity_t;

typedef enum {
     gpio_speed_low,
     gpio_speed_medium,
     gpio_speed_high,
     gpio_speed_unknown,
     gpio_speed_last = gpio_speed_unknown
} __attribute__ ((packed)) gpio_speed_t;

typedef struct {
    GPIO_TypeDef*       port;
    uint8_t             pin;
    gpio_dir_t          dir;
    gpio_func_t         func;
    gpio_output_t       output;
    gpio_pull_t         pull;
    gpio_polarity_t     polarity;
    gpio_speed_t        speed;
} __attribute__ ((packed)) gpio_pin_t;

void gpio_pin_init(const gpio_pin_t *pin);

void gpio_pin_set(const gpio_pin_t *pin, int is_active);
int  gpio_pin_get(const gpio_pin_t *pin);

#endif /* GPIO_G */
