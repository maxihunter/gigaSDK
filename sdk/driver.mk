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
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_sdmmc.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sd.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
driver/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c \
#Core/Src/system_stm32f4xx.c \
#Core/Src/stm32f4xx_it.c \
#Core/Src/stm32f4xx_hal_msp.c \

# ASM sources
ASM_SOURCES +=  \
startup_stm32f401xc.s

# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES +=  \
-ICore/Inc \
-Iili9341 \
-Idriver/STM32F4xx_HAL_Driver/Inc \
-Idriver/STM32F4xx_HAL_Driver/Inc/Legacy \
-Idriver/CMSIS/Device/ST/STM32F4xx/Include \
-Idriver/CMSIS/Include

# LDFLAGS
LDSCRIPT += STM32F401VCTx_FLASH.ld
endif

