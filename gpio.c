#include "gpio.h"

static void _gpio_enable_port(GPIO_TypeDef *port) {
    int portnum = (((uint32_t)port - GPIOA_BASE) / (GPIOB_BASE - GPIOA_BASE));
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN << portnum;
}

void gpio_pin_init(const gpio_pin_config_t *pincfg) {
    volatile uint32_t *crx = &pincfg->port->CRL + (pincfg->pin >> 3);
    uint8_t crx_offset = (pincfg->pin & 0x07) << 2;
    uint32_t modecfg = 0;
    _gpio_enable_port(pincfg->port);
    if (pincfg->dir == gpio_dir_input) {
        if (pincfg->pull == gpio_pull_floating) {
            modecfg |= GPIO_CRL_CNF0_0;
        } else {
            modecfg |= GPIO_CRL_CNF0_1;
            pincfg->port->BSRR = ((pincfg->pull == gpio_pull_up) ? GPIO_BSRR_BS0 : GPIO_BSRR_BR0) << pincfg->pin;
        }
    } else {
        switch (pincfg->speed) {
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
        if (pincfg->output == gpio_output_od) {
            modecfg |= GPIO_CRL_CNF0_0;
        }
        if (pincfg->func == gpio_func_alternate) {
            modecfg |= GPIO_CRL_CNF0_1;
        }
    }
    *crx &= ~((GPIO_CRL_CNF0 | GPIO_CRL_MODE0) << crx_offset);
    *crx |= (modecfg << crx_offset);
}
