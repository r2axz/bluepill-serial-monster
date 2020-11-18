/*
 * MIT License 
 * 
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "usb_cdc.h"
#include "gpio.h"
#include "cdc_config.h"
#include "device_config.h"
#include "cdc_shell.h"


static const char *cdc_shell_banner                 = "\r\n\r\n"
                                                      "*******************************\r\n"
                                                      "* Configuration Shell Started *\r\n"
                                                      "*******************************\r\n\r\n";
static const char *cdc_shell_prompt                 = ">";
static const char *cdc_shell_new_line               = "\r\n";

static const char *escape_cursor_forward        = "\033[C";
static const char *escape_cursor_backward       = "\033[D";
static const char *escape_clear_line_to_end     = "\033[0K";

typedef void (*cmd_func_t)(int argc, char *argv[]);

typedef struct {
    char *cmd;
    cmd_func_t handler;
    char *description;
    char *usage;
} cdc_shell_cmd_t;

/* Shell Helper Functions */

int cdc_shell_invoke_command(int argc, char *argv[], const cdc_shell_cmd_t *commands) {
    const cdc_shell_cmd_t *shell_cmd = commands;
    while (shell_cmd->cmd) {
        if (strcmp(shell_cmd->cmd, *argv) == 0) {
            shell_cmd->handler(argc-1, argv+1);
            return 0;
        }
        shell_cmd++;
    }
    return -1;
}

/* Set Commands */

static const char *cdc_shell_err_uart_missing_arguments     = "Error, no arguments, use \"help uart\" for the list of arguments.\r\n";
static const char *cdc_shell_err_uart_invalid_port          = "Error, invalid port number.\r\n";
static const char *cdc_shell_err_uart_unknown_signal        = "Error, unknown signal name.\r\n";
static const char *cdc_shell_err_uart_missing_signame       = "Error, expected \"show\" or a signal name, got nothing.\r\n";
static const char *cdc_shell_err_uart_missing_params        = "Error, missing signal parameters.\r\n";
static const char *cdc_shell_err_uart_missing_output_type   = "Error, missing output type.\r\n";
static const char *cdc_shell_err_uart_invalid_output_type   = "Error, invalid output type.\r\n";
static const char *cdc_shell_err_uart_missing_polarity      = "Error, missing polarity.\r\n";
static const char *cdc_shell_err_uart_invalid_polarity      = "Error, invalid polarity.\r\n";
static const char *cdc_shell_err_uart_missing_pull_type     = "Error, missing pull type.\r\n";
static const char *cdc_shell_err_uart_invalid_pull_type     = "Error, invalid pull type.\r\n";

static void cdc_shell_cmd_uart_show(int port) {
    for (int port_index = ((port == -1) ? 0 : port);
             port_index < ((port == -1) ? USB_CDC_NUM_PORTS : port + 1);
             port_index++) {
        cdc_shell_write("show stub\r\n", strlen("show stub\r\n"));
    }
}

static void cdc_shell_cmd_uart_set_output_type(int port, cdc_pin_t uart_signal, gpio_output_t output_type) {
    for (int port_index = ((port == -1) ? 0 : port);
             port_index < ((port == -1) ? USB_CDC_NUM_PORTS : port + 1);
             port_index++) {
        cdc_shell_write("set output type stub\r\n", strlen("set output type stub\r\n"));
    }
}

static void cdc_shell_cmd_uart_set_polarity(int port, cdc_pin_t uart_signal, gpio_polarity_t polarity) {
    for (int port_index = ((port == -1) ? 0 : port);
             port_index < ((port == -1) ? USB_CDC_NUM_PORTS : port + 1);
             port_index++) {
        cdc_shell_write("set polarity stub\r\n", strlen("set polarity stub\r\n"));
    }
}

static void cdc_shell_cmd_uart_set_pull_type(int port, cdc_pin_t uart_signal, gpio_pull_t polarity) {
    for (int port_index = ((port == -1) ? 0 : port);
             port_index < ((port == -1) ? USB_CDC_NUM_PORTS : port + 1);
             port_index++) {
        cdc_shell_write("set pull stub\r\n", strlen("set pull stub\r\n"));
    }
}

