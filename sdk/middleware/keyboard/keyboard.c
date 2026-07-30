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

#include <keyboard.h>
#include "stm32f4xx_hal.h"

/**
	@brief Description of keys
	@details Structure describes exact port and pin with connected buttons
*/
struct key_info {
	GPIO_TypeDef *port; ///< Port number of the key
	uint16_t pin; ///< Pin number of the key
	uint32_t mask; ///< KBRD_BTN_* value returned for the key
};

/** @brief Keymap array data size */
#define KEY_MAP_SIZE (sizeof(key_map) / sizeof(key_map[0]))

/** @brief Keymap array */
const struct key_info key_map[] = {
	{GPIOE, GPIO_PIN_6, KBRD_BTN_UP},
	{GPIOB, GPIO_PIN_4, KBRD_BTN_DOWN},
	{GPIOE, GPIO_PIN_7, KBRD_BTN_LEFT},
	{GPIOB, GPIO_PIN_6, KBRD_BTN_RIGHT},
	{GPIOB, GPIO_PIN_7, KBRD_BTN_1},
	{GPIOB, GPIO_PIN_8, KBRD_BTN_2},
	{GPIOB, GPIO_PIN_9, KBRD_BTN_3},
	{GPIOB, GPIO_PIN_9, KBRD_BTN_4},
};

uint32_t getKeyState() {
	uint32_t keymap = 0;
	for (uint8_t i = 0; i < KEY_MAP_SIZE; i++) {
		if (HAL_GPIO_ReadPin(key_map[i].port, key_map[i].pin) == GPIO_PIN_RESET) {
			keymap |= key_map[i].mask;
		}
	}
	return keymap;
}
