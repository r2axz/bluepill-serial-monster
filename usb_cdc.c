/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include <string.h>
#include <stm32f1xx.h>
#include "system_interrupts.h"
#include "circ_buf.h"
#include "usb_std.h"
#include "usb_core.h"
#include "usb_descriptors.h"
#include "usb_panic.h"
#include "cdc_shell.h"
#include "device_config.h"
#include "gpio.h"
#include "usb_cdc.h"

/* USB CDC Device Enabled Flag */

static uint8_t usb_cdc_enabled = 0;
static uint8_t usb_cdc_config_mode = 0;

/* USB CDC State Struct */

static const usb_cdc_line_coding_t usb_cdc_default_line_coding = {
    .dwDTERate      = 9600,
    .bCharFormat    = usb_cdc_char_format_1_stop_bit, 
    .bParityType    = usb_cdc_parity_type_none,
    .bDataBits      = 8,
};

typedef struct {
    circ_buf_t              rx_buf;
    uint8_t                 _rx_data[USB_CDC_BUF_SIZE];
    circ_buf_t              tx_buf;
    uint8_t                 _tx_data[USB_CDC_BUF_SIZE];
    usb_cdc_line_coding_t   line_coding;
    uint8_t                 usb_rx_pending_ep;
    size_t                  last_dma_tx_size;
    uint8_t                 rx_zlp_pending;
    uint8_t                 line_state_change_pending;
    uint8_t                 line_state_change_ready;
    usb_cdc_serial_state_t  serial_state;
    usb_cdc_serial_state_t  serial_state_prev;
    uint8_t                 rts_active;
    uint8_t                 dtr_active;
    uint8_t                 txa_active;
} usb_cdc_state_t;

static usb_cdc_state_t usb_cdc_states[USB_CDC_NUM_PORTS];

/* Helper Functions */

static USART_TypeDef* usb_cdc_get_port_usart(int port) {
    static USART_TypeDef* const port_usarts[] = {
        USART1, USART2, USART3
    };
    if (port < (sizeof(port_usarts) / sizeof(*port_usarts))){
        return port_usarts[port];
    }
    return (USART_TypeDef*)0;
}

typedef enum {
    usb_cdc_port_direction_rx = 0x00,
    usb_cdc_port_direction_tx = 0x01,
    usb_cdc_port_direction_last
} usb_cdc_port_direction_t;

static DMA_Channel_TypeDef* usb_cdc_get_port_dma_channel(int port, usb_cdc_port_direction_t port_dir) {
    static DMA_Channel_TypeDef* const port_dma_channels[][usb_cdc_port_direction_last] = {
        { DMA1_Channel5,  DMA1_Channel4 },
        { DMA1_Channel6,  DMA1_Channel7 },
        { DMA1_Channel3,  DMA1_Channel2 },
    };
    if (port < (sizeof(port_dma_channels) / sizeof(*port_dma_channels)) && 
        port_dir < usb_cdc_port_direction_last) {
        return port_dma_channels[port][port_dir];
    }
    return (DMA_Channel_TypeDef*)0; 
}

static uint8_t const usb_cdc_port_data_endpoints[] = {
    usb_endpoint_address_cdc_0_data,
    usb_endpoint_address_cdc_1_data,
    usb_endpoint_address_cdc_2_data,
};

static uint8_t usb_cdc_get_port_data_ep(int port) {
    if (port < (sizeof(usb_cdc_port_data_endpoints) / sizeof(*usb_cdc_port_data_endpoints))) {
        return usb_cdc_port_data_endpoints[port];
    }
    return -1;
}

static int usb_cdc_data_endpoint_port(uint8_t ep_num) {
    for (int port = 0; port < (sizeof(usb_cdc_port_data_endpoints) / sizeof(*usb_cdc_port_data_endpoints)); port++) {
        if (usb_cdc_port_data_endpoints[port] == ep_num) {
            return port;
        }
    }
    return -1;
}

static uint8_t const usb_cdc_port_interrupt_endpoints[] = {
    usb_endpoint_address_cdc_0_interrupt,
    usb_endpoint_address_cdc_1_interrupt,
    usb_endpoint_address_cdc_2_interrupt,
};

static uint8_t usb_cdc_get_port_notification_ep(int port) {
    if (port < (sizeof(usb_cdc_port_interrupt_endpoints) / sizeof(*usb_cdc_port_interrupt_endpoints))) {
        return usb_cdc_port_interrupt_endpoints[port];
    }
    return -1;
}

static uint8_t const usb_cdc_port_interfaces[] = {
    usb_interface_cdc_0, usb_interface_cdc_1, usb_interface_cdc_2
};

static int usb_cdc_get_port_interface(int port) {
    if (port < (sizeof(usb_cdc_port_interfaces) / sizeof(*usb_cdc_port_interfaces))) {
        return usb_cdc_port_interfaces[port];
    }
    return -1;
}

