/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stddef.h>
#include "usb_core.h"
#include "usb_std.h"
#include "usb_cdc.h"

/* USB VID/PID */

#define USB_ID_VENDOR   0x1209
#define USB_ID_PRODUCT  0xFFFE

/* String Descriptors */

typedef enum {
    usb_string_index_none = 0x00,
    usb_string_lang_id    = 0x00,
    usb_string_index_manufacturer,
    usb_string_index_product,
    usb_string_index_serial,
    usb_string_index_uart_1_interface_name,
    usb_string_index_uart_2_interface_name,
    usb_string_index_uart_3_interface_name,
    usb_string_index_last,
} __attribute__ ((packed)) usb_string_index_t;

extern const usb_string_descriptor_t *usb_string_descriptors[usb_string_index_last];

/* Endpoints */

enum {
    usb_endpoint_address_control            = 0x00,
    usb_endpoint_address_cdc_0_interrupt    = 0x01,
    usb_endpoint_address_cdc_0_data         = 0x02,
    usb_endpoint_address_cdc_1_interrupt    = 0x03,
    usb_endpoint_address_cdc_1_data         = 0x04,
    usb_endpoint_address_cdc_2_interrupt    = 0x05,
    usb_endpoint_address_cdc_2_data         = 0x06,
    usb_endpoint_address_last
};

extern const usb_endpoint_t usb_endpoints[usb_endpoint_address_last];

/* Interfaces */

enum {
    usb_interface_cdc_0 = 0x00,
    usb_interface_cdc_1 = 0x02,
    usb_interface_cdc_2 = 0x04,
};

/* Device Descriptor */

extern const usb_device_descriptor_t usb_device_descriptor;

/* Configuration Descriptor */

typedef struct {
    usb_configuration_descriptor_t      config;
    usb_iad_descriptor_t                comm_iad_0;
    usb_interface_descriptor_t          comm_0;
    usb_cdc_header_desc_t               cdc_hdr_0;
    usb_cdc_call_mgmt_desc_t            cdc_mgmt_0;
    usb_cdc_acm_desc_t                  cdc_acm_0;
    usb_cdc_union_desc_t                cdc_union_0;
    usb_endpoint_descriptor_t           comm_ep_0;
    usb_interface_descriptor_t          data_0;
    usb_endpoint_descriptor_t           data_eprx_0;
    usb_endpoint_descriptor_t           data_eptx_0;
    usb_iad_descriptor_t                comm_iad_1;
    usb_interface_descriptor_t          comm_1;
    usb_cdc_header_desc_t               cdc_hdr_1;
    usb_cdc_call_mgmt_desc_t            cdc_mgmt_1;
    usb_cdc_acm_desc_t                  cdc_acm_1;
    usb_cdc_union_desc_t                cdc_union_1;
    usb_endpoint_descriptor_t           comm_ep_1;
    usb_interface_descriptor_t          data_1;
    usb_endpoint_descriptor_t           data_eprx_1;
    usb_endpoint_descriptor_t           data_eptx_1;
    usb_iad_descriptor_t                comm_iad_2;
    usb_interface_descriptor_t          comm_2;
    usb_cdc_header_desc_t               cdc_hdr_2;
    usb_cdc_call_mgmt_desc_t            cdc_mgmt_2;
    usb_cdc_acm_desc_t                  cdc_acm_2;
    usb_cdc_union_desc_t                cdc_union_2;
    usb_endpoint_descriptor_t           comm_ep_2;
    usb_interface_descriptor_t          data_2;
    usb_endpoint_descriptor_t           data_eprx_2;
    usb_endpoint_descriptor_t           data_eptx_2;
} __attribute__((packed)) usb_device_configuration_descriptor_t;

extern const usb_device_configuration_descriptor_t usb_configuration_descriptor;

#endif /* USB_DESCRIPTORS_H */
