/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include <stm32f1xx.h>
#include "status_led.h"

void status_led_init() {
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRH &= ~GPIO_CRH_CNF13;
    GPIOC->CRH |= GPIO_CRH_MODE13_1;
    GPIOC->BSRR = GPIO_BSRR_BS13;
}

void status_led_set(int on) {
    (void)status_led_set; /* This function does not have to be used */
    GPIOC->BSRR = GPIO_BSRR_BS13 << (on ? GPIO_BSRR_BR0_Pos : 0);
}

void status_led_toggle() {
    (void)status_led_toggle; /* This function does not have to be used */
    GPIOC->BSRR = GPIO_BSRR_BS13 << ((GPIOC->ODR&GPIO_ODR_ODR13) ? GPIO_BSRR_BR0_Pos : 0);
}
