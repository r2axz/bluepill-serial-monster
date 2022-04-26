/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include "usb_core.h"
#include "usb_cdc.h"
#include "version.h"
#include "usb_descriptors.h"

#define USB_CONTROL_ENDPOINT_SIZE           16
#define USB_CDC_INTERRUPT_ENDPOINT_SIZE     16
#define USB_CDC_DATA_ENDPOINT_SIZE_SMALL    32
#define USB_CDC_DATA_ENDPOINT_SIZE_LARGE    64

#define USB_CDC_DATA_0_ENDPOINT_SIZE         USB_CDC_DATA_ENDPOINT_SIZE_SMALL
#define USB_CDC_DATA_1_ENDPOINT_SIZE         USB_CDC_DATA_ENDPOINT_SIZE_LARGE
#define USB_CDC_DATA_2_ENDPOINT_SIZE         USB_CDC_DATA_ENDPOINT_SIZE_LARGE

#define USB_CDC_INTERRUPT_ENDPOINT_POLLING_INTERVAL 20

const usb_endpoint_t usb_endpoints[usb_endpoint_address_last] = {
    /*  Default Control Endpoint */
    {
        .type       = usb_endpoint_type_control,
        .rx_size    = USB_CONTROL_ENDPOINT_SIZE,
        .tx_size    = USB_CONTROL_ENDPOINT_SIZE,
        .event_handler = usb_control_endpoint_event_handler,
    },
    /*  CDC 0 Interrupt Endpoint */
    { 
        .type       = usb_endpoint_type_interrupt,
        .rx_size    = 0,
        .tx_size    = USB_CDC_INTERRUPT_ENDPOINT_SIZE,
        .event_handler = 0,
    },
     /*  CDC 0 Data Endpoint */
    { 
        .type       = usb_endpoint_type_bulk,
        .rx_size    = USB_CDC_DATA_0_ENDPOINT_SIZE,
        .tx_size    = USB_CDC_DATA_0_ENDPOINT_SIZE,
        .event_handler = usb_cdc_data_endpoint_event_handler,
    },
    /*  CDC 1 Interrupt Endpoint */
    { 
        .type       = usb_endpoint_type_interrupt,
        .rx_size    = 0,
        .tx_size    = USB_CDC_INTERRUPT_ENDPOINT_SIZE,
        .event_handler = 0,
    },
     /*  CDC 1 Data Endpoint */
    { 
        .type       = usb_endpoint_type_bulk,
        .rx_size    = USB_CDC_DATA_1_ENDPOINT_SIZE,
        .tx_size    = USB_CDC_DATA_1_ENDPOINT_SIZE,
        .event_handler = usb_cdc_data_endpoint_event_handler,
    },
    /*  CDC 2 Interrupt Endpoint */
    { 
        .type       = usb_endpoint_type_interrupt,
        .rx_size    = 0,
        .tx_size    = USB_CDC_INTERRUPT_ENDPOINT_SIZE,
        .event_handler = 0,
    },
     /*  CDC 2 Data Endpoint */
    { 
        .type       = usb_endpoint_type_bulk,
        .rx_size    = USB_CDC_DATA_2_ENDPOINT_SIZE,
        .tx_size    = USB_CDC_DATA_2_ENDPOINT_SIZE,
        .event_handler = usb_cdc_data_endpoint_event_handler,
    },
};

const usb_string_descriptor_t usb_string_lang                   = USB_ARRAY_DESC(usb_language_code_en_US);
const usb_string_descriptor_t usb_string_manufacturer           = USB_STRING_DESC("R2AXZ Kirill Kotyagin");
const usb_string_descriptor_t usb_string_product                = USB_STRING_DESC("Bluepill Serial Monster");
const usb_string_descriptor_t usb_string_serial                 = USB_STRING_DESC("NO SERIAL"); /* Placeholder, replaced by STM32 UID */
const usb_string_descriptor_t usb_string_uart_1_interface_name  = USB_STRING_DESC("UART1");
const usb_string_descriptor_t usb_string_uart_2_interface_name  = USB_STRING_DESC("UART2");
const usb_string_descriptor_t usb_string_uart_3_interface_name  = USB_STRING_DESC("UART3");

