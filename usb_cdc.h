/*
 * MIT License
 *
 * Copyright (c) 2020 Kirill Kotyagin
 */

#ifndef USB_CDC_H
#define USB_CDC_H

#include <stdint.h>

#include "aux.h"
#include "usb_core.h"

/* USB CDC Class Codes */

typedef enum {
    usb_class_cdc           = 0x02,
    usb_class_cdc_data      = 0x0a,
} __packed usb_class_cdc_t;

typedef enum {
    usb_subclass_cdc_none   = 0x00,
    usb_subclass_cdc_acm    = 0x02,
} __packed usb_subclass_cdc_t;

typedef enum {
    usb_protocol_cdc_none   = 0x00,
    usb_protocol_cdc_v25ter = 0x01,
} __packed usb_protocol_cdc_t;

#define USB_PROTOCOL_CDC_DEFAULT usb_protocol_cdc_none

typedef enum {
    usb_descriptor_subtype_cdc_header          = 0x00,
    usb_descriptor_subtype_cdc_call_management = 0x01,
    usb_descriptor_subtype_cdc_acm             = 0x02,
    usb_descriptor_subtype_cdc_union           = 0x06,
    usb_descriptor_subtype_cdc_country         = 0x07,
} __packed usb_descriptor_subtype_cdc_t;

#define USB_CDC_ACM_CAPABILITY_COMM_FEATURE         0x01
#define USB_CDC_ACM_CAPABILITY_LINE_CODING          0x02
#define USB_CDC_ACM_CAPABILITY_SEND_BREAK           0x04
#define USB_CDC_ACM_CAPABILITY_NETWORK_CONNECTION   0x08

#define USB_CDC_ACM_CAPABILITIES (USB_CDC_ACM_CAPABILITY_LINE_CODING)

/* USB CDC Header Functional Descriptor */

typedef struct  {
    uint8_t     bFunctionLength;
    uint8_t     bDescriptorType;
    uint8_t     bDescriptorSubType;
    uint16_t    bcdCDC;
} __packed usb_cdc_header_desc_t;

/* USB CDC Union Functional Descriptor */

typedef struct {
    uint8_t     bFunctionLength;
    uint8_t     bDescriptorType;
    uint8_t     bDescriptorSubType;
    uint8_t     bMasterInterface0;
    uint8_t     bSlaveInterface0;
} __packed usb_cdc_union_desc_t;

typedef struct {
    uint8_t     bFunctionLength;
    uint8_t     bDescriptorType;
    uint8_t     bDescriptorSubType;
    uint8_t     bmCapabilities;
    uint8_t     bDataInterface;
} __packed usb_cdc_call_mgmt_desc_t;

/* USB CDC Abstract Control Management Functional Descriptor */

typedef struct {
    uint8_t     bFunctionLength;
    uint8_t     bDescriptorType;
    uint8_t     bDescriptorSubType;
    uint8_t     bmCapabilities;
} __packed usb_cdc_acm_desc_t;

/* USB CDC Notifications */

#define USB_CDC_NOTIFICATION_REQUEST_TYPE   0xa1

typedef enum {
    usb_cdc_notification_serial_state   = 0x20,
} __packed usb_cdc_notification_type_t;

typedef struct {
    uint8_t     bmRequestType;
    uint8_t     bNotificationType;
    uint16_t    wValue;
    uint16_t    wIndex;
    uint16_t    wLength;
    uint8_t     data[0];
} __packed usb_cdc_notification_t;

/* Serial State Notification Payload */
typedef uint16_t usb_cdc_serial_state_t;

#define USB_CDC_SERIAL_STATE_DCD            0x01
#define USB_CDC_SERIAL_STATE_DSR            0x02
#define USB_CDC_SERIAL_STATE_RI             0x08
#define USB_CDC_SERIAL_STATE_PARITY_ERROR   0x20
#define USB_CDC_SERIAL_STATE_OVERRUN        0x40

