#include "cdc_shell.h"

static char cmd_line[USB_SHELL_MAX_CMD_LINE_SIZE];
static char *cmd_line_p = cmd_line;

void cdc_shell_init() {
    cmd_line_p = cmd_line;
}

void cdc_shell_exit() {

}

void cdc_shell_process_input(const void *buf, size_t count) {
    _cdc_shell_write(buf, count);
}