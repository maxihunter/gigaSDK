# ------------------------------------------------
#
# Author: MaxiHunter
# Copyright (C) 2025, MaxiHunter, all rights reserved.
#
# Building file for all submodules of gigaSDK
#
# ------------------------------------------------

######################################
# source
######################################
# C sources
ifeq ($(PLATFORM), STM32F4)
C_SOURCES +=  \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_sdmmc.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sd.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c \

# ASM sources
ASM_SOURCES +=  \

# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES +=  \
-I$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Inc \
-I$(DRIVER_DIR)/STM32F4xx_HAL_Driver/Inc/Legacy \
-I$(DRIVER_DIR)/CMSIS/Device/ST/STM32F4xx/Include \
-I$(DRIVER_DIR)/CMSIS/Include

endif

