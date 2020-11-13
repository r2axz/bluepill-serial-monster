/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "cdc_shell.h"

void cdc_shell_process_command(const char *cmd) {

}

static const char *cdc_shell_banner             = "\r\n\r\n"
                                                  "*******************************\r\n"
                                                  "* Configuration Shell Started *\r\n"
                                                  "*******************************\t\n";
static const char *cdc_shell_prompt             = "\r\n>";
static const char *cdc_shell_err_too_long       = "\r\nError, command line is too long.";


static const char *escape_cursor_forward        = "\033[C";
static const char *escape_cursor_backward       = "\033[D";
static const char *escape_clear_line_to_end     = "\033[0K";

static char cmd_line_buf[USB_SHELL_MAX_CMD_LINE_SIZE];
static char *cmd_line_cursor = cmd_line_buf;

static void cdc_shell_clear_cmd_buf() {
    cmd_line_cursor = cmd_line_buf;
    memset(cmd_line_buf, 0, sizeof(cmd_line_buf));
}

static enum {
    cdc_shell_idle,
    cdc_shell_expects_lf,
    cdc_shell_expects_csi,
    cdc_shell_expects_csn
} cdc_shell_state;

void cdc_shell_init() {
    cdc_shell_clear_cmd_buf();
    cdc_shell_state = cdc_shell_idle;
    cdc_shell_write(cdc_shell_banner, strlen(cdc_shell_banner));
    cdc_shell_write(cdc_shell_prompt, strlen(cdc_shell_prompt));
}

#define ANSI_CTRLSEQ_ESCAPE_CHAR    0x1B
#define ANSI_CTRLSEQ_ESCAPE_CSI     0x5B
#define ANSI_CTRLSEQ_CUF            0x43
#define ANSI_CTRLSEQ_CUB            0x44

void cdc_shell_process_input(const void *buf, size_t count) {
    const char *sym = buf;
    while (count--) {
        switch (cdc_shell_state) {
        case cdc_shell_expects_csn:
            if (isdigit((int)*sym)) {
                /* Ignore values for simplicity */
                break;
            } else {
                if (*sym == ANSI_CTRLSEQ_CUF) {
                    if (*cmd_line_cursor) {
                        cmd_line_cursor++;
                        cdc_shell_write(escape_cursor_forward, strlen(escape_cursor_forward));
                    }
                } else if (*sym == ANSI_CTRLSEQ_CUB) {
                    if (cmd_line_cursor > cmd_line_buf) {
                        cmd_line_cursor--;
                        cdc_shell_write(escape_cursor_backward, strlen(escape_cursor_backward));
                    }
                }
                cdc_shell_state = cdc_shell_idle;
            }
            break;
        case cdc_shell_expects_csi:
            if (*sym == ANSI_CTRLSEQ_ESCAPE_CSI) {
                cdc_shell_state = cdc_shell_expects_csn;
                break;
            } else {
                cdc_shell_state = cdc_shell_idle;
                continue;
            }
        case cdc_shell_expects_lf:
            cdc_shell_state = cdc_shell_idle;
            if (*sym == '\n') {
                break;
            }
        case cdc_shell_idle:
            if (*sym == '\r' || *sym == '\n') {
                cdc_shell_state = cdc_shell_expects_lf;
                cdc_shell_process_command(cmd_line_buf);
                cdc_shell_clear_cmd_buf();
                cdc_shell_write(cdc_shell_prompt, strlen(cdc_shell_prompt));
            } else if (*sym == ANSI_CTRLSEQ_ESCAPE_CHAR) {
                cdc_shell_state = cdc_shell_expects_csi;
            } else if (*sym == '\b') {
                if (cmd_line_cursor > cmd_line_buf) {
                    size_t n_symb;
                    char n_symb_str[8];
                    itoa(cmd_line_cursor - cmd_line_buf, n_symb_str, 10);
                    cdc_shell_write("\033[", 2);
                    cdc_shell_write(n_symb_str, strlen(n_symb_str));
                    cdc_shell_write("D", 1);
                    cdc_shell_write(escape_clear_line_to_end, strlen(escape_clear_line_to_end));
                    cmd_line_cursor--;
                    memmove(cmd_line_cursor, cmd_line_cursor+1, strlen(cmd_line_cursor));
                    cdc_shell_write(cmd_line_buf, strlen(cmd_line_buf));
                    n_symb = strlen(cmd_line_buf) - (cmd_line_cursor - cmd_line_buf);
                    if (n_symb) {
                        itoa(n_symb, n_symb_str, 10);
                        cdc_shell_write("\033[", 2);
                        cdc_shell_write(n_symb_str, strlen(n_symb_str));
                        cdc_shell_write("D", 1);
                    }
                }
            } else if (isprint((int)(*sym))) {
                *cmd_line_cursor++ = *sym;
                if ((cmd_line_cursor - cmd_line_buf) >= sizeof(cmd_line_buf)/sizeof(*cmd_line_buf)) {
                    cdc_shell_clear_cmd_buf();
                    cdc_shell_write(cdc_shell_err_too_long, strlen(cdc_shell_err_too_long));
                    cdc_shell_write(cdc_shell_prompt, strlen(cdc_shell_prompt));
                }
                cdc_shell_write(sym, sizeof(*sym));
            }
        }
        sym++;
    }
}
