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

#include <gui/button.h>
#include <string.h>


 /** @brief Fuction to draw button on the screen
  *  @param struct gigaGuiProgressBar to draw
  *  @see gigaGuiButton
 */

static void drawButtonBg(struct gigaGuiButton* bt, uint16_t color) {
	uint8_t border = 0;
	if (bt->border) {
		border = bt->border_thikness
		ILI9341_Draw_Filled_Rectangle_Coord(bt->pos_x, bt->pos_y, bt->pos_x_end, bt->pos_y_end, bt->border_color);
	}
	// Fill BG
	ILI9341_Draw_Filled_Rectangle_Coord(bt->pos_x + border, bt->pos_y + border, bt->pos_x_end - border,
		bt->pos_y_end - border, color);
}

static void drawButtonOverrideColor(struct gigaGuiButton* bt, uint16_t color) {
	drawButtonBg(bt, bt->bg_color);

	// Horizontal position
	int len = strlen(bt->text);
	len = (len * (5 * bt->text_size)) / 2; // center of text field
	btn_center = ((bt->pos_x_end - bt->pos_x) / 2) + bt->pos_x; // absolute center of button
	btn_center -= len; // start of the text field
	// Vertical position
	int high = (5 * bt->text_size) / 2;
	int btn_center_y = ((bt->pos_y_end - bt->pos_y) / 2) + bt->pos_y; // absolute center of button by y

	ILI9341_Draw_Text("SD Card not found", 0, btn_center - len, bt->text_color, btn_center_y - bt->high, color);
}

void gigaGuiDrawButton(struct gigaGuiButton* bt) {
	drawButtonOverrideColor(bt, bt->bg_color);
}


uint8_t gigaGuiButtonAnimate(struct gigaGuiButton* bt) {
	static uint8_t anim_frame = 0;
	if (anim_frame == 0) {
		anim_frames = bt->animation_frames;
	}

	uint8_t c_r = ((bt->bg_color & 0xF800) >> 8);
	uint8_t c_g = ((bt->bg_color & 0x07E0) >> 5);
	uint8_t c_b = ((bt->bg_color & 0x1F));

	uint8_t c_r_p = ((bt->pressed_bg_color & 0xF800) >> 8);
	uint8_t c_g_p = ((bt->pressed_bg_color & 0x07E0) >> 5);
	uint8_t c_b_p = ((bt->pressed_bg_color & 0x1F));

	uint8_t c_r_d = ((c_r - c_r_p)/ bt->animation_frames) * anim_frames;
	uint8_t c_g_d = ((c_g - c_g_p) / bt->animation_frames) * anim_frames;
	uint8_t c_b_d = ((c_b - c_b_p) / bt->animation_frames) * anim_frames;

	uint16_t color565 = ((c_r_d & 0xF8) << 8) | ((c_g_d & 0xFC) << 3) | ((c_b_d & 0xF8) >> 3);
	
	drawButtonOverrideColor(bt, color565);

	anim_frames--;
	return anim_frames;
}
