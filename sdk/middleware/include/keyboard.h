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

#include "stm32f4xx_hal.h"

/*
*     KBRD_BTN_TRIG_L                                        KBRD_BTN_TRIG_R
*   _____\\\\\\\\\___________________________________________/////////_______
*   |                                                                        |
*   |            KBRD_BTN_UP                                                 |
*   |                                             KBRD_BTN_3     KBRD_BTN_4  |
*   |  KBRD_BTN_LEFT    KBRD_BTN_RIGHT                                       |
*   |                                             KBRD_BTN_1     KBRD_BTN_2  |
*   |           KBRD_BTN_DOWN                                                |
*   |                                                                        |
*   |                                                                        |
*   |                     KBRD_BTN_ALT       KBRD_BTN_MENU                   |
*   |________________________________________________________________________|
* 
*/

#define KBRD_BTN_UP      (1 << 0)
#define KBRD_BTN_RIGHT   (1 << 1)
#define KBRD_BTN_LEFT    (1 << 2)
#define KBRD_BTN_DOWN    (1 << 3)
#define KBRD_BTN_1       (1 << 4)
#define KBRD_BTN_2       (1 << 5)
#define KBRD_BTN_3       (1 << 6)
#define KBRD_BTN_4       (1 << 7)
#define KBRD_BTN_MENU    (1 << 8)
#define KBRD_BTN_ALT     (1 << 9)
#define KBRD_BTN_TRIG_L  (1 << 10)
#define KBRD_BTN_TRIG_R  (1 << 11)

 /** @brief Get key state function
 *   @details Function returns a bitmap of pressed keys during call time
 *   @return Returning a bitmap 32-bit word with a key state. 1 is pressed
 *			0 is released. 
 */
uint32_t getKeyState();