static int usb_cdc_get_interface_port(uint8_t if_num) {
    for (int port = 0; port < (sizeof(usb_cdc_port_interfaces) / sizeof(*usb_cdc_port_interfaces)); port++) {
        if (usb_cdc_port_interfaces[port] == if_num) {
            return port;
        }
    }
    return -1;
}

static uint32_t usb_cdc_get_port_fck(int port) {
    if (port == 0) {
        return SystemCoreClock;
    }
    return SystemCoreClock >> 1;
}

/* USB CDC Notifications */

static int usb_cdc_send_port_state(int port, usb_cdc_serial_state_t state) {
    uint8_t ep_num = usb_cdc_get_port_notification_ep(port);
    uint8_t buf[sizeof(usb_cdc_notification_t) + sizeof(state)];
    usb_cdc_notification_t *notification = (usb_cdc_notification_t*)buf;
    uint8_t *state_p = buf + sizeof(usb_cdc_notification_t);
    notification->bmRequestType = USB_CDC_NOTIFICATION_REQUEST_TYPE;
    notification->bNotificationType = usb_cdc_notification_serial_state;
    notification->wValue = 0;
    notification->wIndex = usb_cdc_get_port_interface(port);
    notification->wLength = sizeof(state);
    *state_p++ = state & 0xFF;
    *state_p = state >> 8;
    if (usb_space_available(ep_num)) {
        if (usb_send(ep_num, buf, sizeof(buf)) != sizeof(buf)) {
            usb_panic();
            return -1;
        }
        return 0;
    }
    return -1;
}

static void usb_cdc_notify_port_state_change(int port) {
    usb_cdc_serial_state_t state = usb_cdc_states[port].serial_state;
    if (state != usb_cdc_states[port].serial_state_prev) {
        if (usb_cdc_send_port_state(port, state) != -1) {
            usb_cdc_serial_state_t mask = (state & (USB_CDC_SERIAL_STATE_OVERRUN | USB_CDC_SERIAL_STATE_PARITY_ERROR));
            usb_cdc_serial_state_t _state;
            do {
                _state = usb_cdc_states[port].serial_state;
            } while (!(__sync_bool_compare_and_swap(&usb_cdc_states[port].serial_state, _state, (_state ^ mask))));
            usb_cdc_states[port].serial_state_prev = state ^ mask;
        }
    }
}

static void usb_cdc_notify_port_overrun(int port) {
    usb_cdc_serial_state_t _state;
    do {
        _state = usb_cdc_states[port].serial_state;
    } while (!(__sync_bool_compare_and_swap(&usb_cdc_states[port].serial_state, _state, (_state | USB_CDC_SERIAL_STATE_OVERRUN))));
}

static void usb_cdc_notify_port_parity_error(int port) {
    usb_cdc_serial_state_t _state;
    do {
        _state = usb_cdc_states[port].serial_state;
    } while (!(__sync_bool_compare_and_swap(&usb_cdc_states[port].serial_state, _state, (_state | USB_CDC_SERIAL_STATE_PARITY_ERROR))));
}

/* Line State and Coding */

static void usb_cdc_update_port_dtr(int port) {
    if (port < USB_CDC_NUM_PORTS) {
        usb_cdc_state_t *cdc_state = &usb_cdc_states[port];
        const gpio_pin_t *dtr_pin = &device_config_get()->cdc_config.port_config[port].pins[cdc_pin_dtr];
        gpio_pin_set(dtr_pin, cdc_state->dtr_active);
    }
}

static void usb_cdc_set_port_dtr(int port, int dtr_active) {
    if (port < USB_CDC_NUM_PORTS) {
        usb_cdc_state_t *cdc_state = &usb_cdc_states[port];
        cdc_state->dtr_active = dtr_active;
        usb_cdc_update_port_dtr(port);
    }
}

static void usb_cdc_update_port_rts(int port) {
    if ((port < USB_CDC_NUM_PORTS) && (port != 0)) {
        const gpio_pin_t *rts_pin = &device_config_get()->cdc_config.port_config[port].pins[cdc_pin_rts];
        usb_cdc_state_t *cdc_state = &usb_cdc_states[port];
        circ_buf_t *rx_buf = &cdc_state->rx_buf;
        int rts_active = (circ_buf_space(rx_buf->head, rx_buf->tail, USB_CDC_BUF_SIZE) > (USB_CDC_BUF_SIZE>>1)) && cdc_state->rts_active;
        gpio_pin_set(rts_pin, rts_active);
    }
}

static void usb_cdc_set_port_rts(int port, int rts_active) {
    if ((port < USB_CDC_NUM_PORTS)) {
        usb_cdc_states[port].rts_active = rts_active;
        usb_cdc_update_port_rts(port);
    }
}

