/*
 * MIT License
 *
 * Copyright (c) 2020 Kirill Kotyagin
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "usb_cdc.h"
#include "gpio.h"
#include "cdc_config.h"
#include "device_config.h"
#include "default_config.h"
#include "version.h"
#include "cdc_shell.h"


static const char cdc_shell_banner[]                = "\r\n\r\n"
                                                      "*******************************\r\n"
                                                      "* Configuration Shell Started *\r\n"
                                                      "*******************************\r\n\r\n";
static const char cdc_shell_prompt[]                = ">";
static const char cdc_shell_new_line[]              = "\r\n";
static const char cdc_shell_delim[]                 = "\t- ";

static const char escape_cursor_forward[]       = "\033[C";
static const char escape_cursor_backward[]      = "\033[D";
static const char escape_clear_line_to_end[]    = "\033[0K";

typedef void (*cmd_func_t)(int argc, char *argv[]);

typedef struct {
    char *cmd;
    cmd_func_t handler;
    char *description;
    char *usage;
} cdc_shell_cmd_t;

/* Shell Helper Functions */

__noinline static void cdc_shell_write_string(const char *buf) {
    cdc_shell_write(buf, strlen(buf));
}

__noinline static void cdc_shell_msg(const char *fmt, ...) {
    char buf[1024];
    int len = 0;

    if (fmt != 0) {
        va_list va;
        va_start(va, fmt);
        len = vsnprintf(buf, sizeof(buf) - 2, fmt, va);
        va_end(va);
    }

    if (len >= 0) {
        if (len < 2 || ( buf[len - 2] != '\r' && buf[len - 2] != '\n')) {
            buf[len++] = '\r';
            buf[len++] = '\n';
        }
        cdc_shell_write(buf, len);
    }
}

