/*
 * MIT License
 *
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include <stddef.h>
#include <string.h>
#include "status_led.h"
#include "usb_core.h"
#include "usb_std.h"
#include "usb_cdc.h"
#include "usb_descriptors.h"
#include "usb_io.h"
#include "usb_uid.h"
#include "usb_panic.h"
#include "usb.h"

/* Device Level Events */

static struct {
    usb_device_state_t state;
    uint8_t address;
    uint8_t configuration;
} usb_device;

void usb_device_handle_reset() {
    usb_device.state = usb_device_state_reset;
    usb_device.address = 0;
    usb_device.configuration = 0;
    usb_cdc_reset();
    usb_io_reset();
}

void usb_device_handle_configured() {
    if (usb_device.state != usb_device_state_configured) {
        usb_cdc_enable();
    }
}

void usb_device_handle_suspend() {
    if (usb_device.state == usb_device_state_configured) {
        usb_cdc_suspend();
    }
    USB->DADDR = USB_DADDR_EF;
    usb_device.state = usb_device_state_reset;
}

void usb_device_handle_wakeup() {

}

void usb_device_handle_frame() {
    usb_cdc_frame();
}

void usb_device_poll() {
    usb_cdc_poll();
}

/* Device Descriptor Requests Handling */

usb_status_t usb_control_endpoint_process_get_descriptor(usb_setup_t *setup,
                                            void **payload, size_t *payload_size, usb_tx_complete_cb_t *tx_callback_ptr) {
    (void) tx_callback_ptr;
    usb_descriptor_type_t descriptor_type = (setup->wValue >> 8);
    uint8_t descriptor_index = (setup->wValue & 0xff);
    switch (descriptor_type) {
    case usb_descriptor_type_device:
        *payload = (void*)&usb_device_descriptor;
        *payload_size = usb_device_descriptor.bLength;
        break;
    case usb_descriptor_type_configuration:
        *payload = (void*)&usb_configuration_descriptor;
        *payload_size = usb_configuration_descriptor.config.wTotalLength;
        break;
    case usb_descriptor_type_string:
        if (descriptor_index == usb_string_index_serial) {
            usb_string_descriptor_t *uid_descriptor = usb_get_uid_string_descriptor();
            *payload = uid_descriptor;
            *payload_size = uid_descriptor->bLength;
        } else {
            if (descriptor_index < usb_string_index_last) {
                *payload = (void*)usb_string_descriptors[descriptor_index];
                *payload_size = usb_string_descriptors[descriptor_index]->bLength;
            } else {
                return usb_status_fail;
            }
        }
        break;
    default:
        return usb_status_fail;
    }
    return usb_status_ack;
}

/* Standard Control Endpoint Requests Handling */

static void usb_assign_device_address_cb() {
    USB->DADDR = USB_DADDR_EF | usb_device.address;
    usb_device.state = usb_device_state_address_set;
}

usb_status_t usb_control_endpoint_process_device_request(usb_setup_t *setup,
                                            void **payload, size_t *payload_size, usb_tx_complete_cb_t *tx_callback_ptr) {
    switch(setup->bRequest) {
    case usb_device_request_get_configuration:
        *(uint8_t*)payload[0] = usb_device.configuration;
        return usb_status_ack;
    case usb_device_request_get_descriptor:
        return usb_control_endpoint_process_get_descriptor(setup, payload, payload_size, tx_callback_ptr);
    case usb_device_request_get_status:
        ((uint8_t*)(*payload))[0] = 0;
        ((uint8_t*)(*payload))[1] = 0;
        return usb_status_ack;
    case usb_device_request_set_address:
        usb_device.address = setup->wValue & USB_DADDR_ADD_Msk;
        *tx_callback_ptr = usb_assign_device_address_cb;
        return usb_status_ack;
    case usb_device_request_set_configuration: {
        uint8_t device_configuration = setup->wValue & 0xff;
        if (device_configuration == 1) {
            usb_device.configuration = device_configuration;
            usb_device_handle_configured();
            return usb_status_ack;
        }
        break;
    }
    case usb_device_request_set_descriptor:
    case usb_device_request_set_feature:
    case usb_device_request_clear_feature:
        break;
    default:
        break;
    }
    return usb_status_fail;
}

