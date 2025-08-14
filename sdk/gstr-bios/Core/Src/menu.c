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

static const uint8_t menu_count = 6;
static uint8_t item_selected = 0;
static const char menu_items[][16] = { "APP", "Network", "Storage", "Media", "Settings", "About" };

/**
  * @brief  The application entry point.
  * @retval int
  */
void mainMenu_Handler(void)
{
    ILI9341_Draw_Filled_Rectangle_Coord(80, 20, 240, 220, DARKGREY);
    for (uint8_t i = 0; i < menu_count; i++) {
        if (item_selected == i) {
            ILI9341_Draw_Filled_Rectangle_Coord(80, 20+(i*20), 240, 20 + ((i+1) * 20), YELLOW);
            ILI9341_Draw_Text(menu_items[i], 90, 22 + (i * 20), BLACK, 2, YELLOW);
        } else {
            ILI9341_Draw_Text(menu_items[i], 90, 22 + (i * 20), WHITE, 2, DARKGREY);
        }
    }
}


void mainMenu_TriggerUp() {
    item_selected--;
    if (item_selected >= menu_count) {
        item_selected = menu_count-1;
    }
}

void mainMenu_TriggerDown() {
    item_selected++;
    if (item_selected >= menu_count) {
        item_selected = 0;
    }
}

uint8_t mainMenu_GetSelectedId() {
    return item_selected;
}

