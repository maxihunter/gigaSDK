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

#include <gui/progressbar.h>

void gigaGuiDrawProgressBar(struct gigaGuiProgressBar* pb) {
	uint8_t border = pb->border ? pb->border_thikness : 0;
	if (pb->border) {
		ILI9341_Draw_Filled_Rectangle_Coord(pb->pos_x, pb->pos_y, pb->pos_x_end, pb->pos_y_end, pb->border_color);
	}
	ILI9341_Draw_Filled_Rectangle_Coord(pb->pos_x + border, pb->pos_y + border, pb->pos_x_end - border,
		pb->pos_y_end - border, pb->bg_color);
	ILI9341_Draw_Filled_Rectangle_Coord(pb->pos_x + border, pb->pos_y + border, pb->pos_x + pb->value,
		pb->pos_y_end - border, pb->color);
	
}
