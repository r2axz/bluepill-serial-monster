/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdint.h>
#include "gpio.h"
#include "cdc_config.h"

typedef struct {
    uint32_t        magic;
    gpio_pin_t      status_led_pin;
    gpio_pin_t      config_pin;
    cdc_config_t    cdc_config;
    uint32_t        crc; /* should be the last member of the struct */
} __attribute__ ((packed, aligned(4))) device_config_t;

void device_config_init();
device_config_t *device_config_get();
const device_config_t *device_config_get_default();

void device_config_save();
void device_config_reset();

#endif /* DEVICE_CONFIG_H_ */
