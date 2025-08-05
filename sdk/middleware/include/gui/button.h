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

#ifndef __GUI_BUTTON__
#define __GUI_BUTTON__

#include "ili9341/ILI9341_STM32_Driver.h"
#include "ili9341/ILI9341_GFX.h"


struct gigaGuiButton {
	uint16_t pos_x;
	uint16_t pos_y;
	uint16_t pos_x_end;
	uint16_t pos_y_end;
	uint16_t color; // TODO make a struct for some variation like gradient, etc;
	uint16_t bg_color;
	uint16_t pressed_bg_color;
	uint8_t animation_frames;
	uint8_t border;
	uint16_t border_color;
	uint8_t border_thikness;
	uint8_t text;
	uint8_t text_size;
	uint16_t text_color;
};

 /** @brief Fuction to draw button on the screen
  *  @param struct gigaGuiProgressBar to draw
  *  @see gigaGuiButton
 */
void gigaGuiDrawButton(struct gigaGuiButton* bt);

/** @brief Fuction to animate button press on the screen
  *  @param struct gigaGuiButton to draw
  *  @see gigaGuiButton
 */
uint8_t gigaGuiButtonAnimate(struct gigaGuiButton* bt);

#endif // __GUI_BUTTON__
