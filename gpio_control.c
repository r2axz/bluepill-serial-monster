/*
 * MIT License
 *
 * Copyright (c) 2021 Kiril Zyapkov
 */
#include "gpio_control.h"
#include "device_config.h"


static inline int8_t _port_idx_by_address(GPIO_TypeDef *port) {
    if (port == GPIOA) return 0;
    if (port == GPIOB) return 1;
    return -1;
}

static inline void _pin_control_cfg_prepare(gpio_pin_t *pin, const gpio_pin_t *uart_pin, const gpio_control_pin_t *gpio_pin) {
    // memset(pin, 0, sizeof(*pin));

    pin->port = uart_pin->port;
    pin->pin = uart_pin->pin;
    pin->dir = gpio_pin->dir;
    pin->func = gpio_func_general;
    // if (pin->dir == gpio_dir_output) {
        pin->output = gpio_pin->output;
    // } else { // output
        pin->pull = gpio_pin->pull;
    // }
    pin->polarity = gpio_polarity_high;
    pin->speed = gpio_speed_low;
}

void gpio_control_reconfigure_pin(GPIO_TypeDef *gpio_port, uint8_t gpio_pin_num) {
    if (gpio_pin_num >= GPIO_CONTROL_PINS_PER_PORT) return;
    int8_t port_idx = _port_idx_by_address(gpio_port);
    if (port_idx < 0) return;

    const gpio_control_pin_t *gpio_pin_cfg = &device_config_get()->gpio_control.ports[port_idx].pins[gpio_pin_num];
    if (gpio_pin_cfg->dir == gpio_dir_unknown) {
        // pin is marked as non-usable
        return;
    }
    gpio_pin_t *uart_pin = &device_config_get()->cdc_config.port_config[gpio_pin_cfg->uart_port].pins[gpio_pin_cfg->uart_pin];
    if (uart_pin->func != gpio_func_unknown) {
        // pin is marked for use by uart machinery
        return;
    }
    gpio_pin_t p;
    _pin_control_cfg_prepare(&p, uart_pin, gpio_pin_cfg);
    gpio_pin_init(&p);
}