static int cdc_shell_invoke_command(int argc, char *argv[], const cdc_shell_cmd_t *commands) {
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

static const char cdc_shell_err_uart_missing_arguments[]            = "Error, no arguments, use \"help uart\" for the list of arguments.\r\n";
static const char cdc_shell_err_uart_invalid_uart[]                 = "Error, invalid UART number.\r\n";
static const char cdc_shell_err_uart_unknown_signal[]               = "Error, unknown signal name.\r\n";
static const char cdc_shell_err_uart_missing_signame[]              = "Error, expected \"show\" or a signal name, got nothing.\r\n";
static const char cdc_shell_err_uart_missing_params[]               = "Error, missing signal parameters.\r\n";
static const char cdc_shell_err_uart_missing_output_type[]          = "Error, missing output type.\r\n";
static const char cdc_shell_err_uart_invalid_output_type[]          = "Error, invalid output type.\r\n";
static const char cdc_shell_err_uart_missing_polarity[]             = "Error, missing polarity.\r\n";
static const char cdc_shell_err_uart_invalid_polarity[]             = "Error, invalid polarity.\r\n";
static const char cdc_shell_err_uart_missing_pull_type[]            = "Error, missing pull type.\r\n";
static const char cdc_shell_err_uart_invalid_pull_type[]            = "Error, invalid pull type.\r\n";
static const char cdc_shell_err_cannot_set_output_type_for_input[]  = "Error, cannot set output type for input pin.\r\n";
static const char cdc_shell_err_cannot_change_polarity[]            = "Error, cannot change polarity of alternate function pins.\r\n";
static const char cdc_shell_err_cannot_set_pull_for_output[]        = "Error, cannot pull type for output pin.\r\n";
static const char cdc_shell_err_pin_is_detached[]                   = "Error, pin is detached.\r\n";


static const char *_cdc_uart_signal_names[cdc_pin_last] = {
    "rx", "tx", "rts", "cts", "dsr", "dtr", "dcd", "ri", "txa",
};

static cdc_pin_t _cdc_uart_signal_by_name(char *name) {
    for (int i = 0; i < ARRAY_SIZE(_cdc_uart_signal_names); i++) {
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
    for (int i = 0; i < ARRAY_SIZE(_cdc_uart_output_types); i++) {
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
    for (int i = 0; i < ARRAY_SIZE(_cdc_uart_polarities); i++) {
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
    for (int i = 0; i < ARRAY_SIZE(_cdc_uart_pull_types); i++) {
        if (strcmp(name, _cdc_uart_pull_types[i]) == 0) {
            return (gpio_pull_t)i;
        }
    }
    return gpio_pull_unknown;
}

static void cdc_shell_cmd_uart_show(int port) {
    const char *uart_str = "UART";
    const char *na_str = "n/a";
    const char *in_str = "in, ";
    const char *out_str = "out, ";
    const char *active_str = "active ";
    const char *pull_str = "pull ";
    const char *output_str = "output ";
    const char *comma_str = ", ";
    const char *colon_str = ":";
    char port_index_str[32];
    for (int port_index = ((port == -1) ? 0 : port);
             port_index < ((port == -1) ? USB_CDC_NUM_PORTS : port + 1);
             port_index++) {
        const cdc_port_t *cdc_port = &device_config_get()->cdc_config.port_config[port_index];
        itoa(port_index+1, port_index_str, 10);
        cdc_shell_write_string(uart_str);
        cdc_shell_write_string(port_index_str);
        cdc_shell_write_string(colon_str);
        cdc_shell_write_string(cdc_shell_new_line);
        for (cdc_pin_t pin = 0; pin < cdc_pin_last; pin++) {
            const gpio_pin_t *cdc_pin = gpion_to_gpio(cdc_port->pins[pin]);
            gpio_hal_t hal = gpion_to_hal(cdc_port->pins[pin]);
            const char *pin_name = _cdc_uart_signal_names[pin];
            cdc_shell_write_string(pin_name);
            cdc_shell_write_string(cdc_shell_delim);
            if (cdc_pin && hal.port) {
                const char *active_value = _cdc_uart_polarities[cdc_pin->polarity];
                if (cdc_pin->dir == gpio_dir_input) {
                    cdc_shell_write_string(in_str);
                } else {
                    cdc_shell_write_string(out_str);
                }
                cdc_shell_write_string(active_str);
                cdc_shell_write_string(active_value);
                cdc_shell_write_string(comma_str);
                if (cdc_pin->dir == gpio_dir_input) {
                    const char *pull_value = _cdc_uart_pull_types[cdc_pin->pull];
                    cdc_shell_write_string(pull_str);
                    cdc_shell_write_string(pull_value);
                } else {
                    const char *output_value = _cdc_uart_output_types[cdc_pin->output];
                    cdc_shell_write_string(output_str);
                    cdc_shell_write_string(output_value);
                }
            } else {
                cdc_shell_write_string(na_str);
            }
            cdc_shell_write_string(cdc_shell_new_line);
        }
    }
}

static int cdc_shell_cmd_uart_set_output_type(int port, cdc_pin_t uart_pin, gpio_output_t output) {
    for (int port_index = ((port == -1) ? 0 : port);
             port_index < ((port == -1) ? USB_CDC_NUM_PORTS : port + 1);
             port_index++) {
        gpio_pin_t *pin = gpion_to_gpio(device_config_get()->cdc_config.port_config[port_index].pins[uart_pin]);
        if (pin == 0) {
            cdc_shell_write_string(cdc_shell_err_pin_is_detached);
        } else if (pin->dir == gpio_dir_output) {
            pin->output = output;
            usb_cdc_reconfigure_port_pin(port, uart_pin);
        } else {
            cdc_shell_write_string(cdc_shell_err_cannot_set_output_type_for_input);
            return -1;
        }
    }
    return 0;
}

static int cdc_shell_cmd_uart_set_polarity(int port, cdc_pin_t uart_pin, gpio_polarity_t polarity) {
    for (int port_index = ((port == -1) ? 0 : port);
             port_index < ((port == -1) ? USB_CDC_NUM_PORTS : port + 1);
             port_index++) {
        gpio_pin_t *pin = gpion_to_gpio(device_config_get()->cdc_config.port_config[port_index].pins[uart_pin]);
        if (pin->func == gpio_func_general && (uart_pin != cdc_pin_rx) && (uart_pin != cdc_pin_cts)) {
            pin->polarity = polarity;
            usb_cdc_reconfigure_port_pin(port, uart_pin);
        } else {
            cdc_shell_write_string(cdc_shell_err_cannot_change_polarity);
            return -1;
        }
    }
    return 0;
}

static int cdc_shell_cmd_uart_set_pull_type(int port, cdc_pin_t uart_pin, gpio_pull_t pull) {
    for (int port_index = ((port == -1) ? 0 : port);
             port_index < ((port == -1) ? USB_CDC_NUM_PORTS : port + 1);
             port_index++) {
        gpio_pin_t *pin = gpion_to_gpio(device_config_get()->cdc_config.port_config[port_index].pins[uart_pin]);
        if (pin->dir == gpio_dir_input) {
            pin->pull = pull;
            usb_cdc_reconfigure_port_pin(port, uart_pin);
        } else {
            cdc_shell_write_string(cdc_shell_err_cannot_set_pull_for_output);
            return -1;
        }
    }
    return 0;
}

static void cdc_shell_cmd_uart(int argc, char *argv[]) {
    if (argc--) {
        int port;
        if (strcmp(*argv, "all") == 0) {
            port = -1;
        } else {
            if (((port = atoi(*argv)) < 1) || port > USB_CDC_NUM_PORTS) {
                cdc_shell_write_string(cdc_shell_err_uart_invalid_uart);
                return;
            }
            port = port - 1;
        }
        argv++;
        if (argc) {
            if (strcmp(*argv, "show") == 0) {
                cdc_shell_cmd_uart_show(port);
            } else {
                while(argc) {
                    argc--;
                    cdc_pin_t uart_pin = _cdc_uart_signal_by_name(*argv);
                    if (uart_pin == cdc_pin_unknown) {
                        cdc_shell_write_string(cdc_shell_err_uart_unknown_signal);
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
                                        if (cdc_shell_cmd_uart_set_output_type(port, uart_pin, output_type) == -1) {
                                            return;
                                        }
                                    } else {
                                        cdc_shell_write_string(cdc_shell_err_uart_invalid_output_type);
                                        return;
                                    }
                                } else {
                                    cdc_shell_write_string(cdc_shell_err_uart_missing_output_type);
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
                                        if (cdc_shell_cmd_uart_set_polarity(port, uart_pin, polarity) == -1) {
                                            return;
                                        }
                                    } else {
                                        cdc_shell_write_string(cdc_shell_err_uart_invalid_polarity);
                                        return;
                                    }
                                } else {
                                    cdc_shell_write_string(cdc_shell_err_uart_missing_polarity);
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
                                        if (cdc_shell_cmd_uart_set_pull_type(port, uart_pin, pull) == -1) {
                                            return;
                                        }
                                    } else {
                                        cdc_shell_write_string(cdc_shell_err_uart_invalid_pull_type);
                                        return;
                                    }
                                } else {
                                    cdc_shell_write_string(cdc_shell_err_uart_missing_pull_type);
                                    return;
                                }
                            } else {
                                break;
                            }
                        }
                    } else {
                        cdc_shell_write_string(cdc_shell_err_uart_missing_params);
                    }
                }
            }
        } else {
            cdc_shell_write_string(cdc_shell_err_uart_missing_signame);
        }
    } else {
        cdc_shell_write_string(cdc_shell_err_uart_missing_arguments);
    }
}

