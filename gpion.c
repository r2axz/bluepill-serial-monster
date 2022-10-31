/*
 * MIT License
 *
 * Copyright (c) 2022 Yury Shvedov
 */

#include "aux.h"
#include "gpion.h"
#include "device_config.h"
#include "default_config.h"

#include <string.h>
#include <stdlib.h>

gpio_pin_t *gpion_to_gpio(gpion_pin_t pinn)
{
    device_config_t *device = device_config_get();
    if (device != 0 && pinn < gpio_pin_last) {
        return &device->gpio_config.pins[pinn];
    }
    return 0;
}

gpion_pin_t gpio_to_gpion(const gpio_pin_t *pin)
{
    device_config_t *device = device_config_get();
    if (device != 0) {
        int pinn = pin - device->gpio_config.pins;
        if (0 <= pinn && pinn < (int)gpio_pin_last) {
            return (gpion_pin_t) pinn;
        }
    }
    return gpio_pin_unknown;
}

const char *gpion_to_str(gpion_pin_t pinn){
    static const char *pin_table[] = {

#define pin_table_record(pin) [gpio_pin_ ## pin] = #pin

        pin_table_record(pa0),
        pin_table_record(pa1),
        pin_table_record(pa2),
        pin_table_record(pa3),
        pin_table_record(pa4),
        pin_table_record(pa5),
        pin_table_record(pa6),
        pin_table_record(pa7),
        pin_table_record(pa8),
        pin_table_record(pa9),
        pin_table_record(pa10),
        pin_table_record(pa11),
        pin_table_record(pa12),
        pin_table_record(pa13),
        pin_table_record(pa14),
        pin_table_record(pa15),

        pin_table_record(pb0),
        pin_table_record(pb1),
        pin_table_record(pb2),
        pin_table_record(pb3),
        pin_table_record(pb4),
        pin_table_record(pb5),
        pin_table_record(pb6),
        pin_table_record(pb7),
        pin_table_record(pb8),
        pin_table_record(pb9),
        pin_table_record(pb10),
        pin_table_record(pb11),
        pin_table_record(pb12),
        pin_table_record(pb13),
        pin_table_record(pb14),
        pin_table_record(pb15),

        pin_table_record(pc13),
        pin_table_record(pc14),
        pin_table_record(pc15),

        pin_table_record(unknown),
#undef pin_table_record

    };

    BUILD_BUG_ON(ARRAY_SIZE(pin_table) != gpio_pin_last + 1);

    if (pinn < gpio_pin_last) {
        return pin_table[pinn];
    }

    return pin_table[gpio_pin_unknown];
}

gpion_pin_t str_to_gpion(const char *str){
    gpion_pin_t port = gpio_pin_unknown;
    char *pend = 0;
    unsigned long n = 0;
    device_config_t *device_config = device_config_get();

    if (!strcasecmp(str, "led")) {
        return device_config->status_led_pin;
    } else if (!strcasecmp(str, "shell") || !strcasecmp(str, "control")|| !strcasecmp(str, "config")) {
        return device_config->config_pin;
    }

#define check_prefix(prefix) do {                                              \
    if (strncasecmp(str, prefix, sizeof(prefix) - 1) == 0) {                   \
        str += sizeof(prefix) - 1;                                             \
    }                                                                          \
} while (0)
    check_prefix("gpio_");
    check_prefix("pin_");
    check_prefix("p");

#undef check_prefix

    switch (*str++) {
        case 'a': case 'A': port = gpio_pin_pa_first; break;
        case 'b': case 'B': port = gpio_pin_pb_first; break;
        case 'c': case 'C': port = gpio_pin_pc_first; break;
        default: return gpio_pin_unknown;
    }

    n = strtoul(str, &pend, 10);
    if (pend == 0 || *pend != '\0' || n > 15) {
        return gpio_pin_unknown;
    }
    if (port == gpio_pin_pc_first) {
        if (n < 13) {
            return gpio_pin_unknown;
        }
        n -= 13;
    }
    return port + n;
}

void gpion_pin_init(gpion_pin_t pinn)
{
    gpio_pin_t *pin = gpion_to_gpio(pinn);
    if (pin == 0) return;
    gpio_pin_init(pin);
}


/* This will work only for occupied pins */
void gpion_pin_set(gpion_pin_t pinn, int is_active)
{
    gpio_pin_t *pin = gpion_to_gpio(pinn);
    if (pin == 0 || pin->status != gpio_status_occupied) return;
    gpio_pin_set(pin, is_active);
}

int gpion_pin_get(gpion_pin_t pin)
{
    return gpion_pin_get_default(pin, 0);
}

int  gpion_pin_get_default(gpion_pin_t pinn, int def)
{
    gpio_pin_t *pin = gpion_to_gpio(pinn);
    if (pin == 0 || pin->status != gpio_status_occupied) return def;
    return gpio_pin_get(pin);
}

volatile uint32_t *gpion_pin_get_bitband_clear_addr(gpion_pin_t pinn)
{
    gpio_pin_t *pin = gpion_to_gpio(pinn);
    if (pin == 0 || pin->status != gpio_status_occupied) return 0;
    return gpio_pin_get_bitband_clear_addr(pin);
}

/* Set will work only for free pins, get - for free and occupied */
void gpion_pin_set_free(gpion_pin_t pinn, int is_active)
{
    gpio_pin_t *pin = gpion_to_gpio(pinn);
    if (pin == 0 || pin->status != gpio_status_free) return;
    gpio_pin_set(pin, is_active);
}

int  gpion_pin_get_free(gpion_pin_t pinn, int def)
{
    gpio_pin_t *pin = gpion_to_gpio(pinn);
    if (pin == 0 || (pin->status != gpio_status_free && pin->status != gpio_status_occupied)) return def;
    return gpio_pin_get(pin);
}

gpio_status_t gpion_pin_get_status(gpion_pin_t pinn)
{
    gpio_pin_t *pin = gpion_to_gpio(pinn);
    if (pin == 0) return gpio_status_unknown;
    return pin->status;
}

gpio_hal_t gpion_to_hal(gpion_pin_t pin) {
    gpio_hal_t result = { 0 };
#define pin_is(port) (gpio_pin_ ## port ## _first <= pin && pin <= gpio_pin_ ## port ## _last)
#define get_pin_d(port, d)    pin - gpio_pin_ ## port ## _first + d;
#define get_pin(port) get_pin_d(port, 0)
    if (pin_is(pa)) {
        result.port = GPIOA;
        result.pin = get_pin(pa);
    } else if (pin_is(pb)) {
        result.port = GPIOB;
        result.pin = get_pin(pb);
    } else if (pin_is(pc)) {
        result.port = GPIOC;
        result.pin = get_pin_d(pc, 13);
    }
    return result;
#undef get_pin
#undef get_pin_d
#undef pin_is
}

cdc_pin_ref_t gpion_to_cdc(gpion_pin_t pin)
{
    cdc_pin_ref_t result = { .port = -1, .pin = cdc_pin_unknown, };
    device_config_t *device_config = device_config_get();

    for (int port = 0; port < USB_CDC_NUM_PORTS; ++port) {
        for (cdc_pin_t cpin = 0; cpin < cdc_pin_last; ++cpin) {
            if (device_config->cdc_config.port_config[port].pins[cpin] == pin) {
                result.port = port;
                result.pin = cpin;
                return result;
            }
        }
    }
    return result;
}