const usb_string_descriptor_t *usb_string_descriptors[usb_string_index_last] = {
    &usb_string_lang,
    &usb_string_manufacturer,
    &usb_string_product,
    &usb_string_serial,
    &usb_string_uart_1_interface_name,
    &usb_string_uart_2_interface_name,
    &usb_string_uart_3_interface_name,
};

const usb_device_descriptor_t usb_device_descriptor = {
    .bLength            = sizeof(usb_device_descriptor),
    .bDescriptorType    = usb_descriptor_type_device,
    .bcdUSB             = USB_BCD_VERSION(2, 0, 0),
    .bDeviceClass       = usb_device_class_misc,
    .bDeviceSubClass    = usb_device_subclass_iad,
    .bDeviceProtocol    = usb_device_protocol_iad,
    .bMaxPacketSize     = USB_CONTROL_ENDPOINT_SIZE,
    .idVendor           = USB_ID_VENDOR,
    .idProduct          = USB_ID_PRODUCT,
    .bcdDevice          = USB_BCD_VERSION(DEVICE_VERSION_MAJOR, DEVICE_VERSION_MINOR, DEVICE_VERSION_REVISION),
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
        .iInterface             = usb_string_index_uart_1_interface_name,
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
        .bmCapabilities         = USB_CDC_ACM_CAPABILITIES,
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
        .wMaxPacketSize         = USB_CDC_INTERRUPT_ENDPOINT_SIZE,
        .bInterval              = USB_CDC_INTERRUPT_ENDPOINT_POLLING_INTERVAL,
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
        .wMaxPacketSize         = USB_CDC_DATA_0_ENDPOINT_SIZE,
        .bInterval              = 0,
    },
    .data_eptx_0 = {
        .bLength                = sizeof(usb_configuration_descriptor.data_eptx_0),
        .bDescriptorType        = usb_descriptor_type_endpoint,
        .bEndpointAddress       = usb_endpoint_direction_in | usb_endpoint_address_cdc_0_data,
        .bmAttributes           = usb_endpoint_type_bulk,
        .wMaxPacketSize         = USB_CDC_DATA_0_ENDPOINT_SIZE,
        .bInterval              = 0,
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
        .iInterface             = usb_string_index_uart_2_interface_name,
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
        .bmCapabilities         = USB_CDC_ACM_CAPABILITIES,
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
        .wMaxPacketSize         = USB_CDC_INTERRUPT_ENDPOINT_SIZE,
        .bInterval              = USB_CDC_INTERRUPT_ENDPOINT_POLLING_INTERVAL,
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
        .wMaxPacketSize         = USB_CDC_DATA_1_ENDPOINT_SIZE,
        .bInterval              = 0,
    },
    .data_eptx_1 = {
        .bLength                = sizeof(usb_configuration_descriptor.data_eptx_1),
        .bDescriptorType        = usb_descriptor_type_endpoint,
        .bEndpointAddress       = usb_endpoint_direction_in | usb_endpoint_address_cdc_1_data,
        .bmAttributes           = usb_endpoint_type_bulk,
        .wMaxPacketSize         = USB_CDC_DATA_1_ENDPOINT_SIZE,
        .bInterval              = 0,
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
        .iInterface             = usb_string_index_uart_3_interface_name,
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
        .bmCapabilities         = USB_CDC_ACM_CAPABILITIES,
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
        .wMaxPacketSize         = USB_CDC_INTERRUPT_ENDPOINT_SIZE,
        .bInterval              = USB_CDC_INTERRUPT_ENDPOINT_POLLING_INTERVAL,
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
        .wMaxPacketSize         = USB_CDC_DATA_2_ENDPOINT_SIZE,
        .bInterval              = 0,
    },
    .data_eptx_2 = {
        .bLength                = sizeof(usb_configuration_descriptor.data_eptx_2),
        .bDescriptorType        = usb_descriptor_type_endpoint,
        .bEndpointAddress       = usb_endpoint_direction_in | usb_endpoint_address_cdc_2_data,
        .bmAttributes           = usb_endpoint_type_bulk,
        .wMaxPacketSize         = USB_CDC_DATA_2_ENDPOINT_SIZE,
        .bInterval              = 0,
    },

};
