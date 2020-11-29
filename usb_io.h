/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#ifndef USB_IO
#define USB_IO

/* Buffer table */

#include <stddef.h>
#include <stm32f1xx.h>
#include "circ_buf.h"
#include "usb.h"
#include "usb_std.h"
#include "usb_core.h"

#define USB_TRANSFER_LED_TIME       20 /* usb frames, 1 ms each at full speed */

#define USB_PACKET_BUFFER_ALIGNMENT 4

typedef uint16_t    pb_word_t;
typedef pb_word_t   pb_aligned_word_t __attribute__ ((aligned(USB_PACKET_BUFFER_ALIGNMENT)));

typedef struct {
    pb_aligned_word_t tx_offset;
    pb_aligned_word_t tx_count;
    pb_aligned_word_t rx_offset;
    pb_aligned_word_t rx_count;
} usb_btable_entity_t;

typedef struct {
    pb_aligned_word_t data;
} usb_pbuffer_data_t;

#define USB_BTABLE_SIZE (sizeof(usb_btable_entity_t) * USB_NUM_ENDPOINTS)
#define USB_BTABLE_SMALL_BLOCK_SIZE (sizeof(uint16_t))
#define USB_BTABLE_LARGE_BLOCK_SIZE (USB_BTABLE_SMALL_BLOCK_SIZE<<4)
#define USB_BTABLE_SMALL_BLOCK_SIZE_LIMIT ((USB_COUNT0_RX_NUM_BLOCK>>USB_COUNT0_RX_NUM_BLOCK_Pos)<<1)

/* Endpoint Control Registers */

typedef volatile uint16_t ep_reg_t;
#define ep_regs(ep_num) (((ep_reg_t*)USB_BASE) + (ep_num << 1))

/* USB Endpoint I/O Events */

typedef enum {
    usb_endpoint_event_data_received    = 0x01,
    usb_endpoint_event_data_sent        = 0x02,
    usb_endpoint_event_setup            = 0x03,
} usb_endpoint_event_t;

/* USB Endpoint Definition */

typedef void (*usb_endpoint_event_handler_t)(uint8_t ep_num, usb_endpoint_event_t ep_event);

typedef struct {
    uint32_t    type;
    uint8_t     rx_size;
    uint8_t     tx_size;
    usb_endpoint_event_handler_t event_handler;
} usb_endpoint_t;

#define USB_NUM_ENDPOINTS (sizeof(usb_endpoints)/sizeof(*usb_endpoints))
#define USB_MAX_ENDPOINTS 8

/* USB IO Initialization */

void usb_io_init();
void usb_io_reset();

/* Get Number of RX/TX Bytes Available  */

size_t usb_bytes_available(uint8_t ep_num);
size_t usb_space_available(uint8_t ep_num);

/* Endpoint Read/Write Operations */

int usb_read(uint8_t ep_num, void *buf, size_t buf_size);
size_t usb_send(uint8_t ep_num, const void *buf, size_t count);

/* Circular Buffer Read/Write Operations */

/* NOTE: usb_circ_buf_read assumes enough buffer space is available */
size_t usb_circ_buf_read(uint8_t ep_num, circ_buf_t *buf, size_t buf_size);
/* NOTE: usb_circ_buf_send assumes endpoint is ready to send */
size_t usb_circ_buf_send(uint8_t ep_num, circ_buf_t *buf, size_t buf_size);

/* Endpoint Stall */

void usb_endpoint_set_stall(uint8_t ep_num, usb_endpoint_direction_t ep_direction, uint8_t ep_stall);
int usb_endpoint_is_stalled(uint8_t ep_num, usb_endpoint_direction_t ep_direction);

#endif /* USB_IO */
