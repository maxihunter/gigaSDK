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

#include <gui/checkbox.h>
#include <string.h>


 /** @brief Fuction to draw button on the screen
  *  @param struct gigaGuiProgressBar to draw
  *  @see gigaGuiButton
 */

static void drawCheckBoxBg(struct gigaGuiCheckBox* cb, uint16_t color) {
	uint8_t border = 0;
	if (cb->border) {
		border = cb->border_thikness;
		ILI9341_Draw_Filled_Rectangle_Coord(cb->pos_x, cb->pos_y, cb->pos_x_end, cb->pos_y_end, cb->border_color);
	}
	// Fill BG
	ILI9341_Draw_Filled_Rectangle_Coord(cb->pos_x + border, cb->pos_y + border, cb->pos_x_end - border,
		cb->pos_y_end - border, color);
}

static void drawCheckBoxOverrideColor(struct gigaGuiCheckBox* cb, uint16_t color) {
	drawButtonBg(cb, cb->bg_color);

	// Horizontal position
	int cb_center = ((cb->pos_x_end - cb->pos_x) / 2); // center of checkbox
	// Vertical position
	int cb_center_y = ((cb->pos_y_end - cb->pos_y) / 2); // center of checkbox by y

	// check checked option
	if (cd->is_checked) {
		ILI9341_Draw_Filled_Rectangle_Coord(
			cb->pos_x + border + (cb_center / 2),
			cb->pos_y + border + (cb_center_y / 2),
			cb->pos_x_end - border - (cb_center / 2),
			cb->pos_y_end - border - (cb_center_y / 2), cb->pressed_bg_color);
	}	
}

void gigaGuiDrawCheckBox(struct gigaGuiCheckBox* cb) {
	drawCheckBoxOverrideColor(cb, cb->bg_color);
}

uint8_t gigaGuiCheckBoxAnimate(struct gigaGuiCheckBox* cb) {
	/*
	static uint8_t anim_frame = 0;
	if (anim_frame == 0) {
		anim_frames = cb->animation_frames;
	}

	uint8_t c_r = ((cb->bg_color & 0xF800) >> 8);
	uint8_t c_g = ((cb->bg_color & 0x07E0) >> 5);
	uint8_t c_b = ((cb->bg_color & 0x1F));

	uint8_t c_r_p = ((cb->pressed_bg_color & 0xF800) >> 8);
	uint8_t c_g_p = ((cb->pressed_bg_color & 0x07E0) >> 5);
	uint8_t c_b_p = ((cb->pressed_bg_color & 0x1F));

	uint8_t c_r_d = ((c_r - c_r_p)/ cb->animation_frames) * anim_frames;
	uint8_t c_g_d = ((c_g - c_g_p) / cb->animation_frames) * anim_frames;
	uint8_t c_b_d = ((c_b - c_b_p) / cb->animation_frames) * anim_frames;

	uint16_t color565 = ((c_r_d & 0xF8) << 8) | ((c_g_d & 0xFC) << 3) | ((c_b_d & 0xF8) >> 3);
	
	drawButtonOverrideColor(cb, color565);

	anim_frames--;
	return anim_frames;*/
	return 0;
}
