#ifndef CDC_CONFIG_H
#define CDC_CONFIG_H

#include "gpio.h"
#include "usb_cdc.h"

typedef struct {
    gpio_pin_t pins[cdc_pin_last];
} __attribute__ ((packed)) cdc_port_t;

typedef struct {
    cdc_port_t port_config[USB_CDC_NUM_PORTS];
} __attribute__ ((packed)) cdc_config_t;

#endif /* CDC_CONFIG_H */
