/*
 * MIT License
 *
 * Copyright (c) 2020 Kirill Kotyagin
 */

#ifndef USB_STD_H
#define USB_STD_H

#include <stdint.h>

#include "aux.h"

/* USB Descriptor Types */

typedef enum {
    usb_descriptor_type_device          = 0x01,
    usb_descriptor_type_configuration   = 0x02,
    usb_descriptor_type_string          = 0x03,
    usb_descriptor_type_interface       = 0x04,
    usb_descriptor_type_endpoint        = 0x05,
    usb_descriptor_type_qualifier       = 0x06,
    usb_descriptor_type_other           = 0x07,
    usb_descriptor_type_interface_power = 0x08,
    usb_descriptor_type_otg             = 0x09,
    usb_descriptor_type_debug           = 0x0a,
    usb_descriptor_type_interface_assoc = 0x0b,
    usb_descriptor_type_cs_interface    = 0x24,
    usb_descriptor_type_cs_endpoint     = 0x25,
} __packed usb_descriptor_type_t;

/* USB Device Descriptor */

typedef enum {
    usb_device_class_device                 = 0x00,
    usb_device_class_audio                  = 0x01,
    usb_device_class_comm                   = 0x02,
    usb_device_class_hid                    = 0x03,
    usb_device_class_physical               = 0x05,
    usb_device_class_image                  = 0x06,
    usb_device_class_printer                = 0x07,
    usb_device_class_mass_storage           = 0x08,
    usb_device_class_hub                    = 0x09,
    usb_device_class_cdc_data               = 0x0a,
    usb_device_class_smart_card             = 0x0b,
    usb_device_class_content_security       = 0x0d,
    usb_device_class_video                  = 0x0e,
    usb_device_class_personal_healthcare    = 0x0f,
    usb_device_class_audio_video            = 0x10,
    usb_device_class_billboard              = 0x11,
    usb_device_class_usbc_bridge            = 0x12,
    usb_device_class_diagnostic             = 0xdc,
    usb_device_class_wireless_controller    = 0xe0,
    usb_device_class_misc                   = 0xef,
    usb_device_class_application_specific   = 0xfe,
    usb_device_class_vendor_specific        = 0xff,
} __packed usb_device_class_t;

typedef enum {
    usb_device_subclass_none    = 0x00,
    usb_device_subclass_iad     = 0x02,
} __packed usb_device_subclass_t;

typedef enum {
    usb_device_protocol_none    = 0x00,
    usb_device_protocol_iad     = 0x01,
} __packed usb_device_protocol_t;

#define USB_BCD_VERSION(maj, min, rev)  (((maj & 0xFF) << 8) | ((min & 0x0F) << 4) | (rev & 0x0F))

typedef struct {
    uint8_t     bLength;
    uint8_t     bDescriptorType;
    uint16_t    bcdUSB;
    uint8_t     bDeviceClass;
    uint8_t     bDeviceSubClass;
    uint8_t     bDeviceProtocol;
    uint8_t     bMaxPacketSize;
    uint16_t    idVendor;
    uint16_t    idProduct;
    uint16_t    bcdDevice;
    uint8_t     iManufacturer;
    uint8_t     iProduct;
    uint8_t     iSerialNumber;
    uint8_t     bNumConfigurations;
} __packed usb_device_descriptor_t;

/* USB Qualifier Descriptor */

typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint8_t  bNumConfigurations;
    uint8_t  bReserved;
} __packed usb_qualifier_descriptor_t;

/* USB Configuration Descriptor */

#define USB_CFG_ATTR_REMOTE_WAKEUP   0x20
#define USB_CFG_ATTRI_SELF_POWERED   0x40
#define USB_CFG_ATTR_RESERVED        0x80

#define USB_CFG_POWER_MA(mA)        ((mA) >> 1)

typedef struct  {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
} __packed usb_configuration_descriptor_t;

/* USB Interface Descriptor */

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __packed usb_interface_descriptor_t;

/* USB Interface Association Descriptor */

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bFirstInterface;
    uint8_t bInterfaceCount;
    uint8_t bFunctionClass;
    uint8_t bFunctionSubClass;
    uint8_t bFunctionProtocol;
    uint8_t iFunction;
} __packed usb_iad_descriptor_t;

/* USB Endpoint Descriptor */

typedef enum {
    usb_endpoint_direction_out  = 0x00,
    usb_endpoint_direction_in   = 0x80,
} __packed usb_endpoint_direction_t;

typedef enum {
    usb_endpoint_type_control       = 0x00,
    usb_endpoint_type_isochronous   = 0x01,
    usb_endpoint_type_bulk          = 0x02,
    usb_endpoint_type_interrupt     = 0x03,
} __packed usb_endpoint_type_t;

typedef enum {
    usb_endpoint_attribute_no_sync  = 0x00,
    usb_endpoint_attribute_async    = 0x04,
    usb_endpoint_attribute_adaptive = 0x08,
    usb_endpoint_attribute_sync     = 0x0c
} __packed usb_endpoint_attribute_t;


typedef enum {
    usb_endpoint_usage_data                 = 0x00,
    usb_endpoint_usage_feedback             = 0x10,
    usb_endpoint_usage_implicit_feedback    = 0x20
} __packed usb_endpoint_usage_t;

typedef struct  {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} __packed usb_endpoint_descriptor_t;

/* USB String Descriptor */

typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wString[];
} __spacked usb_string_descriptor_t;

#define __CAT(x,y) x ## y
#define CAT(x,y) __CAT(x,y)

#define USB_ARRAY_DESC(...) {.bLength = 2 + sizeof((uint16_t[]){__VA_ARGS__}),\
                            .bDescriptorType = usb_descriptor_type_string,\
                            .wString = {__VA_ARGS__}}

#define USB_STRING_DESC(s)  {.bLength = sizeof(CAT(u,s)),\
                            .bDescriptorType = usb_descriptor_type_string,\
                            .wString = {CAT(u,s)}}

typedef enum {
    usb_language_code_en_US = 0x409,
} __packed usb_language_code_t;

/* USB Debug Descriptor */

typedef struct  {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bDebugInEndpoint;
    uint8_t  bDebugOutEndpoint;
} __packed usb_debug_descriptor_t;


/* USB Control Endpoint Related Definitions */

typedef enum {
    usb_setup_direction_host_to_device = 0,
    usb_setup_direction_device_to_host = 1
} usb_setup_direction_t;

typedef enum {
    usb_setup_type_standard    = 0,
    usb_setup_type_class       = 1,
    usb_setup_type_vendor      = 2,
    usb_setup_type_reserved    = 3,
} usb_setup_type_t;

typedef enum {
    usb_setup_recepient_device      = 0,
    usb_setup_recepient_interface   = 1,
    usb_setup_recepient_endpoint    = 2,
    usb_setup_recepient_other       = 3,
} usb_setup_recepient_t;

typedef struct {
    union {
        uint8_t bmRequestType;
        struct {
            usb_setup_recepient_t recepient:5;
            usb_setup_type_t type:2;
            usb_setup_direction_t direction:1;
        };
    };
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
    uint8_t payload[0];
} __packed usb_setup_t;

typedef enum {
    usb_device_request_get_status          = 0x00,
    usb_device_request_clear_feature       = 0x01,
    usb_device_request_set_feature         = 0x03,
    usb_device_request_set_address         = 0x05,
    usb_device_request_get_descriptor      = 0x06,
    usb_device_request_set_descriptor      = 0x07,
    usb_device_request_get_configuration   = 0x08,
    usb_device_request_set_configuration   = 0x09,
} usb_device_request_t;

#endif /* USB_STD_H */