/* USB CDC Line Coding */

typedef enum {
    usb_cdc_char_format_1_stop_bit      = 0x00,
    usb_cdc_char_format_1p5_stop_bits   = 0x01,
    usb_cdc_char_format_2_stop_bits     = 0x02,
    usb_cdc_char_format_last
} __packed usb_cdc_char_format_t;

typedef enum {
    usb_cdc_parity_type_none    = 0x00,
    usb_cdc_parity_type_odd     = 0x01,
    usb_cdc_parity_type_even    = 0x02,
    usb_cdc_parity_type_mark    = 0x03,
    usb_cdc_parity_type_space   = 0x04,
} __packed usb_cdc_parity_type_t;

typedef enum {
    usb_cdc_data_bits_5     = 0x05,
    usb_cdc_data_bits_6     = 0x06,
    usb_cdc_data_bits_7     = 0x07,
    usb_cdc_data_bits_8     = 0x08,
    usb_cdc_data_bits_16    = 0x10,
} __packed usb_cdc_data_bits_t;

typedef struct {
    uint32_t                dwDTERate;
    usb_cdc_char_format_t   bCharFormat;
    usb_cdc_parity_type_t   bParityType;
    usb_cdc_data_bits_t     bDataBits;
} __packed usb_cdc_line_coding_t;

/* USB CDC Control Line State */

#define USB_CDC_CONTROL_LINE_STATE_DTR_MASK  0x01
#define USB_CDC_CONTROL_LINE_STATE_RTS_MASK  0x02

/* USB CDC Class-Specific Requests */

typedef enum {
    usb_cdc_request_send_encapsulated_command   = 0x00,
    usb_cdc_request_get_encapsulated_response   = 0x01,
    usb_cdc_request_set_comm_feature            = 0x02,
    usb_cdc_request_get_comm_feature            = 0x03,
    usb_cdc_request_clear_comm_feature          = 0x04,
    usb_cdc_request_set_line_coding             = 0x20,
    usb_cdc_request_get_line_coding             = 0x21,
    usb_cdc_request_set_control_line_state      = 0x22,
    usb_cdc_request_send_break                  = 0x23,
} __packed usb_cdc_request_t;

/* Control Endpoint Request Processing */

usb_status_t usb_cdc_ctrl_process_request(usb_setup_t *setup, void **payload,
                                          size_t *payload_size, usb_tx_complete_cb_t *tx_callback_ptr);

/* Data Endpoints Event Processing */

void usb_cdc_data_endpoint_event_handler(uint8_t ep_num, usb_endpoint_event_t ep_event);
void usb_cdc_interrupt_endpoint_event_handler(uint8_t ep_num, usb_endpoint_event_t ep_event);

/* Device lifecycle functions */

void usb_cdc_reset();
void usb_cdc_enable();
void usb_cdc_suspend();
void usb_cdc_frame();

/* CDC Pins */

typedef enum {
    cdc_pin_rx,
    cdc_pin_tx,
    cdc_pin_rts,
    cdc_pin_cts,
    cdc_pin_dsr,
    cdc_pin_dtr,
    cdc_pin_dcd,
    cdc_pin_ri,
    cdc_pin_txa,
    cdc_pin_unknown,
    cdc_pin_last = cdc_pin_unknown,
} __packed cdc_pin_t;

/* Configuration Changed Hooks */

void usb_cdc_reconfigure_port_pin(int port, cdc_pin_t pin);
void usb_cdc_reconfigure();

/* CDC Device Definitions */

#define USB_CDC_NUM_PORTS                       3
#define USB_CDC_BUF_SIZE                        0x400
#define USB_CDC_CRTL_LINES_POLLING_INTERVAL     20 /* ms */
#define USB_CDC_CONFIG_PORT                     0

/* CDC Polling */

void usb_cdc_poll();

#endif /* USB_CDC_H */
