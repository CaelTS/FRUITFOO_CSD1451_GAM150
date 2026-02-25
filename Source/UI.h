#pragma once
#pragma once
#include <vector>

// ================= Menu =================
void UI_Init();
void UI_Input();
void UI_Draw();
void UI_Exit();
bool UI_IsMenuOpen();

// ================= Fruit Basket Hover =================
struct FruitBasket
{
    int fruitType;        // 0 Apple, 1 Pear, 2 Banana
    float x, y;           // world position (center)
    float width, height;  // hover area size
};

// Provided by Main.cpp
const std::vector<FruitBasket>& GetFruitBaskets();

// UI drawing
void UI_DrawFruitBasketTooltips();

// Debug helper
bool UI_IsMouseOverAnyBasket();

void UI_UpdateButtons();
