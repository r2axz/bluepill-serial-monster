/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include <stm32f1xx.h>
#include "usb_uid.h"

usb_string_descriptor_t *usb_get_uid_string_descriptor() {
    const char hex_digits[] = "0123456789ABCDEF";
    const unsigned usb_device_uid_size = 12;
    static usb_string_descriptor_t uid_string_descriptor = USB_STRING_DESC("XXXXXXXXXXXXXXXXXXXXXXXX");
    uint8_t *uid_p = (uint8_t*)UID_BASE;
    uint16_t *str_p = uid_string_descriptor.wString;
    for (int i=0; i<usb_device_uid_size; i++) {
        *str_p++ = hex_digits[*uid_p >> 4];
        *str_p++ = hex_digits[*uid_p++ & 0x0f];
    }
    return &uid_string_descriptor;
}
