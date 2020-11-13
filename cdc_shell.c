/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include <string.h>
#include "cdc_shell.h"

static char cmd_line[USB_SHELL_MAX_CMD_LINE_SIZE];
static char *cmd_line_p = cmd_line;

static const char *cdc_shell_banner =   "\r\n\r\n"
                                        "*******************************\r\n"
                                        "* Configuration Shell Started *\r\n"
                                        "*******************************\t\n"
                                        "\r\n";
static const char *cdc_shell_prompt = ">";

void cdc_shell_init() {
    cmd_line_p = cmd_line;
    _cdc_shell_write(cdc_shell_banner, strlen(cdc_shell_banner));
    _cdc_shell_write(cdc_shell_prompt, strlen(cdc_shell_prompt));
}

void cdc_shell_exit() {

}

void cdc_shell_process_input(const void *buf, size_t count) {
    _cdc_shell_write(buf, count);
}
