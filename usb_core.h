/*
 * MIT License
 *
 * Copyright (c) 2020 Kirill Kotyagin
 */

#ifndef USB_CORE_H
#define USB_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stm32f1xx.h>
#include "usb_std.h"
#include "usb_io.h"

#define USB_SETUP_MAX_PAYLOAD_SIZE   32
#define USB_SETUP_MAX_SIZE (sizeof(usb_setup_t) + USB_SETUP_MAX_PAYLOAD_SIZE)

/* USB Control Endpoint Operation Status */

typedef enum {
    usb_status_ack     = 0x00,
    usb_status_nak     = 0x01,
    usb_status_fail    = 0x02,
} usb_status_t;

/* USB Control Endpoint Exchange Complete Callback */

typedef void (*usb_tx_complete_cb_t)();

/* USB Control Endpoint Event Handler */

void usb_control_endpoint_event_handler(uint8_t ep_num, usb_endpoint_event_t ep_event);

/* USB Device State */

typedef enum {
    usb_device_state_reset          = 0x00,
    usb_device_state_address_set    = 0x01,
    usb_device_state_configured     = 0x02,
} usb_device_state_t;

/* Device Level Events */

void usb_device_handle_reset();
void usb_device_handle_configured();
void usb_device_handle_suspend();
void usb_device_handle_wakeup();
void usb_device_handle_frame();
void usb_device_poll();

#endif /* USB_CORE_H */
