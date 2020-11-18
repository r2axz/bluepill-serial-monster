#ifndef CDC_CONFIG_H
#define CDC_CONFIG_H

#include "gpio.h"
#include "usb_cdc.h"

typedef enum {
    cdc_pin_rx,
    cdc_pin_tx,
    cdc_pin_rts,
    cdc_pin_cts,
    cdc_pin_dsr,
    cdc_pin_dtr,
    cdc_pin_dcd,
    cdc_pin_unknown,
    cdc_pin_last = cdc_pin_unknown,
} __attribute__ ((packed)) cdc_pin_t;

typedef struct {
    gpio_pin_t pins[cdc_pin_last];
} __attribute__ ((packed)) cdc_port_t;

typedef struct {
    cdc_port_t port_config[USB_CDC_NUM_PORTS];
} __attribute__ ((packed)) cdc_config_t;

#endif /* CDC_CONFIG_H */