usb_status_t usb_control_endpoint_process_interface_request(usb_setup_t *setup,
                                            void **payload, size_t *payload_size, usb_tx_complete_cb_t *tx_callback_ptr) {
    (void) payload_size;
    (void) tx_callback_ptr;
    if (setup->bRequest == usb_device_request_get_status) {
        ((uint8_t*)(*payload))[0] = 0;
        ((uint8_t*)(*payload))[1] = 0;
        return usb_status_ack;
    }
    return usb_status_fail;
}


usb_status_t usb_control_endpoint_process_endpoint_request(usb_setup_t *setup,
                                            void **payload, size_t *payload_size, usb_tx_complete_cb_t *tx_callback_ptr) {

    (void) payload_size;
    (void) tx_callback_ptr;
    uint8_t ep_num = setup->wIndex & ~(usb_endpoint_direction_in);
    usb_endpoint_direction_t ep_direction = setup->wIndex & usb_endpoint_direction_in;
    if ((setup->bRequest == usb_device_request_set_feature) ||
        (setup->bRequest == usb_device_request_clear_feature)) {
        uint8_t ep_stall = (setup->bRequest == usb_device_request_set_feature);
        usb_endpoint_set_stall(ep_num, ep_direction, ep_stall);
        return usb_status_ack;
    } else if (setup->bRequest == usb_device_request_get_status) {
        ((uint8_t*)(*payload))[0] = usb_endpoint_is_stalled(ep_num , ep_direction);
        ((uint8_t*)(*payload))[1] = 0;
        return usb_status_ack;
    } else {
        return usb_status_fail;
    }
}

/* Control Endpoint Request Processing */

usb_status_t usb_control_endpoint_process_request(usb_setup_t *setup,
                                            void **payload, size_t *payload_size, usb_tx_complete_cb_t *tx_callback_ptr) {
    usb_status_t status = usb_cdc_ctrl_process_request(setup, payload, payload_size, tx_callback_ptr);
    if (status != usb_status_fail) {
        return status;
    }

    if (setup->type == usb_setup_type_standard) {
        switch (setup->recepient) {
        case usb_setup_recepient_device:
            return usb_control_endpoint_process_device_request(setup, payload, payload_size, tx_callback_ptr);
        case usb_setup_recepient_interface:
            return usb_control_endpoint_process_interface_request(setup, payload, payload_size, tx_callback_ptr);
        case usb_setup_recepient_endpoint:
            return usb_control_endpoint_process_endpoint_request(setup, payload, payload_size, tx_callback_ptr);
        default:
            break;
        }
    }
    return usb_status_fail;
}

/* Control Endpoint Data Echange */

static struct {
    enum {
        usb_control_state_idle,
        usb_control_state_rx,
        usb_control_state_tx,
        usb_control_state_tx_zlp,
        usb_control_state_tx_last,
        usb_control_state_status_in,
        usb_control_state_status_out,
    } state;
    uint8_t setup_buf[USB_SETUP_MAX_SIZE];
    void *payload;
    size_t payload_size;
    usb_setup_t *setup;
    usb_tx_complete_cb_t tx_complete_callback;
} usb_control_ep_struct;

void usb_control_endpoint_stall(uint8_t ep_num) {
    usb_endpoint_set_stall(ep_num, usb_endpoint_direction_out, 1);
    usb_endpoint_set_stall(ep_num, usb_endpoint_direction_in, 1);
    usb_control_ep_struct.state = usb_control_state_idle;
}

static void usb_control_endpoint_process_callback() {
    if (usb_control_ep_struct.tx_complete_callback) {
        usb_control_ep_struct.tx_complete_callback();
        usb_control_ep_struct.tx_complete_callback = 0;
    }
}

static void usb_control_endpoint_process_tx(uint8_t ep_num) {
    size_t bytes_sent = 0;
    switch (usb_control_ep_struct.state) {
    case usb_control_state_tx:
    case usb_control_state_tx_zlp:
        bytes_sent = usb_send(ep_num, usb_control_ep_struct.payload, usb_control_ep_struct.payload_size);
        usb_control_ep_struct.payload = ((uint8_t*)usb_control_ep_struct.payload) + bytes_sent;
        usb_control_ep_struct.payload_size -= bytes_sent;
        if (usb_control_ep_struct.payload_size == 0) {
            if ((usb_control_ep_struct.state == usb_control_state_tx) ||
                (bytes_sent != usb_endpoints[ep_num].tx_size)) {
                usb_control_ep_struct.state = usb_control_state_tx_last;
            }
        }
        break;
    case usb_control_state_tx_last:
        usb_control_ep_struct.state = usb_control_state_status_out;
        break;
    case usb_control_state_status_in:
        usb_control_ep_struct.state = usb_control_state_idle;
        usb_control_endpoint_process_callback();
        break;
    default:
        usb_panic();
        break;
    }
}

