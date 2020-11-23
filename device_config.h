#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdint.h>
#include "gpio.h"
#include "cdc_config.h"

typedef struct {
    uint32_t magic;
    gpio_pin_t config_pin;
    cdc_config_t cdc_config;
    uint32_t crc;
} __attribute__ ((packed)) device_config_t;

void device_config_init();
device_config_t *device_config_get();

void device_config_store();

#endif /* DEVICE_CONFIG_H_ */