static void usb_cdc_update_port_txa(int port) {
    if (port < USB_CDC_NUM_PORTS) {
        usb_cdc_state_t *cdc_state = &usb_cdc_states[port];
        const gpio_pin_t *txa_pin = &device_config_get()->cdc_config.port_config[port].pins[cdc_pin_txa];
        gpio_pin_set(txa_pin, cdc_state->txa_active);
    }
}

static void usb_cdc_set_port_txa(int port, int txa_active) {
    if ((port < USB_CDC_NUM_PORTS)) {
        usb_cdc_states[port].txa_active = txa_active;
        usb_cdc_update_port_txa(port);
    }
}

static usb_status_t usb_cdc_set_control_line_state(int port, uint16_t state) {
    usb_cdc_set_port_dtr(port, (state & USB_CDC_CONTROL_LINE_STATE_DTR_MASK));
    usb_cdc_set_port_rts(port, (state & USB_CDC_CONTROL_LINE_STATE_RTS_MASK));
    return usb_status_ack;
}

static usb_status_t usb_cdc_set_line_coding(int port, const usb_cdc_line_coding_t *line_coding, int dry_run) {
    USART_TypeDef *usart = usb_cdc_get_port_usart(port);
    if (line_coding->dwDTERate != 0) {
        uint32_t new_brr = usb_cdc_get_port_fck(port) / line_coding->dwDTERate;
        if (!dry_run) {
            usart->BRR = new_brr;
        }
        usb_cdc_states[port].line_coding.dwDTERate = line_coding->dwDTERate;
    }
    if (line_coding->bCharFormat < usb_cdc_char_format_last) {
        uint32_t new_char_format = 0;
        switch(line_coding->bCharFormat) {
            case usb_cdc_char_format_1_stop_bit:
                new_char_format = 0;
                break;
            case usb_cdc_char_format_1p5_stop_bits:
                new_char_format = USART_CR2_STOP_0 | USART_CR2_STOP_1;
                break;
            case usb_cdc_char_format_2_stop_bits:
                new_char_format = USART_CR2_STOP_1;
                break;
            default:
                break;
        }
        if (!dry_run) {
            usart->CR2 = (usart->CR2 & ~(USART_CR2_STOP)) | new_char_format;
        }
        usb_cdc_states[port].line_coding.bCharFormat = line_coding->bCharFormat;
    } else {
        return usb_status_fail;
    }
    if ((line_coding->bDataBits != usb_cdc_data_bits_8) &&
        ((line_coding->bDataBits != usb_cdc_data_bits_7)) && (line_coding->bParityType != usb_cdc_parity_type_none)) {
        return usb_status_fail;
    }
    usb_cdc_states[port].line_coding.bDataBits = line_coding->bDataBits;
    if (line_coding->bParityType < usb_cdc_parity_type_mark) {
        uint32_t new_parity = 0;
        switch (line_coding->bParityType) {
        case usb_cdc_parity_type_none:
            new_parity = 0;
            break;
        case usb_cdc_parity_type_odd:
            new_parity = USART_CR1_PCE | USART_CR1_PS;
            break;
        case usb_cdc_parity_type_even:
            new_parity = USART_CR1_PCE;
            break;
        default:
            break;
        }
        if ((line_coding->bParityType != usb_cdc_parity_type_none) &&
            (line_coding->bDataBits == usb_cdc_data_bits_8)) {
            new_parity |= USART_CR1_M;
        }
        if (!dry_run) {
            usart->CR1 = (usart->CR1 & ~((USART_CR1_M | USART_CR1_PCE | USART_CR1_PS))) | new_parity;
        }
        usb_cdc_states[port].line_coding.bParityType = line_coding->bParityType;
    } else {
        return usb_status_fail;
    }
    if (!dry_run) {
        /* force sending port state, this helps some apps */
        usb_cdc_states[port].serial_state_prev = ~(usb_cdc_states[port].serial_state_prev & 0);
    }
    return usb_status_ack;
}

/* USB USART RX Functions */

static void usb_cdc_port_send_rx_usb(int port) {
    usb_cdc_state_t *cdc_state = &usb_cdc_states[port];
    circ_buf_t *rx_buf = &cdc_state->rx_buf;
    uint8_t rx_ep = usb_cdc_get_port_data_ep(port);
    size_t rx_bytes_available = circ_buf_count(rx_buf->head, rx_buf->tail, USB_CDC_BUF_SIZE);
    if (rx_bytes_available) {
        size_t ep_space_available = usb_space_available(rx_ep);
        if (ep_space_available) {
            if (cdc_state->line_coding.bDataBits == usb_cdc_data_bits_7) {
                size_t bytes_count = ep_space_available < rx_bytes_available ? ep_space_available : rx_bytes_available;
                uint8_t *buf_ptr = &rx_buf->data[rx_buf->tail];
                while (bytes_count--) {
                    *(buf_ptr++) &= 0x7f;
                }
            }
            cdc_state->rx_zlp_pending = (usb_circ_buf_send(rx_ep, rx_buf, USB_CDC_BUF_SIZE) == ep_space_available);
            usb_cdc_update_port_rts(port);
        }
    } else {
        if (cdc_state->rx_zlp_pending) {
            cdc_state->rx_zlp_pending = 0;
            usb_send(rx_ep, 0, 0);
        }
    }
}

