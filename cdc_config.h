#ifndef CDC_CONFIG_H
#define CDC_CONFIG_H

#include "gpio.h"
#include "usb_cdc.h"

typedef enum {
    usb_cdc_signal_rx   = 0x00,
    usb_cdc_signal_tx   = 0x01,
    usb_cdc_signal_rts  = 0x02,
    usb_cdc_signal_cts  = 0x03,
    usb_cdc_signal_dsr  = 0x04,
    usb_cdc_signal_dtr  = 0x05,
    usb_cdc_signal_dcd  = 0x06,
    usb_cdc_signal_last,
} __attribute__ ((packed)) usb_cdc_signal_t;

typedef struct {
    gpio_pin_t pins[usb_cdc_signal_last];
} __attribute__ ((packed)) cdc_port_t;

typedef struct {
    cdc_port_t port_configurations[USB_CDC_NUM_PORTS];
} __attribute__ ((packed)) usb_cdc_config_t;

#endif /* CDC_CONFIG_H */
