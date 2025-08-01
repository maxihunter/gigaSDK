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
display/ili9341/ILI9341_STM32_Driver.c \
display/ili9341/ILI9341_GFX.c
CFLAGS += -DILI9341_320_240
else ifeq ($(DISPLAY), ST7735_160_120)
C_SOURCES +=  \
display/ili9341/ILI9341_STM32_Driver.c \
display/ili9341/ILI9341_GFX.c
CFLAGS += -DST7735_160_120
else 
	echo "Display type does not specified!"
	exit 1
endif


C_SOURCES +=  \
audio/pcm5201.c \
led/ws2812b.c \
display/ili9341/ILI9341_GFX.c \
display/ili9341/ILI9341_GFX.c \
decoder/mp3/bitstream.c  \
decoder/mp3/buffers.c \
decoder/mp3/dct32.c  \
decoder/mp3/dequant.c  \
decoder/mp3/dqchan.c  \
decoder/mp3/huffman.c  \
decoder/mp3/hufftabs.c  \
decoder/mp3/imdct.c  \
decoder/mp3/mp3dec.c  \
decoder/mp3/mp3tabs.c  \
decoder/mp3/scalfact.c  \
decoder/mp3/stproc.c  \
decoder/mp3/subband.c  \
decoder/mp3/trigtabs.c  \
fatfs/diskio.c  \
fatfs/ff.c  \
fatfs/ffsystem.c  \
fatfs/ffunicode.c  \
fatfs/user_diskio_spi.c


ASM_SOURCES +=  \
decoder/mp3/asmpoly_thumb2.s \

# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES +=  \
-Iinclude


