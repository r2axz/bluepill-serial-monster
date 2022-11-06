/*
 * MIT License
 *
 * Copyright (c) 2022 Yury Shvedov
 */

#ifndef DEFAULT_CONFIG_H
#define DEFAULT_CONFIG_H

#include "gpion.h"
#include "usb_cdc.h"
#include "device_config.h"

typedef struct {
    gpion_pin_t         pin;
    gpio_dir_t          dir;
    gpio_func_t         func;
    gpio_output_t       output;
    gpio_pull_t         pull;
    gpio_polarity_t     polarity;
    gpio_speed_t        speed;
    gpio_status_t       status;
} default_gpio_pin_t;

typedef struct {
    default_gpio_pin_t   pins[cdc_pin_last];
} default_port_t;

typedef struct {
    default_port_t port_config[USB_CDC_NUM_PORTS];
} default_cdc_t;

typedef struct {
    gpion_pin_t pin;
    const char *reason;
} default_blocked_pin_t;

#define default_blocked_pin_end gpio_pin_unknown

typedef struct {
    default_gpio_pin_t      status_led_pin;
    default_gpio_pin_t      config_pin;
    default_cdc_t           cdc_config;
    default_blocked_pin_t   blocked_pins[];
} default_config_t;

void default_config_load(device_config_t *target);
const char *default_config_get_blocked_reason(gpion_pin_t pin);
void default_config_load_pin(device_config_t *target, gpion_pin_t pinn);
const default_gpio_pin_t *default_config_get_pin(gpion_pin_t);

#endif /* DEFAULT_CONFIG_H */