/* Gpio macro helpers */

/* Wrapper on cdc_shell_msg, which prints message about specified pin */
#define cdc_shell_msg_pin(pinn, fmt, ...) cdc_shell_msg("%6s [%d]: " fmt,      \
                    gpion_to_str(pinn), gpion_pin_get_free(pinn, 0),           \
                    ## __VA_ARGS__)

/* Extra special numbers of pins which specifies the groups of pins. The numbers
 * lies after gpio_pin_last and used withing shell processing functions to apply
 * massive operations. */
#define cdc_shell_gpion_pin_ref_all      cdc_shell_gpion_pin_ref(1)
#define cdc_shell_gpion_pin_ref_free     cdc_shell_gpion_pin_ref(2)
#define cdc_shell_gpion_pin_ref_blocked  cdc_shell_gpion_pin_ref(3)
#define cdc_shell_gpion_pin_ref_occupied cdc_shell_gpion_pin_ref(4)
#define cdc_shell_gpion_pin_ref_uart1    cdc_shell_gpion_pin_ref(5)
#define cdc_shell_gpion_pin_ref_uart2    cdc_shell_gpion_pin_ref(6)
#define cdc_shell_gpion_pin_ref_uart3    cdc_shell_gpion_pin_ref(7)
#define cdc_shell_gpion_pin_ref_uart     cdc_shell_gpion_pin_ref_uart1

/* This macro will check pinn argument and if it equals to one of defined from
 * above number, it will call the func for each pinn in group. This macro is
 * assumed to be called from void-returning function. If pinn was matched the
 * macro will execute return. The func argument must accept the pin number as
 * first argument and the extra VA_ARGS will be passed in tail. */
#define cdc_shell_pin_apply_extras(func, pinn, ...)                            \
    cdc_shell_pin_declare_extras(cdc_shell_pin_declare_extra_func, func, pinn, \
                                 ## __VA_ARGS__)

/* This is helper macro, which check, does the string str match one of gpio
 * groups. If match found - it will set pinn to specific value. */
#define cdc_shell_pin_check_extras(str, pinn)                                  \
    cdc_shell_pin_declare_extras(cdc_shell_pin_check_extra_func,               \
                                 str, pinn)

/* Helper for defining extra pin groups. */
#define cdc_shell_gpion_pin_ref(n) BUILD_BUG_ON_RET(                           \
    ((gpion_pin_t)(gpio_pin_last + n) <= gpio_pin_last),                       \
    (gpion_pin_t)(gpio_pin_last + n))

