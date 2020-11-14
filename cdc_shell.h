/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#ifndef CDC_SHELL_H
#define CDC_SHELL_H

#include <stddef.h>

#define USB_SHELL_MAX_CMD_LINE_SIZE     0x100
#define USB_SHELL_MAC_CMD_ARGS          0x10

extern void cdc_shell_write(const void *buf, size_t count);

void cdc_shell_init();
void cdc_shell_process_input(const void *buf, size_t count);

#endif /* CDC_SHELL_H */