static void usb_cdc_port_start_rx(int port) {
    DMA_Channel_TypeDef *dma_rx_ch = usb_cdc_get_port_dma_channel(port, usb_cdc_port_direction_rx);
    usb_cdc_state_t *cdc_state = &usb_cdc_states[port];
    circ_buf_t *rx_buf = &cdc_state->rx_buf;
    dma_rx_ch->CCR &= ~(DMA_CCR_EN);
    dma_rx_ch->CMAR = (uint32_t)&rx_buf->data[rx_buf->head];
    dma_rx_ch->CNDTR = USB_CDC_BUF_SIZE;
    dma_rx_ch->CCR |= DMA_CCR_EN;
}

static void usb_cdc_port_rx_interrupt(int port) {
    circ_buf_t *rx_buf = &usb_cdc_states[port].rx_buf;
    int rx_buf_tail = rx_buf->tail;
    size_t current_rx_bytes_available = circ_buf_count(rx_buf->head, rx_buf_tail, USB_CDC_BUF_SIZE);
    DMA_Channel_TypeDef *dma_rx_ch = usb_cdc_get_port_dma_channel(port, usb_cdc_port_direction_rx);
    size_t dma_head = USB_CDC_BUF_SIZE - dma_rx_ch->CNDTR;
    size_t dma_rx_bytes_available = circ_buf_count(dma_head, rx_buf_tail, USB_CDC_BUF_SIZE);
    usb_cdc_update_port_rts(port);
    if (dma_rx_bytes_available < current_rx_bytes_available) {
        usb_cdc_notify_port_overrun(port);
    }
    rx_buf->head = dma_head;
}

/* Configuration Mode Handling */

void usb_cdc_config_mode_enter() {
    usb_cdc_state_t *cdc_state = &usb_cdc_states[USB_CDC_CONFIG_PORT];
    USART_TypeDef *usart = usb_cdc_get_port_usart(USB_CDC_CONFIG_PORT);
    DMA_Channel_TypeDef *dma_tx_ch = usb_cdc_get_port_dma_channel(USB_CDC_CONFIG_PORT, usb_cdc_port_direction_tx);
    cdc_state->rx_buf.tail = cdc_state->rx_buf.head = 0;
    cdc_state->tx_buf.tail = cdc_state->tx_buf.head = 0;
    usart->CR1 &= ~(USART_CR1_RE);
    dma_tx_ch->CCR &= ~(DMA_CCR_EN);
    cdc_shell_init();
    usb_cdc_config_mode = 1;
}

void usb_cdc_config_mode_leave() {
    usb_cdc_state_t *cdc_state = &usb_cdc_states[USB_CDC_CONFIG_PORT];
    DMA_Channel_TypeDef *dma_rx_ch = usb_cdc_get_port_dma_channel(USB_CDC_CONFIG_PORT, usb_cdc_port_direction_rx);
    size_t dma_head = USB_CDC_BUF_SIZE - dma_rx_ch->CNDTR;
    USART_TypeDef *usart = usb_cdc_get_port_usart(USB_CDC_CONFIG_PORT);
    cdc_state->rx_buf.tail = cdc_state->rx_buf.head = dma_head;
    cdc_state->tx_buf.tail = cdc_state->tx_buf.head = 0;
    usart->CR1 |= USART_CR1_RE;
    usb_cdc_config_mode = 0;
}

/*
 * USB_CDC_CONFIG_PORT buffers are reused for the config shell.
 * usb_cdc_config_process_tx must ensure enough tx buf space is available
 * for the next usb transfer.
 */

void usb_cdc_config_mode_process_tx() {
    uint8_t ep_num = usb_cdc_get_port_data_ep(USB_CDC_CONFIG_PORT);
    usb_cdc_state_t *cdc_state = &usb_cdc_states[USB_CDC_CONFIG_PORT];
    circ_buf_t *tx_buf = &cdc_state->tx_buf;
    size_t count;
    if (usb_bytes_available(ep_num) < circ_buf_space(tx_buf->head, tx_buf->tail, USB_CDC_BUF_SIZE)) {
        usb_circ_buf_read(ep_num, tx_buf, USB_CDC_BUF_SIZE);
    } else {
        usb_panic();
    }
    while((count = circ_buf_count_to_end(tx_buf->head, tx_buf->tail, USB_CDC_BUF_SIZE))) {
        cdc_shell_process_input(&tx_buf->data[tx_buf->tail], count);
        tx_buf->tail  = (tx_buf->tail + count) & (USB_CDC_BUF_SIZE - 1);
    }
}

