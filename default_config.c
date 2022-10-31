/*
 * MIT License
 *
 * Copyright (c) 2020 Kirill Kotyagin
 * Copyright (c) 2022 Yury Shvedov
 */

#include "aux.h"
#include "default_config.h"

static const default_config_t default_config = {
    .status_led_pin = { .pin = gpio_pin_pc13, .dir = gpio_dir_output, .speed = gpio_speed_low, .func = gpio_func_general, .output = gpio_output_od, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
    .config_pin = { .pin = gpio_pin_pb5, .dir = gpio_dir_input, .pull = gpio_pull_up, .polarity = gpio_polarity_high, .status = gpio_status_occupied, },
    .cdc_config = {
        .port_config = {
            /*  Port 0 */
            {
                .pins =
                {
                    [cdc_pin_rx]  = { .pin = gpio_pin_pa10, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_high, .status = gpio_status_occupied, },
                    [cdc_pin_tx]  = { .pin =  gpio_pin_pa9, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_alternate, .output = gpio_output_pp, .polarity = gpio_polarity_high, .status = gpio_status_occupied, },
                    [cdc_pin_rts] = { .pin =  gpio_pin_pa15, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_cts] = { .pin = gpio_pin_pa11,  .dir = gpio_dir_input,  .pull = gpio_pull_down, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_dsr] = { .pin =  gpio_pin_pb7, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_dtr] = { .pin =  gpio_pin_pa4, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_dcd] = { .pin = gpio_pin_pb15, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_ri]  = { .pin =  gpio_pin_pb3, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_txa] = { .pin =  gpio_pin_pb0, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_high, .status = gpio_status_occupied, },
                }
            },
            /*  Port 1 */
            {
                .pins =
                {
                    [cdc_pin_rx]  = { .pin =  gpio_pin_pa3, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_high, .status = gpio_status_occupied, },
                    [cdc_pin_tx]  = { .pin =  gpio_pin_pa2, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_alternate, .output = gpio_output_pp, .polarity = gpio_polarity_high, .status = gpio_status_occupied, },
                    [cdc_pin_rts] = { .pin =  gpio_pin_pa1, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_cts] = { .pin =  gpio_pin_pa0, .dir = gpio_dir_input,  .pull = gpio_pull_down, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_dsr] = { .pin =  gpio_pin_pb4, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_dtr] = { .pin =  gpio_pin_pa5, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_dcd] = { .pin =  gpio_pin_pb8, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_ri]  = { .pin = gpio_pin_pb12, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_txa] = { .pin =  gpio_pin_pb1, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_high, .status = gpio_status_occupied, },
                }
            },
            /*  Port 2 */
            {
                .pins =
                {
                    [cdc_pin_rx]  = { .pin = gpio_pin_pb11, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_high, .status = gpio_status_occupied, },
                    [cdc_pin_tx]  = { .pin = gpio_pin_pb10, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_alternate, .output = gpio_output_pp, .polarity = gpio_polarity_high, .status = gpio_status_occupied, },
                    [cdc_pin_rts] = { .pin = gpio_pin_pb14, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_cts] = { .pin = gpio_pin_pb13, .dir = gpio_dir_input,  .pull = gpio_pull_down, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_dsr] = { .pin =  gpio_pin_pb6, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_dtr] = { .pin =  gpio_pin_pa6, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_dcd] = { .pin =  gpio_pin_pb9, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_ri]  = { .pin =  gpio_pin_pa8, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low, .status = gpio_status_occupied, },
                    [cdc_pin_txa] = { .pin =  gpio_pin_pa7, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_high, .status = gpio_status_occupied, },
                }
            },
        }
    },
    .blocked_pins = {
        {
            .pin = gpio_pin_pa11,
            .reason = "usb dm",
        },
        {
            .pin = gpio_pin_pa12,
            .reason = "usb dp",
        },
        {
            .pin = gpio_pin_pa13,
            .reason = "debug swdio",
        },
        {
            .pin = gpio_pin_pa14,
            .reason = "debug swclk",
        },
        {
            .pin = gpio_pin_pb2,
            .reason = "boot control",
        },

        {
            .pin = default_blocked_pin_end,
        }
    },
};

static gpion_pin_t default_config_pin_load(device_config_t *target, const default_gpio_pin_t *source);
static void default_config_pin_last(device_config_t *target, gpion_pin_t pin);

void default_config_load(device_config_t *target) {
    default_gpio_pin_t default_free = {
        .dir = gpio_dir_input,
        .func = gpio_func_general,
        .output = gpio_output_pp,
        .pull = gpio_pull_up,
        .polarity = gpio_polarity_high,
        .speed = gpio_speed_medium,
        .status = gpio_status_free,
    };
    default_gpio_pin_t default_blocked = {
        .dir = gpio_dir_unknown,
        .func = gpio_func_alternate,
        .output = gpio_output_unknown,
        .pull = gpio_pull_unknown,
        .polarity = gpio_polarity_unknown,
        .speed = gpio_speed_unknown,
        .status = gpio_status_blocked,
    };
    if (target != 0) {
        // reset all gpio pins to free.
        for (gpion_pin_t pin = 0; pin < gpio_pin_last; ++pin) {
            default_free.pin = pin;
            target->gpio_config.pins[pin].status = gpio_status_free;
            default_config_pin_load(target, &default_free);
        }
        // configure system pins
        for (int i = 0; default_config.blocked_pins[i].pin != default_blocked_pin_end; ++i) {
            gpion_pin_t pin = default_config.blocked_pins[i].pin;
            default_blocked.pin = pin;
            default_config_pin_load(target, &default_blocked);
        }
        // configure misc pins
        target->status_led_pin = default_config_pin_load(target, &default_config.status_led_pin);
        target->config_pin = default_config_pin_load(target, &default_config.config_pin);
        // configure uart pins
        for (int port = 0; port < ARRAY_SIZE(target->cdc_config.port_config) && port < ARRAY_SIZE(default_config.cdc_config.port_config); ++port) {
            const default_port_t *default_port = &default_config.cdc_config.port_config[port];
            cdc_port_t *port_config = &target->cdc_config.port_config[port];
            for (int pin = 0; pin < ARRAY_SIZE(default_port->pins) && pin < ARRAY_SIZE(port_config->pins); ++pin) {
                port_config->pins[pin] = default_config_pin_load(target, &default_port->pins[pin]);
            }
        }
        // configure other pins
        for (gpion_pin_t pin = 0; pin < gpio_pin_last; ++pin) {
            default_config_pin_last(target, pin);
        }
    }
}

const char *default_config_get_blocked_reason(gpion_pin_t pin) {
    static const char *not_blocked = "pin is not blocked";
    for (int i = 0; default_config.blocked_pins[i].pin != default_blocked_pin_end; ++i) {
        if (pin == default_config.blocked_pins[i].pin) {
            return default_config.blocked_pins[i].reason;
        }
    }
    return not_blocked;
}

static gpion_pin_t default_config_pin_load(device_config_t *target, const default_gpio_pin_t *source){
    if (source->pin == gpio_pin_unknown) {
        return gpio_pin_unknown;
    }
    gpio_pin_t *pin = &target->gpio_config.pins[source->pin];
    if (pin->status == gpio_status_blocked) {
        return source->pin;
    }

    pin->dir = source->dir;
    pin->func = source->func;
    pin->output = source->output;
    pin->pull = source->pull;
    pin->polarity = source->polarity;
    pin->speed = source->speed;
    pin->status = source->status;

    return source->pin;
}

static void default_config_pin_last(device_config_t *target, gpion_pin_t pinn) {
    gpio_pin_t *pin = &target->gpio_config.pins[pinn];
    if (pinn < gpio_pin_last && pin->status == gpio_status_free) {
        pin->dir = gpio_dir_unknown;
        pin->func = gpio_func_general;
        pin->output = gpio_output_unknown;
        pin->pull = gpio_pull_unknown;
        pin->polarity = gpio_polarity_unknown;
        pin->speed = gpio_speed_unknown;
    }
}
