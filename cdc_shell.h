#ifndef CDC_SHELL_H
#define CDC_SHELL_H

#include <stddef.h>

#define USB_SHELL_MAX_CMD_LINE_SIZE     0x100

void cdc_shell_init();
void cdc_shell_exit();
void cdc_shell_write(const void *buf, size_t count);

#endif /* CDC_SHELL_H */
