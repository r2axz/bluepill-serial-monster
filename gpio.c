#include "gpio.h"

static void _gpio_enable_port(GPIO_TypeDef *port) {
    int portnum = (((uint32_t)port - GPIOA_BASE) / (GPIOB_BASE - GPIOA_BASE));
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN << portnum;
}

void gpio_pin_init(const gpio_pin_t *pin) {
    volatile uint32_t *crx = &pin->port->CRL + (pin->pin >> 3);
    uint8_t crx_offset = (pin->pin & 0x07) << 2;
    uint32_t modecfg = 0;
    _gpio_enable_port(pin->port);
    *crx &= ~((GPIO_CRL_CNF0 | GPIO_CRL_MODE0) << crx_offset);
    if (pin->dir == gpio_dir_input) {
        if (pin->pull == gpio_pull_floating) {
            modecfg |= GPIO_CRL_CNF0_0;
        } else {
            modecfg |= GPIO_CRL_CNF0_1;
            pin->port->BSRR = ((pin->pull == gpio_pull_up) ? GPIO_BSRR_BS0 : GPIO_BSRR_BR0) << pin->pin;
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

void gpio_pin_set(const gpio_pin_t *pin, int is_active) {
    pin->port->BSRR = (GPIO_BSRR_BS0 << pin->pin) 
        << (!!is_active != (pin->polarity == gpio_polarity_low) ? 0 : GPIO_BSRR_BR0_Pos); 
}

int gpio_pin_get(const gpio_pin_t *pin) {
    return (!!(pin->port->IDR & (GPIO_IDR_IDR0 << pin->pin))) != (pin->polarity == gpio_polarity_low);
}
