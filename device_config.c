#include <string.h>
#include <stm32f1xx.h>
#include "device_config.h"

#define DEVICE_CONFIG_FLASH_SIZE    0x10000
#define DEVICE_CONFIG_NUM_PAGES     2
#define DEVICE_CONFIG_PAGE_SIZE     0x400U
#define DEVICE_CONFIG_FLASH_END     (FLASH_BASE + DEVCONFIG_FLASH_SIZE)
#define DEVICE_CONFIG_BASE_ADDR     (DEVCONFIG_FLASH_END - DEVCONFIG_NUM_PAGES * DEVCONFIG_PAGE_SIZE)
#define DEVICE_CONFIG_MAGIC         0xDECFDECFUL

static const device_config_t default_device_config = {
    .config_pin = { .port = GPIOB, .pin = 5, .dir = gpio_dir_input, .pull = gpio_pull_up, .polarity = gpio_polarity_low },
    .cdc_config = {
        .port_config = {
            /*  Port 0 */
            {
                .pins = 
                {
                    /*  rx */ { .port = GPIOA, .pin = 10, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_high },
                    /*  tx */ { .port = GPIOA, .pin =  9, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_alternate, .output = gpio_output_pp, .polarity = gpio_polarity_high },
                    /* rts */ { .port = 0 }, /* No RTS due to the below reason  */
                    /* cts */ { .port = 0 }, /* CTS pin is occupied by USB      */
                    /* dsr */ { .port = GPIOB, .pin =  7, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                    /* dtr */ { .port = GPIOA, .pin =  4, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low  },
                    /* dcd */ { .port = GPIOB, .pin = 15, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                }
            },
            /*  Port 1 */
            {
                .pins = 
                {
                    /*  rx */ { .port = GPIOA, .pin =  3, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_high },
                    /*  tx */ { .port = GPIOA, .pin =  2, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_alternate, .output = gpio_output_pp, .polarity = gpio_polarity_high },
                    /* rts */ { .port = GPIOA, .pin =  1, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low},
                    /* cts */ { .port = GPIOA, .pin =  0, .dir = gpio_dir_input,  .pull = gpio_pull_down, .polarity = gpio_polarity_low },
                    /* dsr */ { .port = GPIOB, .pin =  4, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                    /* dtr */ { .port = GPIOA, .pin =  5, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low },
                    /* dcd */ { .port = GPIOB, .pin =  8, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                }
            },
            /*  Port 2 */
            {
                .pins = 
                {
                    /*  rx */ { .port = GPIOB, .pin = 11, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_high },
                    /*  tx */ { .port = GPIOB, .pin = 10, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_alternate, .output = gpio_output_pp, .polarity = gpio_polarity_high  },
                    /* rts */ { .port = GPIOB, .pin = 14, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low },
                    /* cts */ { .port = GPIOB, .pin = 13, .dir = gpio_dir_input,  .pull = gpio_pull_down, .polarity = gpio_polarity_low },
                    /* dsr */ { .port = GPIOB, .pin =  6, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                    /* dtr */ { .port = GPIOA, .pin =  6, .dir = gpio_dir_output, .speed = gpio_speed_medium, .func = gpio_func_general, .output = gpio_output_pp, .polarity = gpio_polarity_low  },
                    /* dcd */ { .port = GPIOB, .pin =  9, .dir = gpio_dir_input,  .pull = gpio_pull_up, .polarity = gpio_polarity_low },
                }
            },
        }
    }
};

static device_config_t currect_device_config;

static int device_config_load() {
    return -1;
}

void device_config_init() {
    if (device_config_load() == -1) {
        memcpy(&currect_device_config, &default_device_config, sizeof(currect_device_config));
    }
}

device_config_t *device_config_get() {
    return &currect_device_config;
}