/* Helper which apply func macro to all pins groups names */
#define cdc_shell_pin_declare_extras(func, ...)  do {                          \
    func(all, ## __VA_ARGS__);                                                 \
    func(free, ## __VA_ARGS__);                                                \
    func(blocked, ## __VA_ARGS__);                                             \
    func(occupied, ## __VA_ARGS__);                                            \
    func(uart1, ## __VA_ARGS__);                                               \
    func(uart2, ## __VA_ARGS__);                                               \
    func(uart3, ## __VA_ARGS__);                                               \
} while (0)

/* Next - goes block of helpers for cdc_shell_pin_apply_extras */

/* Basic helper, which performs check of pinn variable var and cicles through
 * all pins to apply predicate to them to call the func. */
#define cdc_shell_pin_extra_func(func, var, pred, name, ...) do {              \
    if ((var) == cdc_shell_gpion_pin_ref_ ## name) {                           \
        for (gpion_pin_t __pinn = 0; __pinn < gpio_pin_last; ++__pinn) {       \
            if ((pred)) {                                                      \
                (void) (func(__pinn, ## __VA_ARGS__));                         \
            }                                                                  \
        }                                                                      \
        return;                                                                \
    }                                                                          \
} while (0)

/* This helper implements predicate for groups based on pins' status field */
#define cdc_shell_pin_extra_func_status(func, var, status_val, name, ...) do { \
    device_config_t *__device_config = device_config_get();                    \
    cdc_shell_pin_extra_func(func, var,                                        \
        __device_config->gpio_config.pins[__pinn].status == status_val, name,  \
        ## __VA_ARGS__);                                                       \
} while (0)

/* This helper implements predicate for groups based on pins' belonging to uart
 * port */
#define cdc_shell_pin_extra_func_uart(func, var, port_num, ...) do {           \
    cdc_shell_pin_extra_func(func, var,                                        \
        gpion_to_cdc(__pinn).port == (port_num) - 1,                           \
        uart ## port_num, ## __VA_ARGS__);                                     \
} while (0)

/* Next macros implements the checkers for all available pins groups */
#define cdc_shell_pin_extra_func_all(func, var, ...)                           \
    cdc_shell_pin_extra_func(func, var, 1, all, ## __VA_ARGS__)

#define cdc_shell_pin_extra_func_free(func, var, ...)                          \
    cdc_shell_pin_extra_func_status(func, var, gpio_status_free, free,         \
                                    ## __VA_ARGS__)

#define cdc_shell_pin_extra_func_blocked(func, var, ...)                       \
    cdc_shell_pin_extra_func_status(func, var, gpio_status_blocked, blocked,   \
                                    ## __VA_ARGS__)

#define cdc_shell_pin_extra_func_occupied(func, var, ...)                      \
    cdc_shell_pin_extra_func_status(func, var, gpio_status_occupied, occupied, \
                                    ## __VA_ARGS__)

#define cdc_shell_pin_extra_func_uart1(func, var, ...)                         \
    cdc_shell_pin_extra_func_uart(func, var, 1, ## __VA_ARGS__)

#define cdc_shell_pin_extra_func_uart2(func, var, ...)                         \
    cdc_shell_pin_extra_func_uart(func, var, 2, ## __VA_ARGS__)

#define cdc_shell_pin_extra_func_uart3(func, var, ...)                         \
    cdc_shell_pin_extra_func_uart(func, var, 3, ## __VA_ARGS__)

/* This helper just calls one of previously defined macros */
#define cdc_shell_pin_declare_extra_func(name, ...)                            \
    cdc_shell_pin_extra_func_ ## name(__VA_ARGS__)

/* Helper for cdc_shell_pin_apply_extras */
#define cdc_shell_pin_check_extra_func(name, check_var, pin_var) do {          \
    if (pin_var == gpio_pin_unknown && !strcmp(check_var, #name)) {            \
        (pin_var) = cdc_shell_gpion_pin_ref_ ## name;                          \
    }                                                                          \
} while (0)


/* TODO(Shvedov) next macros and function cdc_shell_cmd_gpio_show_pin_controlled
 * should be refactored and joined with cdc_shell_cmd_uart_show. */
#define cdc_shell_gpio_mode_declare_stringify(name, vals) \
static const char *cdc_shell_gpio_stringify_ ## name(gpio_ ## name ##_t val) { \
    static const char *strs[] = vals; \
    if (val < ARRAY_SIZE(strs)) { \
        return strs[val]; \
    } else { \
        return 0; \
    } \
}

#define gpio_mode_dir_strings { [gpio_dir_input] = "in", [gpio_dir_output] = "out", }
cdc_shell_gpio_mode_declare_stringify(dir, gpio_mode_dir_strings)

#define gpio_mode_func_strings { [gpio_func_general] = "gpio", [gpio_func_alternate] = "alt", }
cdc_shell_gpio_mode_declare_stringify(func, gpio_mode_func_strings)

#define gpio_mode_output_strings { [gpio_output_pp] = "pp", [gpio_output_od] = "od", }
cdc_shell_gpio_mode_declare_stringify(output, gpio_mode_output_strings)

#define gpio_mode_pull_strings { [gpio_pull_floating] = "pullfloat", \
                                 [gpio_pull_up] = "pullup", \
                                 [gpio_pull_down] = "pulldown", }
cdc_shell_gpio_mode_declare_stringify(pull, gpio_mode_pull_strings)

#define gpio_mode_polarity_strings { [gpio_polarity_high] = "polar_high", [gpio_polarity_low] = "polar_low", }
cdc_shell_gpio_mode_declare_stringify(polarity, gpio_mode_polarity_strings)

#define gpio_mode_speed_strings { [gpio_speed_low] = "slow", \
                                  [gpio_speed_high] = "fast", \
                                  [gpio_speed_medium] = "speed_medium", }
cdc_shell_gpio_mode_declare_stringify(speed, gpio_mode_speed_strings)

static void cdc_shell_cmd_gpio_show_pin_controlled(gpion_pin_t pinn, int show_info) {
    gpio_pin_t *pin = gpion_to_gpio(pinn);
    cdc_pin_ref_t cpin = gpion_to_cdc(pinn);
    char info_str[64] = "";
    if (show_info) {

        const char *dir = cdc_shell_gpio_stringify_dir(pin->dir);
        const char *func = cdc_shell_gpio_stringify_func(pin->func);
        const char *output = cdc_shell_gpio_stringify_output(pin->output);
        const char *pull = cdc_shell_gpio_stringify_pull(pin->pull);
        const char *speed = cdc_shell_gpio_stringify_speed(pin->speed);
        const char *polarity = cdc_shell_gpio_stringify_polarity(pin->polarity);

#define str_val(var) var ? ", " : "", var ? var : ""
#define str_val_func(var) var ? "" : " invalid func ", var ? var : ""
#define str_mod_fmt "%s%s%s%s%s%s%s%s%s%s%s%s"
#define str_mod_vals str_val_func(func), str_val(dir), str_val(output), \
                     str_val(pull), str_val(speed), str_val(polarity)
        snprintf(info_str, sizeof(info_str), "\t- " str_mod_fmt, str_mod_vals);
    }

    if (pin->status == gpio_status_free) {
        cdc_shell_msg_pin(pinn, "free%s", info_str);
    } else if (cpin.port < 0 || cpin.port >= USB_CDC_NUM_PORTS || cpin.pin >= cdc_pin_last) {
        device_config_t *config = device_config_get();
        if (config->status_led_pin == pinn) {
            cdc_shell_msg_pin(pinn, "status led%s", info_str);
        } else if (config->config_pin == pinn) {
            cdc_shell_msg_pin(pinn, "configuration control%s", info_str);
        } else {
            cdc_shell_msg_pin(pinn, "got invalid occupied pin");
        }
    } else {
        cdc_shell_msg_pin(pinn, "uart%d-%s%s", cpin.port + 1,
                          _cdc_uart_signal_names[cpin.pin], info_str);
    }
}
static void cdc_shell_cmd_gpio_show_ex(gpion_pin_t pinn, int show_info) {
    cdc_shell_pin_apply_extras(cdc_shell_cmd_gpio_show_ex, pinn, show_info);
    gpio_pin_t *pin = gpion_to_gpio(pinn);

    if (!pin) {
        cdc_shell_msg("Invalid pin number \"%d\"", pinn);
        return;
    }

    switch (pin->status) {
    case gpio_status_blocked:
        cdc_shell_msg_pin(pinn, "%s", default_config_get_blocked_reason(pinn));
        break;
    case gpio_status_occupied:
    case gpio_status_free:
        cdc_shell_cmd_gpio_show_pin_controlled(pinn, show_info);
        break;
    default:
        cdc_shell_msg_pin(pinn, "unknown");
        break;
    }
}

static void cdc_shell_cmd_gpio_count(gpion_pin_t pinn, unsigned *out_count) {
    cdc_shell_pin_apply_extras(cdc_shell_cmd_gpio_count, pinn, out_count);
    *out_count += 1;
}
static void cdc_shell_cmd_gpio_show(gpion_pin_t pinn) {
    unsigned count_pins = 0;
    cdc_shell_cmd_gpio_count(pinn, &count_pins);
    cdc_shell_cmd_gpio_show_ex(pinn, count_pins <= cdc_pin_last);
}

static void cdc_shell_cmd_gpio_set_status(gpion_pin_t pinn, gpio_status_t status) {
    cdc_shell_pin_apply_extras(cdc_shell_cmd_gpio_set_status, pinn, status);
    gpio_pin_set_status(pinn, status);
}

static void cdc_shell_cmd_gpio_get(gpion_pin_t pinn) {
    cdc_shell_pin_apply_extras(cdc_shell_cmd_gpio_get, pinn);
    const char *value = "unavailable";
    const gpio_pin_t *pin = gpion_to_gpio(pinn);
    if (!pin) {
        cdc_shell_msg_pin(pinn, "Invalid pin");
        return;
    }
    if (pin->status == gpio_status_free || (pin->status == gpio_status_occupied &&
            pin->func == gpio_func_general))
    {
        value = gpio_pin_get(pin) ? "up" : "down";
    }

    cdc_shell_msg_pin(pinn, "%s", value);
}

static void cdc_shell_cmd_gpio_set(gpion_pin_t pinn, int value) {
    if (value < 0) {
        return;
    }
    cdc_shell_pin_apply_extras(cdc_shell_cmd_gpio_set, pinn, value);
    gpion_pin_set_free(pinn, value);
}

#define cdc_shell_declare_mode_set(name) \
static void cdc_shell_cmd_gpio_##name(gpion_pin_t pinn, gpio_## name ##_t value) \
{ \
    cdc_shell_pin_apply_extras(cdc_shell_cmd_gpio_##name, pinn, value);\
    gpio_pin_t *pin = gpion_to_gpio(pinn);\
    if (pin->status == gpio_status_free) { \
        pin->name = value;\
    } else { \
        cdc_shell_msg_pin(pinn, "Can not set " #name " on non-free pin"); \
    } \
}
cdc_shell_declare_mode_set(dir)
cdc_shell_declare_mode_set(output)
cdc_shell_declare_mode_set(pull)
cdc_shell_declare_mode_set(polarity)
cdc_shell_declare_mode_set(speed)

static void cdc_shell_cmd_gpio_apply_mode(gpion_pin_t pinn)
{
    cdc_shell_pin_apply_extras(cdc_shell_cmd_gpio_apply_mode, pinn);
    gpio_pin_t *pin = gpion_to_gpio(pinn);
    if (pin->status == gpio_status_free) {
        gpio_pin_init(pin);
    }
}

static void cdc_shell_cmd_gpio_mode_usage(void) {
    static const char usage[] =
        "Usage: gpio PIN set [help|config...]: set configuration value of free pin\r\n"
        "\thelp:\t\tshow this text\r\n"
        "\tout:\t\tset pin mode to output\r\n"
        "\tpp:\t\tset pin output mode to pp\r\n"
        "\tod:\t\tset pin output mode to pd\r\n"
        "\tpullup:\tset pin pull mode to up\r\n"
        "\tpulldown:\tset pin pull mode to down\r\n"
        "\tpullfloat:\tset pin pull mode to float\r\n"
        "\tlow:\t\tset pin polarity to low\r\n"
        "\thigh:\t\tset pin polarity to high\r\n"
        "\tslow:\t\tset pin speed to low\r\n"
        "\tfast:\t\tset pin speed to high\r\n"
        "\tmedium:\t\tset pin speed to medium\r\n"
        ;
    cdc_shell_write_string(usage);
}

static void cdc_shell_cmd_gpio_mode(gpion_pin_t pinn, int argc, char *argv[]) {
    if (argc == 0 || (argc > 0 && !strcasecmp(argv[0], "help"))) {
        cdc_shell_cmd_gpio_mode_usage();
        return;
    }
    for (int i = 0; i < argc; ++i) {
        const char *str = argv[i];
        if (!strcasecmp("out", str)) {
            cdc_shell_cmd_gpio_dir(pinn, gpio_dir_output);
        } else if (!strcasecmp("in", str)) {
            cdc_shell_cmd_gpio_dir(pinn, gpio_dir_input);
        } else if (!strcasecmp("pp", str)) {
            cdc_shell_cmd_gpio_output(pinn, gpio_output_pp);
        } else if (!strcasecmp("od", str)) {
            cdc_shell_cmd_gpio_output(pinn, gpio_output_od);
        } else if (!strcasecmp("pullup", str)) {
            cdc_shell_cmd_gpio_pull(pinn, gpio_pull_up);
        } else if (!strcasecmp("pulldown", str)) {
            cdc_shell_cmd_gpio_pull(pinn, gpio_pull_down);
        } else if (!strcasecmp("pullfloat", str)) {
            cdc_shell_cmd_gpio_pull(pinn, gpio_pull_floating);
        } else if (!strcasecmp("low", str)) {
            cdc_shell_cmd_gpio_polarity(pinn, gpio_polarity_low);
        } else if (!strcasecmp("high", str)) {
            cdc_shell_cmd_gpio_polarity(pinn, gpio_polarity_high);
        } else if (!strcasecmp("slow", str)) {
            cdc_shell_cmd_gpio_speed(pinn, gpio_speed_low);
        } else if (!strcasecmp("fast", str)) {
            cdc_shell_cmd_gpio_speed(pinn, gpio_speed_high);
        } else if (!strcasecmp("medium", str)) {
            cdc_shell_cmd_gpio_speed(pinn, gpio_speed_medium);
        } else {
            cdc_shell_msg("Invalid mode: \"%s\"", str);
        }
    }
    cdc_shell_cmd_gpio_apply_mode(pinn);
}

static void cdc_shell_cmd_gpio(int argc, char *argv[]) {
    gpion_pin_t pin = cdc_shell_gpion_pin_ref_all;
    const char *cmd = "show";
    if (argc > 0)
    {
        pin = gpio_pin_unknown;
        cdc_shell_pin_check_extras(argv[0], pin);
        if (pin == gpio_pin_unknown) {
            pin = str_to_gpion(argv[0]);
            if (pin == gpio_pin_unknown) {
                if (argc == 1 && !strcmp(argv[0], "set")) {
                    cdc_shell_cmd_gpio_mode_usage();
                } else {
                    cdc_shell_msg("Invalid pin '%s'", argv[0]);
                }
                return;
            }
        }
    }
    if (argc > 1) {
        cmd = argv[1];
    }
    if (!strcmp(cmd, "show")) {
        return cdc_shell_cmd_gpio_show(pin);
    } else if (!strcmp(cmd, "free")) {
        return cdc_shell_cmd_gpio_set_status(pin, gpio_status_free);
    } else if (!strcmp(cmd, "occupy")) {
        return cdc_shell_cmd_gpio_set_status(pin, gpio_status_occupied);
    } else if (!strcmp(cmd, "get")) {
        return cdc_shell_cmd_gpio_get(pin);
    } else if (!strcmp(cmd, "up")) {
        return cdc_shell_cmd_gpio_set(pin, 1);
    } else if (!strcmp(cmd, "down")) {
        return cdc_shell_cmd_gpio_set(pin, 0);
    } else if (!strcmp(cmd, "set")) {
        if (argc > 2) {
            return cdc_shell_cmd_gpio_mode(pin, argc - 2, argv + 2);
        }
    } else {
        cdc_shell_msg("Invalid gpio command '%s'", cmd);
    }
}


static const char cdc_shell_err_config_missing_arguments[] = "Error, invalid or missing arguments, use \"help config\" for the list of arguments.\r\n";

static void cdc_shell_cmd_config_save() {
    device_config_save();
}

static void cdc_shell_cmd_config_reset() {
    device_config_reset();
    usb_cdc_reconfigure();
}

static void cdc_shell_cmd_config(int argc, char *argv[]) {
    if (argc == 1) {
        if (strcmp(*argv, "save") == 0) {
            return cdc_shell_cmd_config_save();
        }
        if (strcmp(*argv, "reset") == 0) {
            return cdc_shell_cmd_config_reset();
        }
    }
    cdc_shell_write_string(cdc_shell_err_config_missing_arguments);
}

static const char cdc_shell_device_version[]            = DEVICE_VERSION_STRING;

static void cdc_shell_cmd_version(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    cdc_shell_write_string(cdc_shell_device_version);
    cdc_shell_write_string(cdc_shell_new_line);
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
        .cmd            = "config",
        .handler        = cdc_shell_cmd_config,
        .description    = "save and reset configuration paramters in the device flash memory",
        .usage          = "Usage: config save|reset\r\n"
                          "Use: \"config save\" to permanently save device configuration.\r\n"
                          "Use: \"config reset\" to reset device configuration to default.",
    },
    {
        .cmd            = "uart",
        .handler        = cdc_shell_cmd_uart,
        .description    = "set and view UART parameters",
        .usage          = "Usage: uart port-number|all show|signal-name-1 param-1 value-1 ... [param-n value-n] [signal-name-2 ...]\r\n"
                          "Use \"uart port-number|all show\" to view current UART configuration.\r\n"
                          "Use \"uart port-number|all signal-name-1 param-1 value-1 ... [param-n value-n] [signal-name-2 ...]\"\r\n"
                          "to set UART parameters, where signal names are rx, tx, rts, cts, dsr, dtr, dcd, ri, txa,\r\n"
                          "and params are:\r\n"
                          "  output\t[pp|od]\r\n"
                          "  active\t[low|high]\r\n"
                          "  pull\t\t[floating|up|down]\r\n"
                          "Example: \"uart 1 tx output od\" sets UART1 TX output type to open-drain\r\n"
                          "Example: \"uart 3 rts active high dcd active high pull down\" allows to set multiple parameters at once.",
    },
    {
        .cmd            = "gpio",
        .handler        = cdc_shell_cmd_gpio,
        .description    = "set and view GPIO parameters",
        .usage          = "Usage: gpio [pin-name|all|free|blocked|occupied|uartN [command [command_ops]]]\r\n"
                          "pin-name is [gpio_][pin_][p]XN, where X is port name a|b|c, N is pin number in port \r\n"
                          "\tall(default):\tapply command to all gpio pins \r\n"
                          "\tfree:\t\tapply command to all free pins \r\n"
                          "\tblocked:\tapply command to all blocked pins \r\n"
                          "\toccupied:\tapply command to all occupied pins \r\n"
                          "\tuartN:\t\tapply command to all pins, which may be used by uartN, N is one of 1,2,3 \r\n"
                          "\tled:\t\talias for pb13\r\n"
                          "\tshell:\r\n"
                          "\tcontrol:\r\n"
                          "\tconfig:\t\taliases for pb5\r\n"
                          "commands are:\r\n"
                          "\tshow(default):\tshow current usage of pin\r\n"
                          "\tfree:\t\tmake pin free from uart port\r\n"
                          "\toccupy:\t\tattach pin to its uart port\r\n"
                          "\tget:\t\tget value of pin if available\r\n"
                          "\tup:\t\tset free pin up\r\n"
                          "\tdown:\t\tset free pin down\r\n"
                          "\tset [help|config...]: set configuration value of free pin\r\n"
                          "Example: \"gpio\" shows all gpio pins and their status\r\n"
                          "Example: \"gpio all free\" detach all non-blocked pins from their functions\r\n"
                          "Example: \"uart pin_a10 up\" sets gpio pin pa10 up",
    },
    {
        .cmd            = "version",
        .handler        = cdc_shell_cmd_version,
        .description    = "print firmware version",
        .usage          = "Usage: version",
    },
    { 0 }
};

/* Global Commands */

static const char cdc_shell_err_no_help[] = "Error, no help for this command, use \"help\" to get the list of available commands.\r\n";

static void cdc_shell_cmd_help(int argc, char *argv[]) {
    const cdc_shell_cmd_t *cmd = cdc_shell_commands;
    while (cmd->cmd) {
        if (argc) {
            if (strcmp(*argv, cmd->cmd) == 0) {
                const char *delim = ": ";
                cdc_shell_write_string(cmd->cmd);
                cdc_shell_write_string(delim);
                cdc_shell_write_string(cmd->description);
                cdc_shell_write_string(cdc_shell_new_line);
                cdc_shell_write_string(cmd->usage);
                cdc_shell_write_string(cdc_shell_new_line);
                break;
            }
        } else {
            cdc_shell_write_string(cmd->cmd);
            cdc_shell_write_string(cdc_shell_delim);
            cdc_shell_write_string(cmd->description);
            cdc_shell_write_string(cdc_shell_new_line);
        }
        cmd++;
    }
    if (argc && (cmd->cmd == 0)) {
        cdc_shell_write_string(cdc_shell_err_no_help);
    }
}

static const char cdc_shell_err_unknown_command[] = "Error, unknown command, use \"help\" to get the list of available commands.\r\n";

static void cdc_shell_exec_command(int argc, char *argv[]) {
    if (cdc_shell_invoke_command(argc, argv, cdc_shell_commands) == -1) {
        cdc_shell_write_string(cdc_shell_err_unknown_command);
    }
}

static const char cdc_shell_err_too_long[]      = "Error, command line is too long.\r\n";
static const char cdc_shell_err_too_many_args[] = "Error, too many command line arguments.\r\n";

static void cdc_shell_parse_command_line(char *cmd_line) {
    int argc = 0;
    char *argv[USB_SHELL_MAC_CMD_ARGS];
    char *cmd_line_p = cmd_line;
    while (isspace(*(unsigned char *)cmd_line_p)) {
        cmd_line_p++;
    }
    while (*cmd_line_p) {
        if (argc < USB_SHELL_MAC_CMD_ARGS) {
            argv[argc] = cmd_line_p;
            while (*cmd_line_p && !isspace(*(unsigned char *)cmd_line_p)) {
                cmd_line_p++;
            }
            if (*cmd_line_p) {
                *cmd_line_p++ = '\0';
            }
            while (isspace(*(unsigned char *)cmd_line_p)) {
                cmd_line_p++;
            }
            argc++;
        } else {
            cdc_shell_write_string(cdc_shell_err_too_many_args);
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
    cdc_shell_write_string(cdc_shell_banner);
    cdc_shell_write_string(cdc_shell_prompt);
}

#define ASCII_BACKSPACE_CHAR        0x08
#define ASCII_DELETE_CHAR           0x7f
#define ANSI_CTRLSEQ_ESCAPE_CHAR    0x1B
#define ANSI_CTRLSEQ_ESCAPE_CSI     0x5B
#define ANSI_CTRLSEQ_CUU            0x41
#define ANSI_CTRLSEQ_CUD            0x42
#define ANSI_CTRLSEQ_CUF            0x43
#define ANSI_CTRLSEQ_CUB            0x44


static void cdc_shell_cursor_move_back(int n_symb) {
    if (n_symb) {
        char n_symb_str[32];
        itoa(n_symb, n_symb_str, 10);
        cdc_shell_write_string("\033[");
        cdc_shell_write_string(n_symb_str);
        cdc_shell_write_string("D");
    }
}

static void cdc_shell_handle_backspace() {
    if (cmd_line_cursor > cmd_line_buf) {
        cdc_shell_cursor_move_back(cmd_line_cursor - cmd_line_buf);
        cdc_shell_write_string(escape_clear_line_to_end);
        cmd_line_cursor--;
        memmove(cmd_line_cursor, cmd_line_cursor+1, strlen(cmd_line_cursor));
        cdc_shell_write_string(cmd_line_buf);
        cdc_shell_cursor_move_back(strlen(cmd_line_buf) - (cmd_line_cursor - cmd_line_buf));
    }
}

static void cdc_shell_insert_symbol(char c) {
    memmove(cmd_line_cursor+1, cmd_line_cursor, strlen(cmd_line_cursor)+1);
    *cmd_line_cursor = c;
    cdc_shell_write_string(cmd_line_cursor);
    cmd_line_cursor++;
    cdc_shell_cursor_move_back(strlen(cmd_line_buf) - (cmd_line_cursor - cmd_line_buf));
}

void cdc_shell_process_input(const void *buf, size_t count) {
    const char *buf_p= buf;
    while (count--) {
        switch (cdc_shell_state) {
        case cdc_shell_expects_csn:
            if (isdigit(*(unsigned char*)buf_p)) {
                /* Ignore values for simplicity */
                break;
            } else {
                if (*buf_p == ANSI_CTRLSEQ_CUF) {
                    if (*cmd_line_cursor) {
                        cmd_line_cursor++;
                        cdc_shell_write_string(escape_cursor_forward);
                    }
                } else if (*buf_p == ANSI_CTRLSEQ_CUB) {
                    if (cmd_line_cursor > cmd_line_buf) {
                        cmd_line_cursor--;
                        cdc_shell_write_string(escape_cursor_backward);
                    }
                } else if (*buf_p == ANSI_CTRLSEQ_CUU) {
                    size_t prev_cmd_len = strlen(cmd_prev_line_buf);
                    if (prev_cmd_len) {
                        strcpy(cmd_line_buf, cmd_prev_line_buf);
                        cdc_shell_cursor_move_back(cmd_line_cursor - cmd_line_buf);
                        cdc_shell_write_string(escape_clear_line_to_end);
                        cmd_line_cursor = cmd_line_buf + prev_cmd_len;
                        cdc_shell_write_string(cmd_line_buf);
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
        case cdc_shell_expects_lf:
            cdc_shell_state = cdc_shell_idle;
            if (*buf_p == '\n') {
                break;
            }
        case cdc_shell_idle:
            if (*buf_p == '\r' || *buf_p == '\n') {
                cdc_shell_state = cdc_shell_expects_lf;
                cdc_shell_write_string(cdc_shell_new_line);
                if (cmd_line_cursor != cmd_line_buf) {
                    strcpy(cmd_prev_line_buf, cmd_line_buf);
                }
                cdc_shell_parse_command_line(cmd_line_buf);
                cdc_shell_clear_cmd_buf();
                cdc_shell_write_string(cdc_shell_prompt);
            } else if (*buf_p == ANSI_CTRLSEQ_ESCAPE_CHAR) {
                cdc_shell_state = cdc_shell_expects_csi;
            } else if (*buf_p == ASCII_BACKSPACE_CHAR || *buf_p == ASCII_DELETE_CHAR) {
                cdc_shell_handle_backspace();
            } else if (isprint(*(unsigned char *)(buf_p))) {
                cdc_shell_insert_symbol(*buf_p);
                if ((cmd_line_cursor - cmd_line_buf) >= ARRAY_SIZE(cmd_line_buf)) {
                    cdc_shell_clear_cmd_buf();
                    cdc_shell_write_string(cdc_shell_new_line);
                    cdc_shell_write_string(cdc_shell_err_too_long);
                    cdc_shell_write_string(cdc_shell_prompt);
                }
            }
        }
        buf_p++;
    }
}
