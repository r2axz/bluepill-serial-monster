# General Target Settings
TARGET	= bluepill-serial-monster
SRCS	= main.c system_clock.c system_interrupts.c status_led.c usb_core.c usb_descriptors.c\
	usb_io.c usb_uid.c usb_panic.c usb_cdc.c cdc_shell.c gpio.c device_config.c

# Toolchain & Utils
CROSS_COMPILE	?= arm-none-eabi-
CC		= $(CROSS_COMPILE)gcc
OBJCOPY		= $(CROSS_COMPILE)objcopy
SIZE		= $(CROSS_COMPILE)size
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

ifneq ($(FIRMWARE_ORIGIN),)
LDFLAGS		+= -Wl,-section-start=.isr_vector=$(FIRMWARE_ORIGIN)
endif

GIT_VERSION	:= $(subst ., ,$(subst v,,$(shell git describe --abbrev=0 --tags 2>/dev/null || true)))

ifneq ($(GIT_VERSION),)
GIT_VERSION_MAJOR 	:= $(word 1, $(GIT_VERSION))
ifneq ($(GIT_VERSION_MAJOR),)
CFLAGS			+= -DDEVICE_VERSION_MAJOR=$(GIT_VERSION_MAJOR)
endif
GIT_VERSION_MINOR 	:= $(word 2, $(GIT_VERSION))
ifneq ($(GIT_VERSION_MINOR),)
CFLAGS			+= -DDEVICE_VERSION_MINOR=$(GIT_VERSION_MINOR)
endif
GIT_VERSION_REVISION	:= $(word 3, $(GIT_VERSION))
ifneq ($(GIT_VERSION_REVISION),)
CFLAGS			+= -DDEVICE_VERSION_REVISION=$(GIT_VERSION_REVISION)
endif
endif

.PHONY: all
all: $(TARGET).hex $(TARGET).bin size

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -Obinary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -Oihex $< $@

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
flash: $(TARGET).hex
	$(STFLASH) --reset --format ihex write $<

.PHONY: size
size: $(TARGET).elf
	$(SIZE) $<

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(CHKREPORT)

.PHONY: distclean
distclean: clean
	rm -rf $(TARGET).elf $(TARGET).hex