void cdc_shell_write(const void *buf, size_t count) {
    usb_cdc_state_t *cdc_state = &usb_cdc_states[USB_CDC_CONFIG_PORT];
    circ_buf_t *rx_buf = &cdc_state->rx_buf;
    while (count) {
        size_t bytes_to_copy;
        size_t space_available = circ_buf_space_to_end(rx_buf->head, rx_buf->tail, USB_CDC_BUF_SIZE);
        if (space_available == 0) {
            rx_buf->tail = rx_buf->head;
            space_available = circ_buf_space_to_end(rx_buf->head, rx_buf->tail, USB_CDC_BUF_SIZE);
        }
        bytes_to_copy = (space_available > count) ? count : space_available;
        memcpy(&rx_buf->data[rx_buf->head], buf, bytes_to_copy);
        rx_buf->head = (rx_buf->head + bytes_to_copy) & (USB_CDC_BUF_SIZE - 1);
        count -= bytes_to_copy;
        buf = (uint8_t*)buf + bytes_to_copy;
    }
}

/* USB USART TX Functions */

static void usb_cdc_port_start_tx(int port) {
    DMA_Channel_TypeDef *dma_tx_ch = usb_cdc_get_port_dma_channel(port, usb_cdc_port_direction_tx);
    usb_cdc_state_t *cdc_state = &usb_cdc_states[port];
    circ_buf_t *tx_buf = &cdc_state->tx_buf;
    size_t tx_bytes_available = circ_buf_count_to_end(tx_buf->head, tx_buf->tail, USB_CDC_BUF_SIZE);
    int dma_ch_busy = dma_tx_ch->CCR & DMA_CCR_EN;
    if (!dma_ch_busy) {
        if (tx_bytes_available) {
            usb_cdc_set_port_txa(port, 1);
            dma_tx_ch->CMAR = (uint32_t)&tx_buf->data[tx_buf->tail];
            dma_tx_ch->CNDTR = tx_bytes_available;
            dma_tx_ch->CCR |= DMA_CCR_EN;
            cdc_state->last_dma_tx_size = tx_bytes_available;
        } else {
            USART_TypeDef *usart = usb_cdc_get_port_usart(port);
            usart->SR &= ~(USART_SR_TC);
            usart->CR1 |= USART_CR1_TCIE;
        }
    }
}

static void usb_cdc_port_tx_complete(int port) {
    DMA_Channel_TypeDef *dma_tx_ch = usb_cdc_get_port_dma_channel(port, usb_cdc_port_direction_tx);
    usb_cdc_state_t *cdc_state = &usb_cdc_states[port];
    circ_buf_t *tx_buf = &cdc_state->tx_buf;
    tx_buf->tail = (tx_buf->tail + cdc_state->last_dma_tx_size) & (USB_CDC_BUF_SIZE - 1);
    dma_tx_ch->CCR &= ~(DMA_CCR_EN);
    if (cdc_state->line_state_change_pending) {
        size_t tx_bytes_available = circ_buf_count(tx_buf->head, tx_buf->tail, USB_CDC_BUF_SIZE);
        if (tx_bytes_available == 0) {
            cdc_state->line_state_change_ready = 1;
        }
    }
    if ((port != USB_CDC_CONFIG_PORT) || !usb_cdc_config_mode) {
        usb_cdc_port_start_tx(port);
    } else {
        usb_cdc_set_port_txa(port, 0);
        usb_cdc_config_mode_process_tx(port);
    }
}

/* DMA Interrupt Handlers */

void DMA1_Channel5_IRQHandler() {
    (void)DMA1_Channel5_IRQHandler;
    uint32_t status = DMA1->ISR & ( DMA_ISR_TCIF5 | DMA_ISR_HTIF5);
    DMA1->IFCR = status;
    usb_cdc_port_rx_interrupt(0);
}

void DMA1_Channel4_IRQHandler() {
    (void)DMA1_Channel4_IRQHandler;
    uint32_t status = DMA1->ISR & ( DMA_ISR_TCIF4 );
    DMA1->IFCR = status;
    usb_cdc_port_tx_complete(0);
}

void DMA1_Channel6_IRQHandler() {
    (void)DMA1_Channel6_IRQHandler;
    uint32_t status = DMA1->ISR & ( DMA_ISR_TCIF6 | DMA_ISR_HTIF6);
    DMA1->IFCR = status;
    usb_cdc_port_rx_interrupt(1);
}

