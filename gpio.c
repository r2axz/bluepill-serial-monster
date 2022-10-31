/*
 * MIT License
 *
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include "gpio.h"
#include "gpion.h"

static void _gpio_enable_port(GPIO_TypeDef *port) {
    int portnum = (((uint32_t)port - GPIOA_BASE) / (GPIOB_BASE - GPIOA_BASE));
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN << portnum;
}

void gpio_pin_init(const gpio_pin_t *pin) {
    gpio_hal_t hal = gpio_to_hal(pin);
    if (hal.port) {
        volatile uint32_t *crx = &hal.port->CRL + (hal.pin >> 3);
        uint8_t crx_offset = (hal.pin & 0x07) << 2;
        uint32_t modecfg = 0;
        _gpio_enable_port(hal.port);
        *crx &= ~((GPIO_CRL_CNF0 | GPIO_CRL_MODE0) << crx_offset);
        if (pin->dir == gpio_dir_input) {
            if (pin->pull == gpio_pull_floating) {
                modecfg |= GPIO_CRL_CNF0_0;
            } else {
                modecfg |= GPIO_CRL_CNF0_1;
                hal.port->BSRR = ((pin->pull == gpio_pull_up) ? GPIO_BSRR_BS0 : GPIO_BSRR_BR0) << hal.pin;
            }
        } else {
            switch (pin->speed) {
            case gpio_speed_unknown:
            case gpio_speed_low:
                modecfg |= GPIO_CRL_MODE0_1;
                break;
            case gpio_speed_medium:
                modecfg |= GPIO_CRL_MODE0_0;
                break;
            case gpio_speed_high:
                modecfg |= GPIO_CRL_MODE0;
                break;
            }
            if (pin->output == gpio_output_od) {
                modecfg |= GPIO_CRL_CNF0_0;
            }
            if (pin->func == gpio_func_alternate) {
                modecfg |= GPIO_CRL_CNF0_1;
            }
        }
        *crx |= (modecfg << crx_offset);
    }
}

void gpio_pin_set(const gpio_pin_t *pin, int is_active) {
    gpio_hal_t hal = gpio_to_hal(pin);
    if (hal.port) {
        hal.port->BSRR = (GPIO_BSRR_BS0 << hal.pin)
            << (!!is_active != (pin->polarity == gpio_polarity_low) ? 0 : GPIO_BSRR_BR0_Pos);
    }
}

int gpio_pin_get(const gpio_pin_t *pin) {
    gpio_hal_t hal = gpio_to_hal(pin);
    if (hal.port) {
        return (!!(hal.port->IDR & (GPIO_IDR_IDR0 << hal.pin))) != (pin->polarity == gpio_polarity_low);
    }
    return 0;
}

volatile uint32_t *gpio_pin_get_bitband_clear_addr(const gpio_pin_t *pin) {
    volatile uint32_t result = 0;
    gpio_hal_t hal = gpio_to_hal(pin);
    if (hal.port) {
        result = PERIPH_BB_BASE;
        result += ((uint32_t)(&hal.port->BSRR) - PERIPH_BASE) << 5;
        result += hal.pin << 2;
        if (pin->polarity == gpio_polarity_high) {
            result += GPIO_BSRR_BR0_Pos << 2;
        }
    }
    return (volatile uint32_t*)result;
}

gpio_hal_t gpio_to_hal(const gpio_pin_t *pin)
{
    return gpion_to_hal(gpio_to_gpion(pin));
}