static const char *_cdc_uart_signal_names[cdc_pin_last] = {
    "rx", "tx", "rts", "cts", "dsr", "dtr", "dcd", 
};

static cdc_pin_t _cdc_uart_signal_by_name(char *name) {
    for (int i = 0; i < sizeof(_cdc_uart_signal_names)/sizeof(*_cdc_uart_signal_names); i++) {
        if (strcmp(name, _cdc_uart_signal_names[i]) == 0) {
            return (cdc_pin_t)i;
        }
    }
    return cdc_pin_unknown;
}

static const char *_cdc_uart_output_types[gpio_output_last] = {
    "pp", "od",
};

static gpio_output_t _cdc_uart_output_type_by_name(char *name) {
    for (int i = 0; i< sizeof(_cdc_uart_output_types)/sizeof(*_cdc_uart_output_types); i++) {
        if (strcmp(name, _cdc_uart_output_types[i]) == 0) {
            return (gpio_output_t)i;
        }
    }
    return gpio_output_unknown;
}

static const char *_cdc_uart_polarities[gpio_polarity_last] = {
    "high", "low",
};

static gpio_polarity_t _cdc_uart_polarity_by_name(char *name) {
    for (int i = 0; i< sizeof(_cdc_uart_polarities)/sizeof(*_cdc_uart_polarities); i++) {
        if (strcmp(name, _cdc_uart_polarities[i]) == 0) {
            return (gpio_polarity_t)i;
        }
    }
    return gpio_polarity_unknown;
}

static const char *_cdc_uart_pull_types[gpio_pull_last] = {
    "floating", "up", "down",
};

static gpio_pull_t _cdc_uart_pull_type_by_name(char *name) {
    for (int i = 0; i< sizeof(_cdc_uart_pull_types)/sizeof(*_cdc_uart_pull_types); i++) {
        if (strcmp(name, _cdc_uart_pull_types[i]) == 0) {
            return (gpio_pull_t)i;
        }
    }
    return gpio_pull_unknown;
}