void DMA1_Channel7_IRQHandler() {
    (void)DMA1_Channel7_IRQHandler;
    uint32_t status = DMA1->ISR & ( DMA_ISR_TCIF7 );
    DMA1->IFCR = status;
    usb_cdc_port_tx_complete(1);
}

void DMA1_Channel3_IRQHandler() {
    (void)DMA1_Channel3_IRQHandler;
    uint32_t status = DMA1->ISR & ( DMA_ISR_TCIF3 | DMA_ISR_HTIF3);
    DMA1->IFCR = status;
    usb_cdc_port_rx_interrupt(2);
}

void DMA1_Channel2_IRQHandler() {
    (void)DMA1_Channel2_IRQHandler;
    uint32_t status = DMA1->ISR & ( DMA_ISR_TCIF2 );
    DMA1->IFCR = status;
    usb_cdc_port_tx_complete(2);
}

/* USART Interrupt Handlers */

static void usb_cdc_usart_irq_handler(int port) {
    USART_TypeDef *usart = usb_cdc_get_port_usart(port);
    uint32_t wait_rxne = 0;
    uint32_t status = usart->SR;
    if (status & USART_SR_TC) {
        usb_cdc_set_port_txa(port, 0);
        usart->CR1 &= ~(USART_CR1_TCIE);
    }
    if (status & USART_SR_PE) {
        wait_rxne = 1;
        usb_cdc_notify_port_parity_error(port);
    }
    if (status & USART_SR_ORE) {
        usb_cdc_notify_port_overrun(port);
    }
    if (status & USART_SR_IDLE) {
        usb_cdc_port_rx_interrupt(port);
    }
    while (wait_rxne && (usart->SR & USART_SR_RXNE));
    (void)usart->DR;
}

void USART1_IRQHandler() {
    (void)USART1_IRQHandler;
    usb_cdc_usart_irq_handler(0);
}

void USART2_IRQHandler() {
    (void)USART2_IRQHandler;
    usb_cdc_usart_irq_handler(1);
}

void USART3_IRQHandler() {
    (void)USART3_IRQHandler;
    usb_cdc_usart_irq_handler(2);
}

/* Port Configuration & Control Lines Functions */

void usb_cdc_reconfigure_port_pin(int port, cdc_pin_t pin) {
    if (port < USB_CDC_NUM_PORTS && pin < cdc_pin_last) {
        gpio_pin_init(&device_config_get()->cdc_config.port_config[port].pins[pin]);
        if (pin == cdc_pin_rts) {
            usb_cdc_update_port_rts(port);
        } else if (pin == cdc_pin_dtr) {
            usb_cdc_update_port_dtr(port);
        } else if (pin == cdc_pin_txa) {
            usb_cdc_update_port_txa(port);
        }
    }
}

static void usb_cdc_configure_port(int port) {
    const device_config_t *device_config = device_config_get();
    for (cdc_pin_t pin = 0; pin < cdc_pin_last; pin++) {
        gpio_pin_init(&device_config->cdc_config.port_config[port].pins[pin]);
        usb_cdc_update_port_rts(port);
        usb_cdc_update_port_dtr(port);
        usb_cdc_update_port_txa(port);
    }
}

void usb_cdc_reconfigure() {
    for (int port = 0; port < USB_CDC_NUM_PORTS; port++) {
        usb_cdc_configure_port(port);
    }
}

/* Device Lifecycle */

