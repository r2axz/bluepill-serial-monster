#ifndef CDC_CONFIG_H
#define CDC_CONFIG_H

#include <stm32f1xx.h>
#include "usb_cdc.h"

typedef enum {
    usb_cdc_port_dir_input  = 0x00,
    usb_cdc_port_dir_output = 0x00,
} __attribute__ ((packed)) usb_cdc_port_dir_t;

typedef enum {
     usb_cdc_output_oc  = 0x00,
     usb_cdc_output_pp  = 0x01,
     usb_cdc_output_last
} __attribute__ ((packed)) usb_cdc_output_t;

typedef enum {
     usb_cdc_polarity_high  = 0x00,
     usb_cdc_polarity_low   = 0x01,
     usb_cdc_polarity_last
} __attribute__ ((packed)) usb_cdc_polarity_t;

typedef enum {
    usb_cdc_pull_floating   = 0x00,
    usb_cdc_pull_up         = 0x01,
    usb_cdc_pull_down       = 0x02,
    usb_cdc_pull_last
} __attribute__ ((packed)) usb_cdc_pull_type_t;

typedef struct {
    GPIO_TypeDef *port;
    uint8_t pin;
    usb_cdc_port_dir_t  direction;
    usb_cdc_output_t    output_type;
    usb_cdc_polarity_t  polarity;
    usb_cdc_pull_type_t pull;
} __attribute__ ((packed)) cdc_port_signal_t;

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
    usb_cdc_signal_t signal_configurations[usb_cdc_signal_last];
} __attribute__ ((packed)) usb_cdc_port_config_t;

typedef struct {
    usb_cdc_port_config_t port_configurations[USB_CDC_NUM_PORTS];
} __attribute__ ((packed)) usb_cdc_config_t;

#endif /* CDC_CONFIG_H */
