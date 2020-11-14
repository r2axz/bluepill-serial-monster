/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "cdc_shell.h"

static const char *cdc_shell_banner                 = "\r\n\r\n"
                                                      "*******************************\r\n"
                                                      "* Configuration Shell Started *\r\n"
                                                      "*******************************\r\n\r\n";
static const char *cdc_shell_prompt                 = ">";
static const char *cdc_shell_new_line               = "\r\n";
static const char *cdc_shell_err_too_long           = "Error, command line is too long.\r\n";
static const char *cdc_shell_err_too_many_args      = "Error, too many command line arguments.\r\n";
static const char *cdc_shell_err_unknown_command    = "Error, unknown command, use \"help\" to get the list of the available commands.\r\n";


static const char *escape_cursor_forward        = "\033[C";
static const char *escape_cursor_backward       = "\033[D";
static const char *escape_clear_line_to_end     = "\033[0K";

typedef void (*cmd_func_t)(int argc, char *argv[]);

typedef struct {
    char *cmd;
    cmd_func_t handler;
    char *help;
} cdc_shell_cmd_t;

void cdc_shell_cmd_set(int argc, char *argv[]) {

}

void cdc_shell_cmd_help(int argc, char *argv[]);

static const cdc_shell_cmd_t cdc_shell_commands[] = {
    { "help",   cdc_shell_cmd_help, "displays this help message,\r\n\t  use \"command-name help\" to get command-specific help"},
    { "set",    cdc_shell_cmd_set,  "set configuration parameters" },
    { 0 }
};

void cdc_shell_cmd_help(int argc, char *argv[]) {
    const char *delim = "\t- ";
    const cdc_shell_cmd_t *shell_cmd = cdc_shell_commands;
    while (shell_cmd->cmd) {
        cdc_shell_write(shell_cmd->cmd, strlen(shell_cmd->cmd));
        cdc_shell_write(delim, strlen(delim));
        cdc_shell_write(shell_cmd->help, strlen(shell_cmd->help));
        cdc_shell_write(cdc_shell_new_line, strlen(cdc_shell_new_line));
        shell_cmd++;
    }
}

void cdc_shell_exec_command(int argc, char *argv[]) {
    const cdc_shell_cmd_t *shell_cmd = cdc_shell_commands;
    if (argc) {
        while (shell_cmd->cmd) {
            if (strcmp(shell_cmd->cmd, *argv) == 0) {
                shell_cmd->handler(argc-1, argv+1);
                return;
            }
            shell_cmd++;
        }
    }
    cdc_shell_write(cdc_shell_err_unknown_command, strlen(cdc_shell_err_unknown_command));
}

void cdc_shell_parse_command_line(char *cmd_line) {
    int argc = 0;
    char *argv[USB_SHELL_MAC_CMD_ARGS];
    char *cmd_line_p = cmd_line;
    while (isspace((int)(*cmd_line_p))) {
        cmd_line_p++;
    }
    while (*cmd_line_p) {
        if (argc < USB_SHELL_MAC_CMD_ARGS) {
            argv[argc] = cmd_line_p;
            while (*cmd_line_p && !isspace((int)(*cmd_line_p))) {
                cmd_line_p++;
            }
            if (*cmd_line_p) {
                *cmd_line_p++ = '\0';
            }
            while (isspace((int)(*cmd_line_p))) {
                cmd_line_p++;
            }
            argc++;
        } else {
            cdc_shell_write(cdc_shell_err_too_many_args, strlen(cdc_shell_err_too_many_args));
            return;
        }
    }
    if (argc > 0) {
        cdc_shell_exec_command(argc, argv);
    }
}

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
    cdc_shell_expects_csn,
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

void cdc_shell_cursor_move_back(int n_symb) {
    if (n_symb) {
        char n_symb_str[8];
        itoa(n_symb, n_symb_str, 10);
        cdc_shell_write("\033[", 2);
        cdc_shell_write(n_symb_str, strlen(n_symb_str));
        cdc_shell_write("D", 1);
    }
}