void usb_cdc_reset() {
    const device_config_t *device_config = device_config_get();
    usb_cdc_enabled = 0;
    NVIC_SetPriority(DMA1_Channel2_IRQn, SYSTEM_INTERRUTPS_PRIORITY_HIGH);
    NVIC_EnableIRQ(DMA1_Channel2_IRQn);
    NVIC_SetPriority(DMA1_Channel3_IRQn, SYSTEM_INTERRUTPS_PRIORITY_BASE);
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);
    NVIC_SetPriority(DMA1_Channel4_IRQn, SYSTEM_INTERRUTPS_PRIORITY_HIGH);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    NVIC_SetPriority(DMA1_Channel5_IRQn, SYSTEM_INTERRUTPS_PRIORITY_BASE);
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    NVIC_SetPriority(DMA1_Channel6_IRQn, SYSTEM_INTERRUTPS_PRIORITY_BASE);
    NVIC_EnableIRQ(DMA1_Channel6_IRQn);
    NVIC_SetPriority(DMA1_Channel7_IRQn, SYSTEM_INTERRUTPS_PRIORITY_HIGH);
    NVIC_EnableIRQ(DMA1_Channel7_IRQn);
    /* 
     * Disable JTAG interface (SWD is still enabled),
     * this frees PA15, PB3, PB4 (needed for DSR/RI inputs).
     */
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
    /* Configuration Mode Pin */
    gpio_pin_init(&device_config->config_pin);
    /* USART & DMA Reset and Setup */
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_USART3EN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    RCC->APB2RSTR |= RCC_APB2RSTR_USART1RST;
    RCC->APB1RSTR |= RCC_APB1RSTR_USART2RST;
    RCC->APB1RSTR |= RCC_APB1RSTR_USART3RST;
    RCC->APB2RSTR &= ~(RCC_APB2RSTR_USART1RST);
    RCC->APB1RSTR &= ~(RCC_APB1RSTR_USART2RST);
    RCC->APB1RSTR &= ~(RCC_APB1RSTR_USART3RST);
    memset(&usb_cdc_states, 0, sizeof(usb_cdc_states));
    for (int port=0; port<USB_CDC_NUM_PORTS; port++) {
        (void)usb_cdc_states[port]._rx_data;
        (void)usb_cdc_states[port]._tx_data;
        usb_cdc_configure_port(port);
        USART_TypeDef *usart = usb_cdc_get_port_usart(port);
        DMA_Channel_TypeDef *dma_rx_ch = usb_cdc_get_port_dma_channel(port, usb_cdc_port_direction_rx);
        DMA_Channel_TypeDef *dma_tx_ch = usb_cdc_get_port_dma_channel(port, usb_cdc_port_direction_tx);
        usart->CR1 |= USART_CR1_UE | USART_CR1_TE;
        usart->CR3 |= USART_CR3_DMAR | USART_CR3_DMAT | USART_CR3_EIE;
        if (port != 0) {
            usart->CR3 |= USART_CR3_CTSE;
        }
        usb_cdc_set_line_coding(port, &usb_cdc_default_line_coding, 0);
        dma_rx_ch->CCR |= DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_TCIE | DMA_CCR_HTIE | DMA_CCR_PL_0;
        dma_rx_ch->CPAR = (uint32_t)&usart->DR;
        dma_rx_ch->CMAR = (uint32_t)usb_cdc_states[port].rx_buf.data;
        dma_rx_ch->CNDTR = USB_CDC_BUF_SIZE;
        dma_tx_ch->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE;
        dma_tx_ch->CPAR = (uint32_t)&usart->DR;
    }
    NVIC_SetPriority(USART1_IRQn, SYSTEM_INTERRUTPS_PRIORITY_CRITICAL);
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART2_IRQn, SYSTEM_INTERRUTPS_PRIORITY_CRITICAL);
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_SetPriority(USART3_IRQn, SYSTEM_INTERRUTPS_PRIORITY_CRITICAL);
    NVIC_EnableIRQ(USART3_IRQn);
}

void usb_cdc_enable() {
    usb_cdc_enabled = 1;
    for (int port=0; port<USB_CDC_NUM_PORTS; port++) {
        USART_TypeDef *usart = usb_cdc_get_port_usart(port);
        usart->CR1 |= USART_CR1_PEIE | USART_CR1_IDLEIE | USART_CR1_RE | USART_CR1_PEIE;
        usb_cdc_port_start_rx(port);
    }
}

void usb_cdc_suspend() {
    usb_cdc_enabled = 0;
    for (int port=0; port<USB_CDC_NUM_PORTS; port++) {
        USART_TypeDef *usart = usb_cdc_get_port_usart(port);
        usart->CR1 &= ~(USART_CR1_UE);
    }
}

void usb_cdc_frame() {
    if (usb_cdc_enabled) {
        const device_config_t *device_config = device_config_get();
        static unsigned int ctrl_lines_polling_timer = 0;
        if (ctrl_lines_polling_timer == 0) {
            ctrl_lines_polling_timer = USB_CDC_CRTL_LINES_POLLING_INTERVAL;
            for (int port = 0; port < USB_CDC_NUM_PORTS; port++) {
                const cdc_port_t *port_config = &device_config_get()->cdc_config.port_config[port];
                usb_cdc_serial_state_t _state, new_state, control_lines_state = 0;
                if (gpio_pin_get(&port_config->pins[cdc_pin_dsr])) {
                    control_lines_state |= USB_CDC_SERIAL_STATE_DSR;
                }
                if (gpio_pin_get(&port_config->pins[cdc_pin_dcd])) {
                    control_lines_state |= USB_CDC_SERIAL_STATE_DCD;
                }
                if (gpio_pin_get(&port_config->pins[cdc_pin_ri])) {
                    control_lines_state |= USB_CDC_SERIAL_STATE_RI;
                }
                do {
                    _state = usb_cdc_states[port].serial_state;
                    new_state = _state & ~(USB_CDC_SERIAL_STATE_DSR | USB_CDC_SERIAL_STATE_DCD | USB_CDC_SERIAL_STATE_RI);
                    new_state |= control_lines_state;
                } while (!(__sync_bool_compare_and_swap(&usb_cdc_states[port].serial_state, _state, new_state)));
            }
            if (gpio_pin_get(&device_config->config_pin) != usb_cdc_config_mode) {
                if (usb_cdc_config_mode) {
                    usb_cdc_config_mode_leave();
                } else {
                    usb_cdc_config_mode_enter();
                }
            }
        } else {
            ctrl_lines_polling_timer = ctrl_lines_polling_timer - 1;
        }
    }
}

