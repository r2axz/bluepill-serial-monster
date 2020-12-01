/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include <stm32f1xx.h>
#include "gpio.h"
#include "device_config.h"
#include "status_led.h"

void status_led_init() {
    gpio_pin_init(&device_config_get()->status_led_pin);
}

void status_led_set(int on) {
    (void)status_led_set; /* This function does not have to be used */
    gpio_pin_set(&device_config_get()->status_led_pin, on);
}
