# General Target Settings
TARGET	= bluepill-serial-monster
SRCS	= main.c system_clock.c system_interrupts.c status_led.c usb_core.c usb_descriptors.c\
	usb_io.c usb_uid.c usb_panic.c usb_cdc.c cdc_shell.c gpio.c device_config.c

# Toolchain & Utils
CC		= arm-none-eabi-gcc
OBJCOPY		= arm-none-eabi-objcopy
SIZE		= arm-none-eabi-size
STFLASH		= st-flash
STUTIL		= st-util
CPPCHECK	= cppcheck

# STM32Cube Path
STM32CUBE	= ${STM32CUBE_PATH}
STM32_STARTUP	= $(STM32CUBE)/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s
STM32_SYSINIT	= $(STM32CUBE)/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c
STM32_LDSCRIPT	= $(STM32CUBE)/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/linker/STM32F103XB_FLASH.ld

STM32_INCLUDES	+= -I$(STM32CUBE)/Drivers/CMSIS/Core/Include
STM32_INCLUDES	+= -I$(STM32CUBE)/Drivers/CMSIS/Core_A/Include
STM32_INCLUDES	+= -I$(STM32CUBE)/Drivers/CMSIS/Device/ST/STM32F1xx/Include

DEFINES		= -DSTM32F103xB -DHSE_VALUE=8000000U
CPUFLAGS	= -mthumb -mcpu=cortex-m3
WARNINGS	= -Wall
OPTIMIZATION	= -O3
DEBUG		= -ggdb

CFLAGS		= $(DEFINES) $(STM32_INCLUDES) $(CPUFLAGS) $(WARNINGS) $(OPTIMIZATION) $(DEBUG) 
LDFLAGS		= $(CPUFLAGS) -T$(STM32_LDSCRIPT) --specs=nosys.specs --specs=nano.specs
DEPFLAGS	= -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d

CHKREPORT	= cppcheck-report.txt
CHKFLAGS	= --enable=all --error-exitcode=1 --suppress=missingIncludeSystem:nofile -D__GNUC__

BUILD_DIR	= build
OBJS		+= $(SRCS:%.c=$(BUILD_DIR)/%.o)
OBJS		+= $(STM32_SYSINIT:%.c=$(BUILD_DIR)/%.o)
STARTUP		+= $(STM32_STARTUP:%.s=$(BUILD_DIR)/%.o)

.PHONY: all
all: $(TARGET).hex $(TARGET).bin size

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -Obinary $(TARGET).elf $(TARGET).bin

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -Oihex $(TARGET).elf $(TARGET).hex

$(TARGET).elf: $(OBJS) $(STARTUP)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

DEPFILES := $(OBJS:%.o=%.d)
$(DEPFILES):
-include $(DEPFILES)

cppcheck: $(SRCS)
	$(CPPCHECK) $(CHKFLAGS) $(STM32_INCLUDES) $(DEFINES) --output-file=$(CHKREPORT) $^

.PHONY: flash
flash: all
	$(STFLASH) --format ihex write $(TARGET).hex

.PHONY: size
size:
	$(SIZE) $(TARGET).elf

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(CHKREPORT)

.PHONY: distclean
distclean: clean
	rm -rf $(TARGET).elf $(TARGET).hex

