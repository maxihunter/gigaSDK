TARGET := hello
BUILD_DIR := build
GCC_PATH ?= /opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin
PREFIX := $(GCC_PATH)/arm-none-eabi-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size

ROOT := ../..
MCU := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS := $(MCU) -Os -Wall -ffunction-sections -fdata-sections \
	-DSTM32F407xx -I$(ROOT)/drivers/CMSIS/Device/ST/STM32F4xx/Include \
	-I$(ROOT)/drivers/CMSIS/Include
LDFLAGS := $(MCU) -Tapp_flash.ld -nostartfiles -specs=nano.specs \
	-Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map -lc -lnosys

SOURCES := main.c system.c \
	$(ROOT)/drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f407xx.s
OBJECTS := $(BUILD_DIR)/main.o $(BUILD_DIR)/system.o $(BUILD_DIR)/startup.o

.PHONY: all clean package
all: $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/main.o: main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/system.o: system.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/startup.o: $(ROOT)/drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f407xx.s | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) app_flash.ld
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary -S $< $@

package: $(BUILD_DIR)/$(TARGET).bin
	python3 $(ROOT)/tools/gapp_pack.py --payload $< \
		--output $(BUILD_DIR)/apps/hello/app.bin --name "Hello" \
		--version "1.0.0" --icon-color "32,96,192"

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