/* Endpoint Handlers */

void usb_cdc_data_endpoint_event_handler(uint8_t ep_num, usb_endpoint_event_t ep_event) {
    int port = usb_cdc_data_endpoint_port(ep_num);
    if (port != -1) {
        usb_cdc_state_t *cdc_state = &usb_cdc_states[port];
        if (ep_event == usb_endpoint_event_data_received) {
            circ_buf_t *tx_buf = &cdc_state->tx_buf;
            size_t tx_space_available = circ_buf_space(tx_buf->head, tx_buf->tail, USB_CDC_BUF_SIZE);
            size_t rx_bytes_available = usb_bytes_available(ep_num);
            if ((port == USB_CDC_CONFIG_PORT) && usb_cdc_config_mode) {
                usb_cdc_config_mode_process_tx(port);
            } else {
                /* Do not receive data until line state change is complete */
                if ((tx_space_available < rx_bytes_available) || (cdc_state->line_state_change_pending)) {
                    cdc_state->usb_rx_pending_ep = ep_num;
                } else {
                    usb_circ_buf_read(ep_num, tx_buf, USB_CDC_BUF_SIZE);
                    usb_cdc_port_start_tx(port);
                }
            }
        }
    }
}

usb_status_t usb_cdc_ctrl_process_request(usb_setup_t *setup, void **payload,
                                          size_t *payload_size, usb_tx_complete_cb_t *tx_callback_ptr) {
    if ((setup->type == usb_setup_type_class) &&
        (setup->recepient == usb_setup_recepient_interface)) {
        int if_num = setup->wIndex;
        int port = usb_cdc_get_interface_port(if_num);
        if (port != -1) {
            switch (setup->bRequest) {
            case usb_cdc_request_set_control_line_state:
                return usb_cdc_set_control_line_state(port, setup->wValue);
            case usb_cdc_request_set_line_coding: {
                usb_cdc_line_coding_t *line_coding = (usb_cdc_line_coding_t *)setup->payload;
                if (setup->wLength == sizeof(usb_cdc_line_coding_t)) {
                    int dry_run = 0;
                    circ_buf_t *tx_buf = &usb_cdc_states[port].tx_buf;
                    /* 
                     * If the TX buffer is not empty, defer setting
                     * line coding until all data are sent over the serial port.
                     */
                    if ((port != USB_CDC_CONFIG_PORT) || !usb_cdc_config_mode) {
                        if (circ_buf_count(tx_buf->head, tx_buf->tail, USB_CDC_BUF_SIZE) != 0) {
                            dry_run = 1;
                            usb_cdc_states[port].line_state_change_pending = 1;
                        }
                    }
                    return usb_cdc_set_line_coding(port, line_coding, dry_run);
                }
                break;
            }
            case usb_cdc_request_get_line_coding:
                if (setup->wLength == sizeof(usb_cdc_line_coding_t)) {
                    *payload = (uint8_t*)&usb_cdc_states[port].line_coding;
                    *payload_size = sizeof(usb_cdc_line_coding_t);
                    return usb_status_ack;
                }
                break;
            default:
                usb_panic();
            }
        }
    }
    return usb_status_fail;
}

void usb_cdc_poll() {
    for (int port = 0; port < (USB_CDC_NUM_PORTS); port++) {
        usb_cdc_state_t *cdc_state = &usb_cdc_states[port];
        circ_buf_t *tx_buf = &cdc_state->tx_buf;
        usb_cdc_notify_port_state_change(port);
        usb_cdc_port_send_rx_usb(port);
        if (cdc_state->line_state_change_ready) {
            usb_cdc_set_line_coding(port, &cdc_state->line_coding, 0);
            cdc_state->line_state_change_pending = 0;
            cdc_state->line_state_change_ready = 0;
        }
        if (cdc_state->usb_rx_pending_ep) {
            size_t tx_space_available = circ_buf_space(tx_buf->head, tx_buf->tail, USB_CDC_BUF_SIZE);
            size_t rx_bytes_available = usb_bytes_available(cdc_state->usb_rx_pending_ep);
            if (tx_space_available >= rx_bytes_available) {
                usb_circ_buf_read(cdc_state->usb_rx_pending_ep, tx_buf, USB_CDC_BUF_SIZE);
                usb_cdc_port_start_tx(port);
                cdc_state->usb_rx_pending_ep = 0;
        }

    }
    }
}
