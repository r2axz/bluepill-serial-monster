/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include <string.h>
#include <stm32f1xx.h>
#include <limits.h>
#include "device_config.h"

#define DEVICE_CONFIG_FLASH_SIZE    0x10000UL
#define DEVICE_CONFIG_NUM_PAGES     2
#define DEVICE_CONFIG_PAGE_SIZE     0x400UL
#define DEVICE_CONFIG_FLASH_END     (FLASH_BASE + DEVICE_CONFIG_FLASH_SIZE)
#define DEVICE_CONFIG_BASE_ADDR     ((void*)(DEVICE_CONFIG_FLASH_END - DEVICE_CONFIG_NUM_PAGES * DEVICE_CONFIG_PAGE_SIZE))
#define DEVICE_CONFIG_MAGIC         0xDECFDECFUL

static const device_config_t default_device_config = {
    .status_led_pin = { .port = GPIOC, .pin = 13, .dir = gpio_dir_output, .speed = gpio_speed_low, .func = gpio_func_general, .output = gpio_output_od, .polarity = gpio_polarity_low },
    .config_pin = { .port = GPIOB, .pin = 5, .dir = gpio_dir_input, .pull = gpio_pull_up, .polarity = gpio_polarity_low },
    .cdc_config = {
        .port_config = {
            /*  Port 0 */
            {
                .pins = 
                {
                    /*  rx */ { .port = GPIOA, .pin = 10, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_high },
                    /*  tx */ { .port = GPIOA, .pin =  9, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_alternate, .output = gpio_output_pp, .polarity = gpio_polarity_high },
                    /* rts */ { .port = 0 }, /* No RTS due to the below reason  */
                    /* cts */ { .port = 0 }, /* CTS pin is occupied by USB      */
                    /* dsr */ { .port = GPIOB, .pin =  7, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                    /* dtr */ { .port = GPIOA, .pin =  4, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low  },
                    /* dcd */ { .port = GPIOB, .pin = 15, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                    /*  ri */ { .port = GPIOB, .pin =  3, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                    /* txa */ { .port = GPIOB, .pin =  0, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_high  },
                }
            },
            /*  Port 1 */
            {
                .pins = 
                {
                    /*  rx */ { .port = GPIOA, .pin =  3, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_high },
                    /*  tx */ { .port = GPIOA, .pin =  2, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_alternate, .output = gpio_output_pp, .polarity = gpio_polarity_high },
                    /* rts */ { .port = GPIOA, .pin =  1, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low},
                    /* cts */ { .port = GPIOA, .pin =  0, .dir = gpio_dir_input,  .pull = gpio_pull_down, .polarity = gpio_polarity_low },
                    /* dsr */ { .port = GPIOB, .pin =  4, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                    /* dtr */ { .port = GPIOA, .pin =  5, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low },
                    /* dcd */ { .port = GPIOB, .pin =  8, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                    /*  ri */ { .port = GPIOB, .pin = 12, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                    /* txa */ { .port = GPIOB, .pin =  1, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_high  },
                }
            },
            /*  Port 2 */
            {
                .pins = 
                {
                    /*  rx */ { .port = GPIOB, .pin = 11, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_high },
                    /*  tx */ { .port = GPIOB, .pin = 10, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_alternate, .output = gpio_output_pp, .polarity = gpio_polarity_high  },
                    /* rts */ { .port = GPIOB, .pin = 14, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low },
                    /* cts */ { .port = GPIOB, .pin = 13, .dir = gpio_dir_input,  .pull = gpio_pull_down, .polarity = gpio_polarity_low },
                    /* dsr */ { .port = GPIOB, .pin =  6, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                    /* dtr */ { .port = GPIOA, .pin =  6, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low  },
                    /* dcd */ { .port = GPIOB, .pin =  9, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                    /*  ri */ { .port = GPIOA, .pin =  8, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                    /* txa */ { .port = GPIOA, .pin =  7, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_high  },
                }
            },
        }
    }
};

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
        stored_config = &default_device_config;
    }
    memcpy(&current_device_config, stored_config, sizeof(*stored_config));
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
    memcpy(&current_device_config, &default_device_config, sizeof(default_device_config));
    device_config_save();
}