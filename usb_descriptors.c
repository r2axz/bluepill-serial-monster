/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include "usb_core.h"
#include "usb_cdc.h"
#include "usb_descriptors.h"

const usb_endpoint_t usb_endpoints[usb_endpoint_address_last] = {
    /*  Default Control Endpoint */
    {
        .type       = usb_endpoint_type_control,
        .rx_size    = 8,
        .tx_size    = 8,
        .event_handler = usb_control_endpoint_event_handler,
    },
    /*  CDC 0 Interrupt Endpoint */
    { 
        .type       = usb_endpoint_type_interrupt,
        .rx_size    = 0,
        .tx_size    = 16,
        .interval   = 100,
        .event_handler = usb_cdc_interrupt_endpoint_event_handler,
    },
     /*  CDC 0 Data Endpoint */
    { 
        .type       = usb_endpoint_type_bulk,
        .rx_size    = 32,
        .tx_size    = 32,
        .interval   = 0,
        .event_handler = usb_cdc_data_endpoint_event_handler,
    },
    /*  CDC 1 Interrupt Endpoint */
    { 
        .type       = usb_endpoint_type_interrupt,
        .rx_size    = 0,
        .tx_size    = 16,
        .interval   = 100,
        .event_handler = usb_cdc_interrupt_endpoint_event_handler,
    },
     /*  CDC 1 Data Endpoint */
    { 
        .type       = usb_endpoint_type_bulk,
        .rx_size    = 64,
        .tx_size    = 64,
        .interval   = 0,
        .event_handler = usb_cdc_data_endpoint_event_handler,
    },
    /*  CDC 2 Interrupt Endpoint */
    { 
        .type       = usb_endpoint_type_interrupt,
        .rx_size    = 0,
        .tx_size    = 16,
        .interval   = 100,
        .event_handler = usb_cdc_interrupt_endpoint_event_handler,
    },
     /*  CDC 2 Data Endpoint */
    { 
        .type       = usb_endpoint_type_bulk,
        .rx_size    = 64,
        .tx_size    = 64,
        .interval   = 0,
        .event_handler = usb_cdc_data_endpoint_event_handler,
    },
};

const usb_string_descriptor_t usb_string_lang            = USB_ARRAY_DESC(usb_language_code_en_US);
const usb_string_descriptor_t usb_string_manufacturer    = USB_STRING_DESC("R2AXZ Kirill Kotyagin");
const usb_string_descriptor_t usb_string_product         = USB_STRING_DESC("STM32 Blue Pill Serial Monster");
const usb_string_descriptor_t usb_string_serial          = USB_STRING_DESC("NO SERIAL"); /* Placeholder, replaced by STM32 UID */

const usb_string_descriptor_t *usb_string_descriptors[usb_string_index_last] = {
    &usb_string_lang,
    &usb_string_manufacturer,
    &usb_string_product,
    &usb_string_serial,
};

const usb_device_descriptor_t usb_device_descriptor = {
    .bLength            = sizeof(usb_device_descriptor),
    .bDescriptorType    = usb_descriptor_type_device,
    .bcdUSB             = USB_BCD_VERSION(2, 0, 0),
    .bDeviceClass       = usb_device_class_misc,
    .bDeviceSubClass    = usb_device_subclass_iad,
    .bDeviceProtocol    = usb_device_protocol_iad,
    .bMaxPacketSize     = usb_endpoints[usb_endpoint_address_control].rx_size,
    .idVendor           = USB_ID_VENDOR,
    .idProduct          = USB_ID_PRODUCT,
    .bcdDevice          = USB_BCD_VERSION(1, 0, 0),
    .iManufacturer      = usb_string_index_manufacturer,
    .iProduct           = usb_string_index_product,
    .iSerialNumber      = usb_string_index_serial,
    .bNumConfigurations = 1,
};

