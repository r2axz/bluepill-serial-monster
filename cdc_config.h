/*
 * MIT License
 *
 * Copyright (c) 2020 Kirill Kotyagin
 */

#ifndef CDC_CONFIG_H
#define CDC_CONFIG_H

#include "gpion.h"
#include "usb_cdc.h"

typedef struct {
    gpion_pin_t pins[cdc_pin_last];
} cdc_port_t;

typedef struct {
    cdc_port_t port_config[USB_CDC_NUM_PORTS];
} cdc_config_t;

#endif /* CDC_CONFIG_H */