void cdc_shell_handle_backspace() {
    if (cmd_line_cursor > cmd_line_buf) {
        cdc_shell_cursor_move_back(cmd_line_cursor - cmd_line_buf);
        cdc_shell_write(escape_clear_line_to_end, strlen(escape_clear_line_to_end));
        cmd_line_cursor--;
        memmove(cmd_line_cursor, cmd_line_cursor+1, strlen(cmd_line_cursor));
        cdc_shell_write(cmd_line_buf, strlen(cmd_line_buf));
        cdc_shell_cursor_move_back(strlen(cmd_line_buf) - (cmd_line_cursor - cmd_line_buf));
    }
}

void cdc_shell_insert_symbol(char c) {
    memmove(cmd_line_cursor+1, cmd_line_cursor, strlen(cmd_line_cursor)+1);
    *cmd_line_cursor = c;
    cdc_shell_write(cmd_line_cursor, strlen(cmd_line_cursor));
    cmd_line_cursor++;
    cdc_shell_cursor_move_back(strlen(cmd_line_buf) - (cmd_line_cursor - cmd_line_buf));
}

void cdc_shell_process_input(const void *buf, size_t count) {
    const char *buf_p= buf;
    while (count--) {
        switch (cdc_shell_state) {
        case cdc_shell_expects_csn:
            if (isdigit((int)*buf_p)) {
                /* Ignore values for simplicity */
                break;
            } else {
                if (*buf_p == ANSI_CTRLSEQ_CUF) {
                    if (*cmd_line_cursor) {
                        cmd_line_cursor++;
                        cdc_shell_write(escape_cursor_forward, strlen(escape_cursor_forward));
                    }
                } else if (*buf_p == ANSI_CTRLSEQ_CUB) {
                    if (cmd_line_cursor > cmd_line_buf) {
                        cmd_line_cursor--;
                        cdc_shell_write(escape_cursor_backward, strlen(escape_cursor_backward));
                    }
                }
                cdc_shell_state = cdc_shell_idle;
            }
            break;
        case cdc_shell_expects_csi:
            if (*buf_p == ANSI_CTRLSEQ_ESCAPE_CSI) {
                cdc_shell_state = cdc_shell_expects_csn;
                break;
            }
            cdc_shell_state = cdc_shell_idle;
        case cdc_shell_expects_lf:
            cdc_shell_state = cdc_shell_idle;
            if (*buf_p == '\n') {
                break;
            }
        case cdc_shell_idle:
            if (*buf_p == '\r' || *buf_p == '\n') {
                cdc_shell_state = cdc_shell_expects_lf;
                cdc_shell_write(cdc_shell_new_line, strlen(cdc_shell_new_line));
                cdc_shell_parse_command_line(cmd_line_buf);
                cdc_shell_clear_cmd_buf();
                cdc_shell_write(cdc_shell_prompt, strlen(cdc_shell_prompt));
            } else if (*buf_p == ANSI_CTRLSEQ_ESCAPE_CHAR) {
                cdc_shell_state = cdc_shell_expects_csi;
            } else if (*buf_p == '\b' || *buf_p == '\177') {
                cdc_shell_handle_backspace();
            } else if (isprint((int)(*buf_p))) {
                cdc_shell_insert_symbol(*buf_p);
                if ((cmd_line_cursor - cmd_line_buf) >= sizeof(cmd_line_buf)/sizeof(*cmd_line_buf)) {
                    cdc_shell_clear_cmd_buf();
                    cdc_shell_write(cdc_shell_new_line, strlen(cdc_shell_new_line));
                    cdc_shell_write(cdc_shell_err_too_long, strlen(cdc_shell_err_too_long));
                    cdc_shell_write(cdc_shell_prompt, strlen(cdc_shell_prompt));
                }
            }
        }
        buf_p++;
    }
}
