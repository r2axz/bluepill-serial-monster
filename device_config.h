#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include "gpio.h"
#include "cdc_config.h"

typedef struct {
    gpio_pin_t config_pin;
    cdc_config_t cdc_config;
} __attribute__ ((packed)) device_config_t;

void device_config_init();
device_config_t *device_config_get();

#endif /* DEVICE_CONFIG_H_ */
