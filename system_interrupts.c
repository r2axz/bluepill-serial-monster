#include <string.h>
#include <stm32f1xx.h>
#include "system_interrupts.h"

#define SYSTEM_INTERRUPTS_COUNT             68
#define SYSTEM_INTERRUPTS_TABLE_ALIGNMENT   0x200

static volatile void *_sram_interrupt_table[SYSTEM_INTERRUPTS_COUNT] __attribute__ ((aligned(SYSTEM_INTERRUPTS_TABLE_ALIGNMENT)));

void system_interrupts_init() {
    memcpy(_sram_interrupt_table, (const void *)SCB->VTOR, sizeof(_sram_interrupt_table));
    SCB->VTOR = (uint32_t)_sram_interrupt_table;
    NVIC_SetPriorityGrouping(SYSTEM_INTERRUPTS_PRIORITY_GROUPING);
}
