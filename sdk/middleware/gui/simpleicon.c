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

#include <gui/simpleicon.h>
#include <string.h>

static void drawBoxBg(struct gigaGuiSimpleIcon* si, uint16_t color) {
	uint8_t border = 0;
	if (si->border) {
		border = si->border_thikness;
		ILI9341_Draw_Filled_Rectangle_Coord(si->pos_x, si->pos_y, si->pos_x_end, si->pos_y_end, si->border_color);
	}
	// Fill BG
	ILI9341_Draw_Filled_Rectangle_Coord(si->pos_x + border, si->pos_y + border, si->pos_x_end - border,
		si->pos_y_end - border, color);
}

static void drawCircleBg(struct gigaGuiSimpleIcon* si, uint16_t color) {
	uint8_t border = 0;
	uint16_t radius = (si->pos_x_end - si->pos_x) / 2;
	if (si->border) {
		border = si->border_thikness;
		ILI9341_Draw_Filled_Circle(si->pos_x, si->pos_y, radius, si->border_color);
	}
	// Fill BG
	ILI9341_Draw_Filled_Circle(si->pos_x, si->pos_y, radius - border, color);
}

static void drawSymbol(struct gigaGuiSimpleIcon* si, uint16_t color) {
	// Horizontal position
	int size = (5 * si->text_size) / 2; // center of text field
	int icon_center = ((si->pos_x_end - si->pos_x) / 2) + si->pos_x; // absolute center of icon
	icon_center -= size; // start of the text field
	// Vertical position
	int icon_center_y = ((si->pos_y_end - si->pos_y) / 2) + si->pos_y; // absolute center of icon by y
	icon_center_y -= size;

	char buff[2] = { 0 };
	buff[0] = si->symbol;

	uint16_t high = ((si->pos_x_end - si->pos_x) / 5) - 1;
	ILI9341_Draw_Text(si->text, icon_center - len, icon_center_y - len, si->symbol_color, high, color);
}

void gigaGuiDrawCircleIcon(struct gigaGuiSimpleIcon* si) {
	drawCircleBg(si, ci->bg_color);
	drawSymbol(si, ci->bg_color)
}

void gigaGuiDrawSquareIcon(struct gigaGuiSimpleIcon* si) {
	drawBoxBg(si, ci->bg_color);
	drawSymbol(si, ci->bg_color);
}

void gigaGuiDrawTriangleIcon(struct gigaGuiSimpleIcon* si) {
	drawBoxBg(si, ci->bg_color);
	drawSymbol(si, ci->bg_color);
}

uint8_t gigaGuiSimpleIconAnimate(struct gigaGuiSimpleIcon* si, uint8_t type) {

	switch (type) {
	case TYPE_CIRCLE:
		break;
	case TYPE_RECTANGLE:
		break;
	case TYPE_TRIANGLE:
		break;
	}
	/*
	static uint8_t anim_frame = 0;
	if (anim_frame == 0) {
		anim_frames = si->animation_frames;
	}

	uint8_t c_r = ((si->bg_color & 0xF800) >> 8);
	uint8_t c_g = ((si->bg_color & 0x07E0) >> 5);
	uint8_t c_b = ((si->bg_color & 0x1F));

	uint8_t c_r_p = ((si->pressed_bg_color & 0xF800) >> 8);
	uint8_t c_g_p = ((si->pressed_bg_color & 0x07E0) >> 5);
	uint8_t c_b_p = ((si->pressed_bg_color & 0x1F));

	uint8_t c_r_d = ((c_r - c_r_p)/ si->animation_frames) * anim_frames;
	uint8_t c_g_d = ((c_g - c_g_p) / si->animation_frames) * anim_frames;
	uint8_t c_b_d = ((c_b - c_b_p) / si->animation_frames) * anim_frames;

	uint16_t color565 = ((c_r_d & 0xF8) << 8) | ((c_g_d & 0xFC) << 3) | ((c_b_d & 0xF8) >> 3);
	
	drawButtonOverrideColor(si, color565);

	anim_frames--;
	return anim_frames;*/
	return 0;
}
