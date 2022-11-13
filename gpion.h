/*
 * MIT License
 *
 * Copyright (c) 2022 Yury Shvedov
 */

#ifndef GPION_H
#define GPION_H

#include <stdint.h>

#include "gpio.h"
#include "usb_cdc.h"

typedef enum {
    gpio_pin_pa_first = 0,
    gpio_pin_pa0 = gpio_pin_pa_first,
    gpio_pin_pa1,
    gpio_pin_pa2,
    gpio_pin_pa3,
    gpio_pin_pa4,
    gpio_pin_pa5,
    gpio_pin_pa6,
    gpio_pin_pa7,
    gpio_pin_pa8,
    gpio_pin_pa9,
    gpio_pin_pa10,
    gpio_pin_pa11,
    gpio_pin_pa12,
    gpio_pin_pa13,
    gpio_pin_pa14,
    gpio_pin_pa15,
    gpio_pin_pa_last = gpio_pin_pa15,

    gpio_pin_pb_first = gpio_pin_pa_last + 1,
    gpio_pin_pb0 = gpio_pin_pb_first,
    gpio_pin_pb1,
    gpio_pin_pb2,
    gpio_pin_pb3,
    gpio_pin_pb4,
    gpio_pin_pb5,
    gpio_pin_pb6,
    gpio_pin_pb7,
    gpio_pin_pb8,
    gpio_pin_pb9,
    gpio_pin_pb10,
    gpio_pin_pb11,
    gpio_pin_pb12,
    gpio_pin_pb13,
    gpio_pin_pb14,
    gpio_pin_pb15,
    gpio_pin_pb_last = gpio_pin_pb15,

    gpio_pin_pc_first = gpio_pin_pb_last + 1,
    gpio_pin_pc13 = gpio_pin_pc_first,
    gpio_pin_pc14,
    gpio_pin_pc15,
    gpio_pin_pc_last = gpio_pin_pc15,

    gpio_pin_unknown,
    gpio_pin_last = gpio_pin_unknown,
} gpion_pin_t;

typedef struct {
    gpio_pin_t  pins [gpio_pin_last];
} gpio_config_t;

typedef struct {
    int port;
    cdc_pin_t pin;
} __packed cdc_pin_ref_t;

gpio_pin_t *gpion_to_gpio(gpion_pin_t pinn);
gpion_pin_t gpio_to_gpion(const gpio_pin_t *pin);

const char *gpion_to_str(gpion_pin_t pinn);
gpion_pin_t str_to_gpion(const char *str);

void gpion_pin_init(gpion_pin_t pin);

/* This will work only for occupied pins */
void gpion_pin_set(gpion_pin_t pin, int is_active);
int  gpion_pin_get(gpion_pin_t pin);
int  gpion_pin_get_default(gpion_pin_t pin, int def);

volatile uint32_t *gpion_pin_get_bitband_clear_addr(gpion_pin_t pin);

/* Set will work only for free pins, get - for free and occupied */
void gpion_pin_set_free(gpion_pin_t pin, int is_active);
int  gpion_pin_get_free(gpion_pin_t pin, int def);

gpio_status_t gpion_pin_get_status(gpion_pin_t pin);

gpio_hal_t gpion_to_hal(gpion_pin_t pin);

cdc_pin_ref_t gpion_to_cdc(gpion_pin_t pin);
#endif /* GPION_H */