static void usb_control_endpoint_process_rx(uint8_t ep_num) {
    size_t setup_size;
    size_t payload_bytes_received = 0;
    switch (usb_control_ep_struct.state) {
    case usb_control_state_idle:
        setup_size = usb_read(ep_num, usb_control_ep_struct.setup_buf, sizeof(usb_setup_t));
        if (setup_size != sizeof(usb_setup_t)) {
            usb_control_endpoint_stall(ep_num);
            return;
        } else {
            usb_control_ep_struct.setup = (usb_setup_t *)&usb_control_ep_struct.setup_buf;
            usb_control_ep_struct.payload = usb_control_ep_struct.setup->payload;
            usb_control_ep_struct.payload_size = usb_control_ep_struct.setup->wLength;
            if ((usb_control_ep_struct.setup->direction == usb_setup_direction_host_to_device) &&
                    (usb_control_ep_struct.setup->wLength != 0)) {
                if (usb_control_ep_struct.payload_size > USB_SETUP_MAX_PAYLOAD_SIZE) {
                    usb_control_endpoint_stall(ep_num);
                } else {
                    usb_control_ep_struct.state = usb_control_state_rx;
                }
                return;
            }
        }
        break;
    case usb_control_state_rx:
        payload_bytes_received = usb_read(ep_num, usb_control_ep_struct.payload, usb_control_ep_struct.payload_size);
        if (usb_control_ep_struct.payload_size != payload_bytes_received) {
            usb_control_ep_struct.payload = ((uint8_t*)usb_control_ep_struct.payload) + payload_bytes_received;
            usb_control_ep_struct.payload_size -= payload_bytes_received;
            return;
        }
        break;
    case usb_control_state_status_out:
        usb_read(ep_num, 0, 0);
        usb_control_ep_struct.state = usb_control_state_idle;
        usb_control_endpoint_process_callback();
        return;
    default:
        usb_control_endpoint_stall(ep_num);
        return;
    }
    usb_control_ep_struct.payload = usb_control_ep_struct.setup->payload;
    usb_control_ep_struct.payload_size = USB_SETUP_MAX_PAYLOAD_SIZE;
    switch (usb_control_endpoint_process_request(usb_control_ep_struct.setup,
                                                 &usb_control_ep_struct.payload,
                                                 &usb_control_ep_struct.payload_size,
                                                 &usb_control_ep_struct.tx_complete_callback)) {
    case usb_status_ack:
        if (usb_control_ep_struct.setup->direction == usb_setup_direction_device_to_host) {
            if (usb_control_ep_struct.payload_size < usb_control_ep_struct.setup->wLength) {
                usb_control_ep_struct.state = usb_control_state_tx_zlp;
            } else {
                if (usb_control_ep_struct.payload_size > usb_control_ep_struct.setup->wLength) {
                    usb_control_ep_struct.payload_size = usb_control_ep_struct.setup->wLength;
                }
                usb_control_ep_struct.state = usb_control_state_tx;
            }
            usb_control_endpoint_process_tx(ep_num);
            return;
        } else {
            usb_send(ep_num, 0, 0);
            usb_control_ep_struct.state = usb_control_state_status_in;
        }
        break;
    case usb_status_nak:
        usb_control_ep_struct.state = usb_control_state_status_in;
        break;
    default:
        usb_control_endpoint_stall(ep_num);
    }
}

void usb_control_endpoint_event_handler(uint8_t ep_num, usb_endpoint_event_t ep_event) {
    if (ep_num == usb_endpoint_address_control) {
        if (ep_event == usb_endpoint_event_setup || ep_event == usb_endpoint_event_data_received) {
            if (ep_event == usb_endpoint_event_setup) {
                usb_control_ep_struct.state = usb_control_state_idle;
            }
            usb_control_endpoint_process_rx(ep_num);
        } else if (ep_event == usb_endpoint_event_data_sent) {
            usb_control_endpoint_process_tx(ep_num);
        } else {
            usb_panic();
        }
    } else {
        usb_panic();
    }
}

void usb_init() {
    usb_io_init();
}
