/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include <stm32f1xx.h>
#include "system_interrupts.h"
#include "status_led.h"
#include "usb_descriptors.h"
#include "usb_core.h"
#include "usb_panic.h"
#include "usb_io.h"

static volatile usb_btable_entity_t *usb_btable = (usb_btable_entity_t*)USB_PMAADDR;

/* USB Initialization After Reset */

void usb_io_reset() {
    uint16_t offset = USB_BTABLE_SIZE;
    for (uint8_t ep_num=0; ep_num<USB_NUM_ENDPOINTS; ep_num++) {
        ep_reg_t ep_type = 0;
        ep_reg_t *ep_reg = ep_regs(ep_num);
        usb_btable[ep_num].tx_offset = offset;
        usb_btable[ep_num].tx_count = 0;
        offset += usb_endpoints[ep_num].tx_size;
        usb_btable[ep_num].rx_offset = offset;
        if (usb_endpoints[ep_num].rx_size > USB_BTABLE_SMALL_BLOCK_SIZE_LIMIT) {
            usb_btable[ep_num].rx_count = ((usb_endpoints[ep_num].rx_size / USB_BTABLE_LARGE_BLOCK_SIZE) - 1) << USB_COUNT0_RX_NUM_BLOCK_Pos;
            usb_btable[ep_num].rx_count |= USB_COUNT0_RX_BLSIZE;
        } else {
            usb_btable[ep_num].rx_count = (usb_endpoints[ep_num].rx_size / USB_BTABLE_SMALL_BLOCK_SIZE) << USB_COUNT0_RX_NUM_BLOCK_Pos;
        }
        offset += usb_endpoints[ep_num].rx_size;
        switch(usb_endpoints[ep_num].type) {
        case usb_endpoint_type_control:
            ep_type = USB_EP_CONTROL;
            break;
        case usb_endpoint_type_isochronous:
            ep_type = USB_EP_ISOCHRONOUS;
            break;
        case usb_endpoint_type_bulk:
            ep_type = USB_EP_BULK;
            break;
        case usb_endpoint_type_interrupt:
            ep_type = USB_EP_INTERRUPT;
            break;
        }
        *ep_reg = USB_EP_RX_VALID | USB_EP_TX_NAK | ep_type | ep_num;
    }
    USB->CNTR = USB_CNTR_CTRM | USB_CNTR_RESETM | USB_CNTR_SUSPM | USB_CNTR_WKUPM | USB_CNTR_SOFM;
    USB->DADDR = USB_DADDR_EF;
}

void usb_io_init() {
    /* Force USB re-enumeration */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    GPIOA->CRH &= ~GPIO_CRH_CNF12;
    GPIOA->CRH |= GPIO_CRH_MODE12_1;
    for (int i=0; i<0xFFFF; i++) {
        __NOP();
    }
    GPIOA->CRH &= ~GPIO_CRH_MODE12;
    GPIOA->CRH |= GPIO_CRH_CNF12_0;
    /* Initialize USB */
    NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
    if (SystemCoreClock != RCC_MAX_FREQUENCY) {
        RCC->CFGR |= RCC_CFGR_USBPRE;
    }
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
    USB->CNTR = USB_CNTR_FRES;
    USB->BTABLE = 0;
    USB->DADDR = 0;
    USB->ISTR = 0;
    USB->CNTR = USB_CNTR_RESETM;
}

/* Get Number of RX/TX Bytes Available  */
size_t usb_bytes_available(uint8_t ep_num) {
    return usb_btable[ep_num].rx_count & USB_COUNT0_RX_COUNT0_RX;
}

size_t usb_space_available(uint8_t ep_num) {
    ep_reg_t *ep_reg = ep_regs(ep_num);
    size_t tx_space_available = 0;
    if ((*ep_reg & USB_EPTX_STAT) == USB_EP_TX_NAK) {
        tx_space_available = usb_endpoints[ep_num].tx_size;
    }
    return tx_space_available;
}

/* Endpoint Read/Write Operations */

int usb_read(uint8_t ep_num, void *buf, size_t buf_size) {
    ep_reg_t *ep_reg = ep_regs(ep_num);
    usb_pbuffer_data_t *ep_buf = (usb_pbuffer_data_t *)(USB_PMAADDR + (usb_btable[ep_num].rx_offset<<1));
    pb_word_t ep_bytes_count = usb_btable[ep_num].rx_count & USB_COUNT0_RX_COUNT0_RX;
    pb_word_t words_left = ep_bytes_count>>1;
    pb_word_t *buf_p = (pb_word_t*)buf;
    if (ep_bytes_count > buf_size) {
        return -1;
    }
    usb_btable[ep_num].rx_count &= ~USB_COUNT0_RX_COUNT0_RX;
    while(words_left--) {
         *buf_p++ = (ep_buf++)->data;
    }
    if (ep_bytes_count & 0x01) {
        *((uint8_t*)buf_p) = (uint8_t)ep_buf->data;
    }
    *ep_reg = ((*ep_reg ^ USB_EP_RX_VALID) & (USB_EPREG_MASK | USB_EPRX_STAT)) | (USB_EP_CTR_RX | USB_EP_CTR_TX);
    return ep_bytes_count;
}

