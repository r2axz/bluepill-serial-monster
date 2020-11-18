#ifndef DEV_CONFIG_H
#define DEV_CONFIG_H

#include "cdc_config.h"

typedef struct {
    cdc_config_t cdc_config;
} __attribute__ ((packed)) device_config_t;

const device_config_t *device_config_get();

#endif /* DEV_CONFIG_H_ */
