# ------------------------------------------------
#
# Author: MaxiHunter
# Copyright (C) 2025, MaxiHunter, all rights reserved.
#
# Building file for all submodules of gigaSDK
#
# ------------------------------------------------


# C sources
ifeq ($(PLATFORM), STM32F4)
C_SOURCES +=  \
Core/Src/stm32f4xx_it.c \
Core/Src/stm32f4xx_hal_msp.c \
Core/Src/system_stm32f4xx.c \

# ASM sources
ASM_SOURCES =  \
startup_stm32f401xc.s
endif

# C includes
C_INCLUDES += \
-ICore/Inc \

