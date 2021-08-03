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

static inline GPIO_TypeDef *_port_address_by_idx(int portnum) {
    if (portnum == 0) return GPIOA;
    if (portnum == 1) return GPIOB;
    return NULL;
}

static inline void _pin_control_cfg_prepare(gpio_pin_t *pin, int portnum, int pinnum) {
    const gpio_control_pin_t *gc_pin = &device_config_get()->gpio_control.ports[portnum].pins[pinnum];

    pin->port = _port_address_by_idx(portnum);
    pin->pin = pinnum;
    pin->dir = gc_pin->dir;
    pin->func = gpio_func_general;
    // if (pin->dir == gpio_dir_output) {
        pin->output = gc_pin->output;
    // } else { // output
        pin->pull = gc_pin->pull;
    // }
    pin->polarity = gpio_polarity_high;
    pin->speed = gpio_speed_low;
}

void gpio_control_reconfigure_pin(int portnum, int pinnum) {
    if (portnum >= GPIO_CONGROL_NUM_PORTS) return;
    if (pinnum >= GPIO_CONTROL_PINS_PER_PORT) return;

    const gpio_control_pin_t *gpio_pin_cfg = &device_config_get()->gpio_control.ports[portnum].pins[pinnum];
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
    _pin_control_cfg_prepare(&p, portnum, pinnum);
    gpio_pin_init(&p);
    if (p.dir == gpio_dir_output) {
        gpio_pin_set(&p, gpio_pin_cfg->val);
    }
}

gpio_pin_t *gpio_control_find_uart_pincfg(gpio_control_pin_t *gc_pin) {
    // if (gc_pin->dir == gpio_dir_unknown) return NULL;
    if (gc_pin->uart_port < 0) return NULL;
    return &device_config_get()->cdc_config.port_config[gc_pin->uart_port].pins[gc_pin->uart_pin];
}

cdc_use_t gpio_control_pin_get_use(int portnum, int pinnum) {
    if (portnum >= GPIO_CONGROL_NUM_PORTS) return cdc_use_unknown;
    if (pinnum >= GPIO_CONTROL_PINS_PER_PORT) return cdc_use_unknown;

    gpio_control_pin_t *gc_pin = &device_config_get()->gpio_control.ports[portnum].pins[pinnum];
    if (gc_pin->dir == gpio_dir_unknown) return cdc_use_unknown;
    gpio_pin_t *u_pin = gpio_control_find_uart_pincfg(gc_pin);
    if (u_pin->func == gpio_func_unknown) return cdc_use_gpio;
    return cdc_use_uart;
}

int gpio_control_read(int portnum, int pinnum) {
    if (portnum >= GPIO_CONGROL_NUM_PORTS) return -1;
    if (pinnum >= GPIO_CONTROL_PINS_PER_PORT) return -1;
    gpio_control_pin_t *gc_pin = &device_config_get()->gpio_control.ports[portnum].pins[pinnum];
    gpio_pin_t *u_pin = gpio_control_find_uart_pincfg(gc_pin);
    if (u_pin == NULL) return 0; // ???

    gpio_pin_t p;
    _pin_control_cfg_prepare(&p, portnum, pinnum);
    return gpio_pin_get(&p);
}

int gpio_control_write(int portnum, int pinnum, int val) {
    if (portnum >= GPIO_CONGROL_NUM_PORTS) return -1;
    if (pinnum >= GPIO_CONTROL_PINS_PER_PORT) return 1;

    if (gpio_control_pin_get_use(portnum, pinnum) != cdc_use_gpio) return -1;

    gpio_pin_t p;
    _pin_control_cfg_prepare(&p, portnum, pinnum);
    gpio_pin_set(&p, val);
    return 0;
}