size_t usb_send(uint8_t ep_num, const void *buf, size_t count) {
    ep_reg_t *ep_reg = ep_regs(ep_num);
    usb_pbuffer_data_t *ep_buf = (usb_pbuffer_data_t *)(USB_PMAADDR + (usb_btable[ep_num].tx_offset<<1));
    pb_word_t *buf_p = (pb_word_t*)buf;
    pb_word_t words_left  = count >> 1;
    size_t tx_space_available = usb_endpoints[ep_num].tx_size;
    if (count > tx_space_available) {
        count = tx_space_available;
    }
    while (words_left--) {
        (ep_buf++)->data = *buf_p++;
    }
    if (count & 0x01) {
        (ep_buf)->data = (uint8_t)*buf_p;
    }
    usb_btable[ep_num].tx_count = count;
    *ep_reg = ((*ep_reg ^ USB_EP_TX_VALID) & (USB_EPREG_MASK | USB_EPTX_STAT)) | (USB_EP_CTR_RX | USB_EP_CTR_TX);
    return count;
}

/* Circular Buffer Read/Write Operations */

/* NOTE: usb_circ_buf_read assumes enough buffer space is available */
size_t usb_circ_buf_read(uint8_t ep_num, circ_buf_t *buf, size_t buf_size) {
    ep_reg_t *ep_reg = ep_regs(ep_num);
    usb_pbuffer_data_t *ep_buf = (usb_pbuffer_data_t *)(USB_PMAADDR + (usb_btable[ep_num].rx_offset<<1));
    pb_word_t ep_bytes_count = usb_btable[ep_num].rx_count & USB_COUNT0_RX_COUNT0_RX;
    size_t words_left = ep_bytes_count>>1;
    usb_btable[ep_num].rx_count &= ~USB_COUNT0_RX_COUNT0_RX;
    while(words_left--) {
        buf->data[buf->head] = (uint8_t)(ep_buf->data);
        buf->head = (buf->head + 1) & (buf_size - 1);
        buf->data[buf->head] = (uint8_t)(((ep_buf++)->data) >> 8);
        buf->head = (buf->head + 1) & (buf_size - 1);
    }
    if (ep_bytes_count & 0x1) {
        buf->data[buf->head] = (uint8_t)(ep_buf->data);
        buf->head = (buf->head + 1) & (buf_size - 1);
    }
    *ep_reg = ((*ep_reg ^ USB_EP_RX_VALID) & (USB_EPREG_MASK | USB_EPRX_STAT)) | (USB_EP_CTR_RX | USB_EP_CTR_TX);
    return ep_bytes_count;
}

/* NOTE: usb_circ_buf_send assumes endpoint is ready to send */
size_t usb_circ_buf_send(uint8_t ep_num, circ_buf_t *buf, size_t buf_size) {
    ep_reg_t *ep_reg = ep_regs(ep_num);
    usb_pbuffer_data_t *ep_buf = (usb_pbuffer_data_t *)(USB_PMAADDR + (usb_btable[ep_num].tx_offset<<1));
    size_t count = circ_buf_count(buf->head, buf->tail, buf_size);
    size_t tx_space_available = usb_endpoints[ep_num].tx_size;
    size_t words_left;
    if (count > tx_space_available) {
        count = tx_space_available;
    }
    words_left = count >> 1;
    while (words_left--) {
        pb_word_t pb_word = buf->data[buf->tail];
        buf->tail = (buf->tail + 1) & (buf_size - 1);
        pb_word |= ((uint16_t)buf->data[buf->tail]) << 8;
        buf->tail = (buf->tail + 1) & (buf_size - 1);
        (ep_buf++)->data = pb_word;
    }
    if (count & 0x1) {
        (ep_buf)->data = buf->data[buf->tail];
        buf->tail = (buf->tail + 1) & (buf_size - 1);
    }
    usb_btable[ep_num].tx_count = count;
    *ep_reg = ((*ep_reg ^ USB_EP_TX_VALID) & (USB_EPREG_MASK | USB_EPTX_STAT)) | (USB_EP_CTR_RX | USB_EP_CTR_TX);
    return count;
}

