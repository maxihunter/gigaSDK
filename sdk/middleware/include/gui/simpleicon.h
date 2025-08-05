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

#ifndef __GUI_SIMPLE_ICON__
#define __GUI_SIMPLE_ICON__

#include "ili9341/ILI9341_STM32_Driver.h"
#include "ili9341/ILI9341_GFX.h"

#define TYPE_CIRCLE 1
#define TYPE_RECTANGLE 2
#define TYPE_TRIANGLE 3

struct gigaGuiSimpleIcon {
	uint16_t pos_x;
	uint16_t pos_y;
	uint16_t pos_x_end;
	uint16_t pos_y_end;
	uint16_t symbol_color; // TODO make a struct for some variation like gradient, etc;
	uint8_t symbol;
	uint16_t bg_color;
	uint8_t border;
	uint16_t border_color;
	uint8_t border_thikness;
};

 /** @brief Fuction to draw button on the screen
  *  @param struct gigaGuiProgressBar to draw
  *  @see gigaGuiButton
 */
void gigaGuiDrawCircleIcon(struct gigaGuiSimpleIcon *si);

/** @brief Fuction to animate button press on the screen
  *  @param struct gigaGuiButton to draw
  *  @see gigaGuiButton
 */
void gigaGuiDrawSquareIcon(struct gigaGuiSimpleIcon* si);

/** @brief Fuction to animate button press on the screen
  *  @param struct gigaGuiButton to draw
  *  @see gigaGuiButton
 */
void gigaGuiDrawTriangleIcon(struct gigaGuiSimpleIcon* si);

#endif // __GUI_SIMPLE_ICON__
