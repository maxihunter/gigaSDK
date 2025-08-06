# ------------------------------------------------
#
# Author: MaxiHunter
# Copyright (C) 2025, MaxiHunter, all rights reserved.
#
# Building file for all submodules of gigaSDK
#
# ------------------------------------------------

# C sources
ifeq ($(DISPLAY), ILI9341_320_240)
C_SOURCES +=  \
$(MIDDLEWARE_PATH)/display/ili9341/ILI9341_STM32_Driver.c \
$(MIDDLEWARE_PATH)/display/ili9341/ILI9341_GFX.c
CFLAGS += -DILI9341_320_240
else ifeq ($(DISPLAY), ST7735_160_120)
C_SOURCES +=  \
$(MIDDLEWARE_PATH)/display/ili9341/ILI9341_STM32_Driver.c \
$(MIDDLEWARE_PATH)/display/ili9341/ILI9341_GFX.c
CFLAGS += -DST7735_160_120
else 
	echo "Display type does not specified!"
	exit 1
endif

C_SOURCES +=  \
$(MIDDLEWARE_PATH)/keyboard/keyboard.c  \
$(MIDDLEWARE_PATH)/decoder/mp3/bitstream.c  \
$(MIDDLEWARE_PATH)/decoder/mp3/buffers.c \
$(MIDDLEWARE_PATH)/decoder/mp3/dct32.c  \
$(MIDDLEWARE_PATH)/decoder/mp3/dequant.c  \
$(MIDDLEWARE_PATH)/decoder/mp3/dqchan.c  \
$(MIDDLEWARE_PATH)/decoder/mp3/huffman.c  \
$(MIDDLEWARE_PATH)/decoder/mp3/hufftabs.c  \
$(MIDDLEWARE_PATH)/decoder/mp3/imdct.c  \
$(MIDDLEWARE_PATH)/decoder/mp3/mp3dec.c  \
$(MIDDLEWARE_PATH)/decoder/mp3/mp3tabs.c  \
$(MIDDLEWARE_PATH)/decoder/mp3/scalfact.c  \
$(MIDDLEWARE_PATH)/decoder/mp3/stproc.c  \
$(MIDDLEWARE_PATH)/decoder/mp3/subband.c  \
$(MIDDLEWARE_PATH)/decoder/mp3/trigtabs.c  \
$(MIDDLEWARE_PATH)/decoder/microlzw/microlzw.c
#$(MIDDLEWARE_PATH)/fatfs/diskio.c  \
#$(MIDDLEWARE_PATH)/fatfs/ff.c  \
#$(MIDDLEWARE_PATH)/fatfs/ffsystem.c  \
#$(MIDDLEWARE_PATH)/fatfs/ffunicode.c  \
#$(MIDDLEWARE_PATH)/fatfs/user_diskio_spi.c
#$(MIDDLEWARE_PATH)/audio/pcm5102.c \
#$(MIDDLEWARE_PATH)/led/ws2812b.c \


ASM_SOURCES +=  \
$(MIDDLEWARE_PATH)/decoder/mp3/asmpoly_thumb2.s \

# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES +=  \
-I$(MIDDLEWARE_PATH)/include


