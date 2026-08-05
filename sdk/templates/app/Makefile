# Change these three values after copying the template to sdk/apps/<app_id>.
TARGET := app_template
APP_ID := template
APP_NAME := App Template
APP_VERSION := 1.0.0

BUILD_DIR := build
ROOT := ../..
BIOS := $(ROOT)/gstr-bios
DRIVER := $(ROOT)/drivers
MW := $(ROOT)/middleware
GCC_PATH ?= /opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin
CC := $(GCC_PATH)/arm-none-eabi-gcc
OBJCOPY := $(GCC_PATH)/arm-none-eabi-objcopy
SIZE := $(GCC_PATH)/arm-none-eabi-size
MCU := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
INCLUDES := -I. -I$(BIOS)/Core/Inc -I$(BIOS)/FATFS/App \
	-I$(BIOS)/FATFS/Target -I$(BIOS)/Middlewares/Third_Party/FatFs/src \
	-I$(MW)/include -I$(DRIVER)/STM32F4xx_HAL_Driver/Inc \
	-I$(DRIVER)/STM32F4xx_HAL_Driver/Inc/Legacy \
	-I$(DRIVER)/CMSIS/Device/ST/STM32F4xx/Include -I$(DRIVER)/CMSIS/Include
CFLAGS := $(MCU) -Os -Wall -ffunction-sections -fdata-sections -MMD -MP \
	-DUSE_HAL_DRIVER -DSTM32F407xx -DILI9341_320_240 \
	-DVIDEO_FPS_OVERLAY=0 $(INCLUDES)
LDFLAGS := $(MCU) -Tapp_flash.ld -nostartfiles -specs=nano.specs \
	-Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map -lc -lm -lnosys

# All standard subsystems are listed here. --gc-sections removes unreferenced
# functions, but removing unused middleware source lines also speeds compilation.
SOURCES := main.c platform.c system.c \
	$(BIOS)/FATFS/App/fatfs.c $(BIOS)/FATFS/Target/bsp_driver_sd.c \
	$(BIOS)/FATFS/Target/sd_diskio.c $(BIOS)/FATFS/Target/fatfs_platform.c \
	$(BIOS)/Middlewares/Third_Party/FatFs/src/diskio.c \
	$(BIOS)/Middlewares/Third_Party/FatFs/src/ff.c \
	$(BIOS)/Middlewares/Third_Party/FatFs/src/ff_gen_drv.c \
	$(BIOS)/Middlewares/Third_Party/FatFs/src/option/syscall.c \
	$(MW)/display/ili9341/ILI9341_STM32_Driver.c \
	$(MW)/display/ili9341/ILI9341_GFX.c \
	$(MW)/keyboard/keyboard.c $(MW)/rtc/rtc_clock.c \
	$(MW)/audio/audio.c $(MW)/codec/ima_adpcm.c $(MW)/video/video.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_sdmmc.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sd.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s_ex.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc.c \
	$(DRIVER)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc_ex.c

VPATH := $(sort $(dir $(SOURCES)))
OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(SOURCES:.c=.o))) $(BUILD_DIR)/startup.o
.PHONY: all clean package
all: $(BUILD_DIR)/$(TARGET).bin
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD_DIR)/startup.o: $(DRIVER)/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f407xx.s | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) app_flash.ld
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary -S $< $@
$(BUILD_DIR)/icon.rgb565: icon.py | $(BUILD_DIR)
	python3 icon.py
package: $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/icon.rgb565
	python3 $(ROOT)/tools/gapp_pack.py --payload $(BUILD_DIR)/$(TARGET).bin \
		--icon $(BUILD_DIR)/icon.rgb565 --output $(BUILD_DIR)/apps/$(APP_ID)/app.bin \
		--name "$(APP_NAME)" --version "$(APP_VERSION)"
$(BUILD_DIR):
	mkdir -p $@
clean:
	rm -rf $(BUILD_DIR)
-include $(OBJECTS:.o=.d)