static void cdc_shell_cmd_uart(int argc, char *argv[]) {
    if (argc--) {
        int port;
        if (strcmp(*argv, "all") == 0) {
            port = -1;
        } else if (((port = atoi(*argv)) < 1) || port > USB_CDC_NUM_PORTS) {
            cdc_shell_write(cdc_shell_err_uart_invalid_port, strlen(cdc_shell_err_uart_invalid_port));
            return;
        }
        argv++;
        if (argc) {
            if (strcmp(*argv, "show") == 0) {
                cdc_shell_cmd_uart_show(port);
            } else {
                while(argc) {
                    argc--;
                    cdc_pin_t uart_signal = _cdc_uart_signal_by_name(*argv);
                    if (uart_signal == cdc_pin_unknown) {
                        cdc_shell_write(cdc_shell_err_uart_unknown_signal, strlen(cdc_shell_err_uart_unknown_signal));
                        return;
                    }
                    argv++;
                    if (argc) {
                        while(argc) {
                            if (strcmp(*argv, "output") == 0) {
                                argc--;
                                argv++;
                                if (argc) {
                                    gpio_output_t output_type = _cdc_uart_output_type_by_name(*argv);
                                    if (output_type != gpio_output_unknown) {
                                        argc--;
                                        argv++;
                                        cdc_shell_cmd_uart_set_output_type(port, uart_signal, output_type);
                                    } else {
                                        cdc_shell_write(cdc_shell_err_uart_invalid_output_type, strlen(cdc_shell_err_uart_invalid_output_type));
                                        return;
                                    }
                                } else {
                                    cdc_shell_write(cdc_shell_err_uart_missing_output_type, strlen(cdc_shell_err_uart_missing_output_type));
                                    return;
                                }
                            } else if (strcmp(*argv, "active") == 0) {
                                argc--;
                                argv++;
                                if (argc) {
                                    gpio_polarity_t polarity = _cdc_uart_polarity_by_name(*argv);
                                    if (polarity != gpio_polarity_unknown) {
                                        argc--;
                                        argv++;
                                        cdc_shell_cmd_uart_set_polarity(port, uart_signal, polarity);
                                    } else {
                                        cdc_shell_write(cdc_shell_err_uart_invalid_polarity, strlen(cdc_shell_err_uart_invalid_polarity));
                                        return;
                                    }
                                } else {
                                    cdc_shell_write(cdc_shell_err_uart_missing_polarity, strlen(cdc_shell_err_uart_missing_polarity));
                                    return;
                                }
                            } else if (strcmp(*argv, "pull") == 0) {
                                argc--;
                                argv++;
                                if (argc) {
                                    gpio_pull_t pull = _cdc_uart_pull_type_by_name(*argv);
                                    if (pull != gpio_pull_unknown) {
                                        argc--;
                                        argv++;
                                        cdc_shell_cmd_uart_set_pull_type(port, uart_signal, pull);
                                    } else {
                                        cdc_shell_write(cdc_shell_err_uart_invalid_pull_type, strlen(cdc_shell_err_uart_invalid_pull_type));
                                        return;
                                    }
                                } else {
                                    cdc_shell_write(cdc_shell_err_uart_missing_pull_type, strlen(cdc_shell_err_uart_missing_pull_type));
                                    return;
                                }
                            } else {
                                break;
                            }
                        }
                    } else {
                        cdc_shell_write(cdc_shell_err_uart_missing_params, strlen(cdc_shell_err_uart_missing_params));            
                    }
                }
            }
        } else {
            cdc_shell_write(cdc_shell_err_uart_missing_signame, strlen(cdc_shell_err_uart_missing_signame));
        }
    } else {
        cdc_shell_write(cdc_shell_err_uart_missing_arguments, strlen(cdc_shell_err_uart_missing_arguments));
    } 
}

static void cdc_shell_cmd_help(int argc, char *argv[]);

static const cdc_shell_cmd_t cdc_shell_commands[] = {
    { 
        .cmd            = "help",
        .handler        = cdc_shell_cmd_help,
        .description    = "shows this help message, use \"help command-name\" to get command-specific help",
        .usage          = "Usage: help [command-name]",
    },
    { 
        .cmd            = "uart",
        .handler        = cdc_shell_cmd_uart,
        .description    = "set and view UART parameters",
        .usage          = "Usage: uart port-number|all show|signal-name-1 param-1 value-1 ... [param-n value-n] [signal-name-2 ...]\r\n"
                          "Use uart port-number|all show, to view current UART configuration.\r\n"
                          "Use uart port-number|all signal-name-1 param-1 value-1 ... [param-n value-n] [signal-name-2 ...]\r\n"
                          "to set UART parameters, where signal names are rx, tx, rts, cts, dsr, dtr, dcd,\r\n"
                          "and params are:\r\n"
                          "  output\t[pp|od]\r\n"
                          "  active\t[low|high]\r\n"
                          "  pull\t\t[floating|up|down]",
    },
    { 0 }
};

/* Global Commands */

static const char *cdc_shell_err_no_help = "Error, no help for this command, use \"help\" to get the list of available commands.\r\n";

static void cdc_shell_cmd_help(int argc, char *argv[]) {
    const cdc_shell_cmd_t *cmd = cdc_shell_commands;
    while (cmd->cmd) {
        if (argc) {
            if (strcmp(*argv, cmd->cmd) == 0) {
                const char *delim = ": ";
                cdc_shell_write(cmd->cmd, strlen(cmd->cmd));
                cdc_shell_write(delim, strlen(delim));
                cdc_shell_write(cmd->description, strlen(cmd->description));
                cdc_shell_write(cdc_shell_new_line, strlen(cdc_shell_new_line));
                cdc_shell_write(cmd->usage, strlen(cmd->usage));
                cdc_shell_write(cdc_shell_new_line, strlen(cdc_shell_new_line));
                break;
            }
        } else {
            const char *delim = "\t- ";
            cdc_shell_write(cmd->cmd, strlen(cmd->cmd));
            cdc_shell_write(delim, strlen(delim));
            cdc_shell_write(cmd->description, strlen(cmd->description));
            cdc_shell_write(cdc_shell_new_line, strlen(cdc_shell_new_line));
        }
        cmd++;
    }
    if (argc && (cmd->cmd == 0)) {
        cdc_shell_write(cdc_shell_err_no_help, strlen(cdc_shell_err_no_help));
    }
}

