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

/* Includes ------------------------------------------------------------------*/
#include "menu.h"

#include "ili9341/ILI9341_STM32_Driver.h"
#include "ili9341/ILI9341_GFX.h"
#include "string.h"

extern SPI_HandleTypeDef hspi2;
unsigned int sd_error = 0;

static const uint8_t menu_count = 5;
static uint8_t item_selected = 0;
static const char menu_items[][16] = { "Run", "Network", "Storage", "Settings", "Test" };

/**
  * @brief  The application entry point.
  * @retval int
  */
int mainMenu_Handler(void)
{
    ILI9341_Draw_Filled_Rectangle_Coord(20, 20, 300, 220, DARKGREY);
    for (uint8_t i = 0; i < menu_count; i++) {
        if (item_selected == i) {
            ILI9341_Draw_Filled_Rectangle_Coord(20+(i*15), 20, 300, 20 + ((i+1) * 15), DARKYELLOW);
            ILI9341_Draw_Text(buff, 20, 20 + (i * 15), WHITE, 2, DARKYELLOW);
        } else {
            ILI9341_Draw_Text(buff, 20, 20 + (i * 15), WHITE, 2, BLACK);
        }
    }
}
