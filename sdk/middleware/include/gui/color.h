/*
 * This file is part of the gigaSDK source code.
 * Copyright (c) 2025 MaxiHunter
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __COLOR_STYLE__
#define __COLOR_STYLE__

#include "ili9341/ILI9341_STM32_Driver.h"
#include "ili9341/ILI9341_GFX.h"

typedef uint16_t gigaGuiColor_t;

enum {
	COLOR_TYPE_SOLID = 0,
	COLOR_TYPE_LEANER_GRADIENT,
	COLOR_TYPE_RADIAL_GRADIENT,
};

struct gigaGuiColor {
	gigaGuiColor_t* color;
	uint8_t size;
	uint8_t * weight;
	uint8_t type;

};

 /** @brief Fuction to convert color from RGB space to rgb565
  *  @param r 8 bit red value
  *  @param g 8 bit green value
  *  @param b 8 bit blue value
  *  @return 16 bit color in rgb565
 */
uint16_t gigaGuiGetColor16bit(uint8_t r, uint8_t g, uint8_t b );

#endif // __COLOR_STYLE__
