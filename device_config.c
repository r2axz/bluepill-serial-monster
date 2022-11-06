/*
 * MIT License
 *
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include <string.h>
#include <stm32f1xx.h>
#include <limits.h>
#include "device_config.h"
#include "default_config.h"

#define DEVICE_CONFIG_FLASH_SIZE    0x10000UL
#define DEVICE_CONFIG_NUM_PAGES     2
#define DEVICE_CONFIG_PAGE_SIZE     0x400UL
#define DEVICE_CONFIG_FLASH_END     (FLASH_BASE + DEVICE_CONFIG_FLASH_SIZE)
#define DEVICE_CONFIG_BASE_ADDR     ((void*)(DEVICE_CONFIG_FLASH_END - DEVICE_CONFIG_NUM_PAGES * DEVICE_CONFIG_PAGE_SIZE))
#define DEVICE_CONFIG_MAGIC         0xDECFDECFUL

static device_config_t current_device_config;

static uint32_t device_config_calc_crc(const device_config_t *device_config) {
    uint32_t *word_p = (uint32_t*)device_config;
    size_t bytes_left = offsetof(device_config_t, crc);
    CRC->CR |= CRC_CR_RESET;
    while (bytes_left > sizeof(*word_p)) {
        CRC->DR = *word_p++;
        bytes_left -= sizeof(*word_p);
    }
    if (bytes_left) {
        uint32_t shift = 0;
        uint32_t tail = 0;
        uint8_t *byte_p = (uint8_t*)word_p;
        for (int i = 0; i < bytes_left; i++) {
            tail |= (uint32_t)(*byte_p++) << (shift);
            shift += CHAR_BIT;
        }
        CRC->DR = tail;
    }
    return CRC->DR;
}

const static device_config_t* device_config_get_stored() {
    uint8_t *config_page = (uint8_t*)DEVICE_CONFIG_BASE_ADDR;
    size_t config_pages = DEVICE_CONFIG_NUM_PAGES;
    while (config_pages--) {
        const device_config_t *stored_config = (device_config_t*)config_page;
        if ((stored_config->magic == DEVICE_CONFIG_MAGIC) &&
            (device_config_calc_crc(stored_config) == stored_config->crc)) {
            return stored_config;
        }
        config_page += DEVICE_CONFIG_PAGE_SIZE;
    }
    return 0;
}

void device_config_init() {
    RCC->AHBENR |= RCC_AHBENR_CRCEN;
    const device_config_t *stored_config = device_config_get_stored();
    if (stored_config == 0) {
        default_config_load(&current_device_config);
    } else {
        memcpy(&current_device_config, stored_config, sizeof(*stored_config));
    }
}

device_config_t *device_config_get() {
    return &current_device_config;
}

void device_config_save() {
    uint16_t *last_config_magic = 0;
    uint8_t *config_page = (uint8_t*)DEVICE_CONFIG_BASE_ADDR;
    size_t config_pages = DEVICE_CONFIG_NUM_PAGES;
    uint16_t *src_word_p = (uint16_t*)&current_device_config;
    uint16_t *dst_word_p;
    size_t bytes_left = sizeof(current_device_config);
    while (config_pages-- && (last_config_magic == 0)) {
        const device_config_t *stored_config = (device_config_t*)config_page;
        if ((stored_config->magic == DEVICE_CONFIG_MAGIC) &&
            (device_config_calc_crc(stored_config) == stored_config->crc)) {
            last_config_magic = (uint16_t*)&stored_config->magic;
        }
        config_page += DEVICE_CONFIG_PAGE_SIZE;
    }
    if (config_page == ((uint8_t*)DEVICE_CONFIG_BASE_ADDR + (DEVICE_CONFIG_NUM_PAGES * DEVICE_CONFIG_PAGE_SIZE))) {
        config_page = (uint8_t*)DEVICE_CONFIG_BASE_ADDR;
    }
    dst_word_p = (uint16_t*)config_page;
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }
    while (FLASH->SR & FLASH_SR_BSY);
    FLASH->SR = FLASH->SR & FLASH_SR_EOP;
    FLASH->CR = FLASH_CR_PER;
    FLASH->AR = (uint32_t)config_page;
    FLASH->CR |=  FLASH_CR_STRT;
    while (!(FLASH->SR & FLASH_SR_EOP));
    FLASH->SR = FLASH_SR_EOP;
    FLASH->CR &= ~FLASH_CR_PER;
    current_device_config.magic = DEVICE_CONFIG_MAGIC;
    current_device_config.crc = device_config_calc_crc(&current_device_config);
    FLASH->CR |= FLASH_CR_PG;
    while (bytes_left > 1) {
        *dst_word_p++ = *src_word_p++;
        while (!(FLASH->SR & FLASH_SR_EOP));
        FLASH->SR = FLASH_SR_EOP;
        bytes_left -= sizeof(*dst_word_p);
    }
    if (bytes_left) {
        *dst_word_p = (uint16_t)(*(uint8_t*)src_word_p);
        while (!(FLASH->SR & FLASH_SR_EOP));
        FLASH->SR = FLASH_SR_EOP;
    }
    if (last_config_magic) {
        *last_config_magic = 0x0000;
        while (!(FLASH->SR & FLASH_SR_EOP));
        FLASH->SR = FLASH_SR_EOP;
    }
    FLASH->CR &= ~(FLASH_CR_PG);
    FLASH->CR |= FLASH_CR_LOCK;
}

void device_config_reset() {
    default_config_load(&current_device_config);
    device_config_save();
}

static int cdc_port_set_enable_confugred(int port, int enabled);
static void gpio_pin_alternative_update(int port, gpio_status_t new_status)
{
    device_config_t *device_config = device_config_get();
    cdc_port_t *port_config = &device_config->cdc_config.port_config[port];
    gpio_pin_t *rx = gpion_to_gpio(port_config->pins[cdc_pin_rx]);
    gpio_pin_t *tx = gpion_to_gpio(port_config->pins[cdc_pin_tx]);

    if (rx->status == gpio_status_occupied && tx->status == gpio_status_occupied) {
        cdc_port_set_enable_confugred(port, 1);
    } else if (rx->status != tx->status && new_status == gpio_status_free) {
        cdc_port_set_enable_confugred(port, 0);
    }
}

int gpio_pin_set_status(gpion_pin_t pinn, gpio_status_t new_status)
{
    gpio_pin_t *pin = gpion_to_gpio(pinn);
    device_config_t *device_config = device_config_get();
    if (!pin) return -1;
    if (pin->status == new_status || pin->status == gpio_status_blocked ||
        (new_status != gpio_status_free && new_status != gpio_status_occupied)) {
        return 0;
    }
    cdc_pin_ref_t cdc_pin = gpion_to_cdc(pinn);

    pin->status = new_status;
    if (cdc_pin.port >= 0 && cdc_pin.port < USB_CDC_NUM_PORTS) {
        if (new_status == gpio_status_occupied) {
            default_config_load_pin(device_config, pinn);
            usb_cdc_reconfigure_port_pin(cdc_pin.port, cdc_pin.pin);
        }
        if (cdc_pin.pin == cdc_pin_rx || cdc_pin.pin == cdc_pin_tx) {
            gpio_pin_alternative_update(cdc_pin.port, new_status);
        }
    }
    return 0;
}

int cdc_port_set_enable(int port, int enabled)
{
    device_config_t *device_config = device_config_get();
    cdc_port_t *port_config = &device_config->cdc_config.port_config[port];
    if (enabled) {
        gpio_pin_set_status(port_config->pins[cdc_pin_rx], gpio_status_occupied);
        gpio_pin_set_status(port_config->pins[cdc_pin_tx], gpio_status_occupied);
    } else {
        gpio_pin_set_status(port_config->pins[cdc_pin_rx], gpio_status_free);
        gpio_pin_set_status(port_config->pins[cdc_pin_tx], gpio_status_free);
    }
    return 0;
}

static int cdc_port_set_enable_confugred(int port, int enabled)
{
    device_config_t *device_config = device_config_get();
    cdc_port_t *port_config = &device_config->cdc_config.port_config[port];
    if (enabled) {
        usb_cdc_reconfigure_port(port);
        usb_cdc_enable_port(port);
    } else {
        usb_cdc_suspend_port(port);
        for (int pin = 0; pin < cdc_pin_last; ++pin) {
            gpio_pin_set_status(port_config->pins[pin], gpio_status_free);
        }
    }
    return 0;
}
