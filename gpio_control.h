/*
 * MIT License
 *
 * Copyright (c) 2021 Kiril Zyapkov
 */
#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

#include <stdint.h>
#include <stm32f1xx.h>
#include "gpio.h"
#include "usb_cdc.h"

#define GPIO_CONTROL_PINS_PER_PORT 16
#define GPIO_CONGROL_NUM_PORTS 2

typedef struct {
    int             uart_port; // -1 if no uart port association
    cdc_pin_t       uart_pin;
    gpio_dir_t      dir; // unusable pins are marked with dir_unknown
    gpio_pull_t     pull;
    gpio_output_t   output;
    uint8_t         val;
} __attribute__ ((packed)) gpio_control_pin_t;

typedef struct {
    gpio_control_pin_t pins[GPIO_CONTROL_PINS_PER_PORT];
} __attribute__ ((packed)) gpio_control_port_t;

typedef struct {
    gpio_control_port_t ports[2];
} __attribute__ ((packed)) gpio_control_config_t;


void gpio_control_reconfigure_pin(int portnum, int pinnum);
gpio_pin_t *gpio_control_find_uart_pincfg(gpio_control_pin_t *gc_pin);

cdc_use_t gpio_control_pin_get_use(int portnum, int pinnum);

int gpio_control_read(int portnum, int pinnum);
int gpio_control_write(int portnum, int pinnum, int val);

#endif
