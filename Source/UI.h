#pragma once
#include <vector>

// ================= Menu =================
void UI_Init();
void UI_Input();
void UI_Draw();
void UI_Exit();
bool UI_IsMenuOpen();

// ================= Fruit Basket Hover =================
enum fruitType
{
    FRUIT_APPLE = 0,
    FRUIT_PEAR = 1,
    FRUIT_BANANA = 2
};

struct FruitBasket
{
    int fruitType;        // 0 Apple, 1 Pear, 2 Banana
    int stock;
    float x, y;           // world position (center)
    float width, height;  // hover area size
};

// Provided by Main.cpp
const std::vector<FruitBasket>& GetFruitBaskets();

// UI drawing
void UI_DrawFruitBasketTooltips();

// Debug helper
//bool UI_IsMouseOverAnyBasket();

void UI_UpdateButtons();
float UI_GetPlotSlotX(int index);
float UI_GetPlotSlotY(int index);

// crates
struct CrateLayoutConfig
{
    struct UVRect { float uCenter, vCenter, uWidth, vHeight; } bins[3];

    // Tooltip
    float tipWidth;        // panel width (world units)
    float tipHeight;       // panel height
    float tipMargin;       // gap between crate and tooltip
    bool  tipCenterOnCrate; // true: center under crate; false: left-align to crate
};

const CrateLayoutConfig& UI_GetCrateLayoutConfig();
void UI_SetCrateLayoutConfig(const CrateLayoutConfig& cfg);


// Build crate hitboxes from the current stall transform.
void UI_RebuildCrateHitboxesFromStall(float stallX, float stallY, float stallW, float stallH);

void UI_DrawPlotTooltips();

// Simple yellow hover tint (same look as your Upgrades highlight).
void UI_DrawCrateHoverTint_Yellow();
float UI_GetCrateSlotX(int index);
float UI_GetCrateSlotY(int index);

struct PlotSlot
{
    float x, y;
    float width, height;
};

extern std::vector<PlotSlot> plotSlots;

extern bool gSoundEnabled;
extern bool gMusicEnabled;