const usb_device_configuration_descriptor_t usb_configuration_descriptor = {
    .config = {
        .bLength                = sizeof(usb_configuration_descriptor.config),
        .bDescriptorType        = usb_descriptor_type_configuration,
        .wTotalLength           = sizeof(usb_configuration_descriptor),
        .bNumInterfaces         = 6,
        .bConfigurationValue    = 1,
        .iConfiguration         = usb_string_index_none,
        .bmAttributes           = USB_CFG_ATTR_RESERVED,
        .bMaxPower              = USB_CFG_POWER_MA(500),
    },
    .comm_iad_0 = {
        .bLength                = sizeof(usb_configuration_descriptor.comm_iad_0),
        .bDescriptorType        = usb_descriptor_type_interface_assoc,
        .bFirstInterface        = usb_interface_cdc_0,
        .bInterfaceCount        = 2,
        .bFunctionClass         = usb_class_cdc,
        .bFunctionSubClass      = usb_subclass_cdc_acm,
        .bFunctionProtocol      = USB_PROTOCOL_CDC_DEFAULT,
        .iFunction              = usb_string_index_none,
    },
    .comm_0 = {
        .bLength                = sizeof(usb_configuration_descriptor.comm_0),
        .bDescriptorType        = usb_descriptor_type_interface,
        .bInterfaceNumber       = usb_interface_cdc_0,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 1,
        .bInterfaceClass        = usb_class_cdc,
        .bInterfaceSubClass     = usb_subclass_cdc_acm,
        .bInterfaceProtocol     = USB_PROTOCOL_CDC_DEFAULT,
        .iInterface             = usb_string_index_none
    },
    .cdc_hdr_0 = {
        .bFunctionLength        = sizeof(usb_configuration_descriptor.cdc_hdr_0),
        .bDescriptorType        = usb_descriptor_type_cs_interface,
        .bDescriptorSubType     = usb_descriptor_subtype_cdc_header,
        .bcdCDC                 = USB_BCD_VERSION(1, 1, 0),
    },
    .cdc_mgmt_0 = {
        .bFunctionLength        = sizeof(usb_configuration_descriptor.cdc_mgmt_0),
        .bDescriptorType        = usb_descriptor_type_cs_interface,
        .bDescriptorSubType     = usb_descriptor_subtype_cdc_call_management,
        .bmCapabilities         = 0,
        .bDataInterface         = usb_interface_cdc_0 + 1,
    },
    .cdc_acm_0 = {
        .bFunctionLength        = sizeof(usb_configuration_descriptor.cdc_acm_0),
        .bDescriptorType        = usb_descriptor_type_cs_interface,
        .bDescriptorSubType     = usb_descriptor_subtype_cdc_acm,
        .bmCapabilities         = 0,
    },
    .cdc_union_0 = {
        .bFunctionLength        = sizeof(usb_configuration_descriptor.cdc_union_0),
        .bDescriptorType        = usb_descriptor_type_cs_interface,
        .bDescriptorSubType     = usb_descriptor_subtype_cdc_union,
        .bMasterInterface0      = usb_interface_cdc_0,
        .bSlaveInterface0       = usb_interface_cdc_0 + 1,
    },
    .comm_ep_0 = {
        .bLength                = sizeof(usb_configuration_descriptor.comm_ep_0),
        .bDescriptorType        = usb_descriptor_type_endpoint,
        .bEndpointAddress       = usb_endpoint_direction_in | usb_endpoint_address_cdc_0_interrupt,
        .bmAttributes           = usb_endpoint_type_interrupt,
        .wMaxPacketSize         = usb_endpoints[usb_endpoint_address_cdc_0_interrupt].tx_size,
        .bInterval              = usb_endpoints[usb_endpoint_address_cdc_0_interrupt].interval,
    },
    .data_0 = {
        .bLength                = sizeof(usb_configuration_descriptor.data_0),
        .bDescriptorType        = usb_descriptor_type_interface,
        .bInterfaceNumber       = usb_interface_cdc_0 + 1,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 2,
        .bInterfaceClass        = usb_class_cdc_data,
        .bInterfaceSubClass     = usb_subclass_cdc_none,
        .bInterfaceProtocol     = usb_protocol_cdc_none,
        .iInterface             = usb_string_index_none,
    },
    .data_eprx_0 = {
        .bLength                = sizeof(usb_configuration_descriptor.data_eprx_0),
        .bDescriptorType        = usb_descriptor_type_endpoint,
        .bEndpointAddress       = usb_endpoint_direction_out | usb_endpoint_address_cdc_0_data,
        .bmAttributes           = usb_endpoint_type_bulk,
        .wMaxPacketSize         = usb_endpoints[usb_endpoint_address_cdc_0_data].rx_size,
        .bInterval              = usb_endpoints[usb_endpoint_address_cdc_0_data].interval,
    },
    .data_eptx_0 = {
        .bLength                = sizeof(usb_configuration_descriptor.data_eprx_0),
        .bDescriptorType        = usb_descriptor_type_endpoint,
        .bEndpointAddress       = usb_endpoint_direction_in | usb_endpoint_address_cdc_0_data,
        .bmAttributes           = usb_endpoint_type_bulk,
        .wMaxPacketSize         = usb_endpoints[usb_endpoint_address_cdc_0_data].tx_size,
        .bInterval              = usb_endpoints[usb_endpoint_address_cdc_0_data].interval,
    },
    .comm_iad_1 = {
        .bLength                = sizeof(usb_configuration_descriptor.comm_iad_1),
        .bDescriptorType        = usb_descriptor_type_interface_assoc,
        .bFirstInterface        = usb_interface_cdc_1,
        .bInterfaceCount        = 2,
        .bFunctionClass         = usb_class_cdc,
        .bFunctionSubClass      = usb_subclass_cdc_acm,
        .bFunctionProtocol      = USB_PROTOCOL_CDC_DEFAULT,
        .iFunction              = usb_string_index_none,
    },
    .comm_1 = {
        .bLength                = sizeof(usb_configuration_descriptor.comm_1),
        .bDescriptorType        = usb_descriptor_type_interface,
        .bInterfaceNumber       = usb_interface_cdc_1,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 1,
        .bInterfaceClass        = usb_class_cdc,
        .bInterfaceSubClass     = usb_subclass_cdc_acm,
        .bInterfaceProtocol     = USB_PROTOCOL_CDC_DEFAULT,
        .iInterface             = usb_string_index_none
    },
    .cdc_hdr_1 = {
        .bFunctionLength        = sizeof(usb_configuration_descriptor.cdc_hdr_1),
        .bDescriptorType        = usb_descriptor_type_cs_interface,
        .bDescriptorSubType     = usb_descriptor_subtype_cdc_header,
        .bcdCDC                 = USB_BCD_VERSION(1, 1, 0),
    },
    .cdc_mgmt_1 = {
        .bFunctionLength        = sizeof(usb_configuration_descriptor.cdc_mgmt_1),
        .bDescriptorType        = usb_descriptor_type_cs_interface,
        .bDescriptorSubType     = usb_descriptor_subtype_cdc_call_management,
        .bmCapabilities         = 0,
        .bDataInterface         = usb_interface_cdc_1 + 1,
    },
    .cdc_acm_1 = {
        .bFunctionLength        = sizeof(usb_configuration_descriptor.cdc_acm_1),
        .bDescriptorType        = usb_descriptor_type_cs_interface,
        .bDescriptorSubType     = usb_descriptor_subtype_cdc_acm,
        .bmCapabilities         = 0,
    },
    .cdc_union_1 = {
        .bFunctionLength        = sizeof(usb_configuration_descriptor.cdc_union_1),
        .bDescriptorType        = usb_descriptor_type_cs_interface,
        .bDescriptorSubType     = usb_descriptor_subtype_cdc_union,
        .bMasterInterface0      = usb_interface_cdc_1,
        .bSlaveInterface0       = usb_interface_cdc_1 + 1,
    },
    .comm_ep_1 = {
        .bLength                = sizeof(usb_configuration_descriptor.comm_ep_1),
        .bDescriptorType        = usb_descriptor_type_endpoint,
        .bEndpointAddress       = usb_endpoint_direction_in | usb_endpoint_address_cdc_1_interrupt,
        .bmAttributes           = usb_endpoint_type_interrupt,
        .wMaxPacketSize         = usb_endpoints[usb_endpoint_address_cdc_1_interrupt].tx_size,
        .bInterval              = usb_endpoints[usb_endpoint_address_cdc_1_interrupt].interval,
    },
    .data_1 = {
        .bLength                = sizeof(usb_configuration_descriptor.data_1),
        .bDescriptorType        = usb_descriptor_type_interface,
        .bInterfaceNumber       = usb_interface_cdc_1 + 1,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 2,
        .bInterfaceClass        = usb_class_cdc_data,
        .bInterfaceSubClass     = usb_subclass_cdc_none,
        .bInterfaceProtocol     = usb_protocol_cdc_none,
        .iInterface             = usb_string_index_none,
    },
    .data_eprx_1 = {
        .bLength                = sizeof(usb_configuration_descriptor.data_eprx_1),
        .bDescriptorType        = usb_descriptor_type_endpoint,
        .bEndpointAddress       = usb_endpoint_direction_out | usb_endpoint_address_cdc_1_data,
        .bmAttributes           = usb_endpoint_type_bulk,
        .wMaxPacketSize         = usb_endpoints[usb_endpoint_address_cdc_1_data].rx_size,
        .bInterval              = usb_endpoints[usb_endpoint_address_cdc_1_data].interval,
    },
    .data_eptx_1 = {
        .bLength                = sizeof(usb_configuration_descriptor.data_eprx_1),
        .bDescriptorType        = usb_descriptor_type_endpoint,
        .bEndpointAddress       = usb_endpoint_direction_in | usb_endpoint_address_cdc_1_data,
        .bmAttributes           = usb_endpoint_type_bulk,
        .wMaxPacketSize         = usb_endpoints[usb_endpoint_address_cdc_1_data].tx_size,
        .bInterval              = usb_endpoints[usb_endpoint_address_cdc_1_data].interval,
    },
    .comm_iad_2 = {
        .bLength                = sizeof(usb_configuration_descriptor.comm_iad_2),
        .bDescriptorType        = usb_descriptor_type_interface_assoc,
        .bFirstInterface        = usb_interface_cdc_2,
        .bInterfaceCount        = 2,
        .bFunctionClass         = usb_class_cdc,
        .bFunctionSubClass      = usb_subclass_cdc_acm,
        .bFunctionProtocol      = USB_PROTOCOL_CDC_DEFAULT,
        .iFunction              = usb_string_index_none,
    },
    .comm_2 = {
        .bLength                = sizeof(usb_configuration_descriptor.comm_2),
        .bDescriptorType        = usb_descriptor_type_interface,
        .bInterfaceNumber       = usb_interface_cdc_2,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 1,
        .bInterfaceClass        = usb_class_cdc,
        .bInterfaceSubClass     = usb_subclass_cdc_acm,
        .bInterfaceProtocol     = USB_PROTOCOL_CDC_DEFAULT,
        .iInterface             = usb_string_index_none
    },
    .cdc_hdr_2 = {
        .bFunctionLength        = sizeof(usb_configuration_descriptor.cdc_hdr_2),
        .bDescriptorType        = usb_descriptor_type_cs_interface,
        .bDescriptorSubType     = usb_descriptor_subtype_cdc_header,
        .bcdCDC                 = USB_BCD_VERSION(1, 1, 0),
    },
    .cdc_mgmt_2 = {
        .bFunctionLength        = sizeof(usb_configuration_descriptor.cdc_mgmt_2),
        .bDescriptorType        = usb_descriptor_type_cs_interface,
        .bDescriptorSubType     = usb_descriptor_subtype_cdc_call_management,
        .bmCapabilities         = 0,
        .bDataInterface         = usb_interface_cdc_2 + 1,
    },
    .cdc_acm_2 = {
        .bFunctionLength        = sizeof(usb_configuration_descriptor.cdc_acm_2),
        .bDescriptorType        = usb_descriptor_type_cs_interface,
        .bDescriptorSubType     = usb_descriptor_subtype_cdc_acm,
        .bmCapabilities         = 0,
    },
    .cdc_union_2 = {
        .bFunctionLength        = sizeof(usb_configuration_descriptor.cdc_union_2),
        .bDescriptorType        = usb_descriptor_type_cs_interface,
        .bDescriptorSubType     = usb_descriptor_subtype_cdc_union,
        .bMasterInterface0      = usb_interface_cdc_2,
        .bSlaveInterface0       = usb_interface_cdc_2 + 1,
    },
    .comm_ep_2 = {
        .bLength                = sizeof(usb_configuration_descriptor.comm_ep_2),
        .bDescriptorType        = usb_descriptor_type_endpoint,
        .bEndpointAddress       = usb_endpoint_direction_in | usb_endpoint_address_cdc_2_interrupt,
        .bmAttributes           = usb_endpoint_type_interrupt,
        .wMaxPacketSize         = usb_endpoints[usb_endpoint_address_cdc_2_interrupt].tx_size,
        .bInterval              = usb_endpoints[usb_endpoint_address_cdc_2_interrupt].interval,
    },
    .data_2 = {
        .bLength                = sizeof(usb_configuration_descriptor.data_2),
        .bDescriptorType        = usb_descriptor_type_interface,
        .bInterfaceNumber       = usb_interface_cdc_2 + 1,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 2,
        .bInterfaceClass        = usb_class_cdc_data,
        .bInterfaceSubClass     = usb_subclass_cdc_none,
        .bInterfaceProtocol     = usb_protocol_cdc_none,
        .iInterface             = usb_string_index_none,
    },
    .data_eprx_2 = {
        .bLength                = sizeof(usb_configuration_descriptor.data_eprx_2),
        .bDescriptorType        = usb_descriptor_type_endpoint,
        .bEndpointAddress       = usb_endpoint_direction_out | usb_endpoint_address_cdc_2_data,
        .bmAttributes           = usb_endpoint_type_bulk,
        .wMaxPacketSize         = usb_endpoints[usb_endpoint_address_cdc_2_data].rx_size,
        .bInterval              = usb_endpoints[usb_endpoint_address_cdc_2_data].interval,
    },
    .data_eptx_2 = {
        .bLength                = sizeof(usb_configuration_descriptor.data_eprx_2),
        .bDescriptorType        = usb_descriptor_type_endpoint,
        .bEndpointAddress       = usb_endpoint_direction_in | usb_endpoint_address_cdc_2_data,
        .bmAttributes           = usb_endpoint_type_bulk,
        .wMaxPacketSize         = usb_endpoints[usb_endpoint_address_cdc_2_data].tx_size,
        .bInterval              = usb_endpoints[usb_endpoint_address_cdc_2_data].interval,
    },

};