/* Endpoint Stall */

void usb_endpoint_set_stall(uint8_t ep_num, usb_endpoint_direction_t ep_direction, uint8_t ep_stall) {
    ep_reg_t *ep_reg = ep_regs(ep_num);
    if ((*ep_reg & USB_EP_T_FIELD) != USB_EP_ISOCHRONOUS) {
        if (ep_direction == usb_endpoint_direction_in) {
            if ((*ep_reg & USB_EPTX_STAT) != USB_EP_TX_DIS) {
                if (ep_stall) {
                    *ep_reg = ((*ep_reg ^ USB_EP_TX_STALL) & (USB_EPREG_MASK | USB_EPTX_STAT)) | (USB_EP_CTR_RX | USB_EP_CTR_TX);
                } else {
                    *ep_reg = ((*ep_reg ^ USB_EP_TX_NAK) & (USB_EPREG_MASK | USB_EPTX_STAT | USB_EP_DTOG_TX)) | (USB_EP_CTR_RX | USB_EP_CTR_TX);
                }
            }
        } else {
            if ((*ep_reg & USB_EPRX_STAT) != USB_EP_RX_DIS) {
                if (ep_stall) {
                    *ep_reg = ((*ep_reg ^ USB_EP_RX_STALL) & (USB_EPREG_MASK | USB_EPRX_STAT)) | (USB_EP_CTR_RX | USB_EP_CTR_TX);
                } else {
                    *ep_reg = ((*ep_reg ^ USB_EP_TX_VALID) & (USB_EPREG_MASK | USB_EPRX_STAT | USB_EP_DTOG_RX)) | (USB_EP_CTR_RX | USB_EP_CTR_TX);
                }
            }
        }
    }
}

int usb_endpoint_is_stalled(uint8_t ep_num, usb_endpoint_direction_t ep_direction) {
    if (ep_direction == usb_endpoint_direction_in) {
        return (*ep_regs(ep_num) & USB_EPTX_STAT) == USB_EP_TX_STALL;
    }
    return (*ep_regs(ep_num) & USB_EPRX_STAT) == USB_EP_RX_STALL;
}

/* USB Polling */

static uint8_t usb_transfer_led_timer = 0;

uint16_t istr;

void usb_poll() {
    istr = USB->ISTR;
    if (istr & USB_ISTR_CTR) {
        uint8_t ep_num = USB->ISTR & USB_ISTR_EP_ID;
        ep_reg_t *ep_reg = ep_regs(ep_num);
        if (*ep_reg & USB_EP_CTR_TX) {
            *ep_reg = ((*ep_reg & (USB_EP_T_FIELD | USB_EP_KIND | USB_EPADDR_FIELD)) | USB_EP_CTR_RX);
            if (usb_endpoints[ep_num].event_handler) {
                usb_endpoints[ep_num].event_handler(ep_num, usb_endpoint_event_data_sent);
            }
        } else {
            usb_endpoint_event_t ep_event = usb_endpoint_event_data_received;
            if (*ep_reg & USB_EP_SETUP) {
                ep_event = usb_endpoint_event_setup;
            }
            *ep_reg = ((*ep_reg & (USB_EP_T_FIELD | USB_EP_KIND | USB_EPADDR_FIELD)) | USB_EP_CTR_TX);
            if (usb_endpoints[ep_num].event_handler) {
                usb_endpoints[ep_num].event_handler(ep_num, ep_event);
            }
        }
        usb_transfer_led_timer = USB_TRANSFER_LED_TIME;
        status_led_set(1);
    } else if (istr & USB_ISTR_RESET) {
        USB->ISTR = (uint16_t)(~USB_ISTR_RESET);
        usb_device_handle_reset();   
    } else if (istr & USB_ISTR_SUSP) {
        USB->ISTR = (uint16_t)(~USB_ISTR_SUSP);
        USB->CNTR |= USB_CNTR_FSUSP;
        status_led_set(0);
        usb_device_handle_suspend();
    } else if (istr & USB_ISTR_WKUP) {
        USB->ISTR = (uint16_t)(~USB_ISTR_WKUP);
        USB->CNTR &= ~USB_CNTR_FSUSP;
        usb_device_handle_wakeup();
    } else if (istr & USB_ISTR_SOF) {
        USB->ISTR = (uint16_t)(~USB_ISTR_SOF);
        if (usb_transfer_led_timer) {
            status_led_set(--usb_transfer_led_timer);
        }
        usb_device_handle_frame();
    }
    usb_device_poll();
}