static const char *cdc_shell_err_unknown_command = "Error, unknown command, use \"help\" to get the list of available commands.\r\n";

static void cdc_shell_exec_command(int argc, char *argv[]) {
    if (cdc_shell_invoke_command(argc, argv, cdc_shell_commands) == -1) {
        cdc_shell_write(cdc_shell_err_unknown_command, strlen(cdc_shell_err_unknown_command));
    }
}

static const char *cdc_shell_err_too_long       = "Error, command line is too long.\r\n";
static const char *cdc_shell_err_too_many_args  = "Error, too many command line arguments.\r\n";

static void cdc_shell_parse_command_line(char *cmd_line) {
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
static char cmd_prev_line_buf[USB_SHELL_MAX_CMD_LINE_SIZE];
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
    memset(cmd_prev_line_buf, 0, sizeof(cmd_prev_line_buf));
    cdc_shell_state = cdc_shell_idle;
    cdc_shell_write(cdc_shell_banner, strlen(cdc_shell_banner));
    cdc_shell_write(cdc_shell_prompt, strlen(cdc_shell_prompt));
}

#define ANSI_CTRLSEQ_ESCAPE_CHAR    0x1B
#define ANSI_CTRLSEQ_ESCAPE_CSI     0x5B
#define ANSI_CTRLSEQ_CUU            0x41
#define ANSI_CTRLSEQ_CUD            0x42
#define ANSI_CTRLSEQ_CUF            0x43
#define ANSI_CTRLSEQ_CUB            0x44


static void cdc_shell_cursor_move_back(int n_symb) {
    if (n_symb) {
        char n_symb_str[8];
        itoa(n_symb, n_symb_str, 10);
        cdc_shell_write("\033[", 2);
        cdc_shell_write(n_symb_str, strlen(n_symb_str));
        cdc_shell_write("D", 1);
    }
}

static void cdc_shell_handle_backspace() {
    if (cmd_line_cursor > cmd_line_buf) {
        cdc_shell_cursor_move_back(cmd_line_cursor - cmd_line_buf);
        cdc_shell_write(escape_clear_line_to_end, strlen(escape_clear_line_to_end));
        cmd_line_cursor--;
        memmove(cmd_line_cursor, cmd_line_cursor+1, strlen(cmd_line_cursor));
        cdc_shell_write(cmd_line_buf, strlen(cmd_line_buf));
        cdc_shell_cursor_move_back(strlen(cmd_line_buf) - (cmd_line_cursor - cmd_line_buf));
    }
}

static void cdc_shell_insert_symbol(char c) {
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
                } else if (*buf_p == ANSI_CTRLSEQ_CUU) {
                    size_t prev_cmd_len = strlen(cmd_prev_line_buf);
                    if (prev_cmd_len) {
                        strcpy(cmd_line_buf, cmd_prev_line_buf);
                        cdc_shell_cursor_move_back(cmd_line_cursor - cmd_line_buf);
                        cdc_shell_write(escape_clear_line_to_end, strlen(escape_clear_line_to_end));
                        cmd_line_cursor = cmd_line_buf + prev_cmd_len;
                        cdc_shell_write(cmd_line_buf, prev_cmd_len);
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
                if (cmd_line_cursor != cmd_line_buf) {
                    strcpy(cmd_prev_line_buf, cmd_line_buf);
                }
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
