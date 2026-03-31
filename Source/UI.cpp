#define NOMINMAX

#include "UI.h"
#include "Farm.h"
#include "Economy.h"
#include "AEEngine.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include "Inventory.h"
#include "Crate.h"
#include "Upgrades.h"
#include "Main.h"
#include "UIAudio.h"
#include "GameStateManager.h"


extern AEGfxVertexList* g_pMeshFullScreen;
extern s8 fontId;

static bool menuOpen = false;
static bool popupOpen = false;
static bool seedsPopupOpen = false;

static int activePopupIndex = -1;
static int selectedSeed = -1;
static int hoveredSeed = -1;   // purely for highlight
static int infoSeed = -1;         // which seed info panel is showing
static int hoveredPlotIndex = -1;
static int activePlotIndex = -1;

float UI_GetPlotCenterX();
float UI_GetPlotCenterY();

std::vector<PlotSlot> plotSlots;
static AEGfxTexture* plotSlotTexture = nullptr;

static AEGfxTexture* menuTexture = nullptr;
static AEGfxTexture* seedsTexture = nullptr;

static AEGfxTexture* inventoryIcon = nullptr;
static AEGfxTexture* inventoryBG = nullptr;
static AEGfxTexture* invFruit = nullptr;
static AEGfxTexture* invFruitHighlight = nullptr;
static AEGfxTexture* invSeed = nullptr;
static AEGfxTexture* invSeedHighlight = nullptr;
static AEGfxTexture* invSliderKnob = nullptr;
static AEGfxTexture* invSliderFill = nullptr;
static AEGfxTexture* appleIcon = nullptr;
static AEGfxTexture* trashIcon = nullptr;
static AEGfxTexture* confirmBG = nullptr;
static AEGfxTexture* confirmYes = nullptr;
static AEGfxTexture* confirmNo = nullptr;

static AEGfxTexture* collectionIcon = nullptr;
static AEGfxTexture* collectionBG = nullptr;

static AEGfxTexture* settingsIcon = nullptr;
static AEGfxTexture* settingsBG = nullptr;
static AEGfxTexture* settingsOn = nullptr;
static AEGfxTexture* settingsOff = nullptr;

static AEGfxTexture* appleSeedIcon = nullptr;
static AEGfxTexture* pearSeedIcon = nullptr;
static AEGfxTexture* appleSeedInfo = nullptr;
static AEGfxTexture* leftArrowTexture = nullptr;
static AEGfxTexture* rightArrowTexture = nullptr;


enum ButtonType
{
    BUTTON_INVENTORY,
    BUTTON_COLLECTION,
    BUTTON_SETTINGS
};

enum SeedType
{
    SEED_APPLE = 0,
    SEED_PEAR,
    SEED_BANANA,
    SEED_COUNT
};

struct MenuButton
{
    float x, y;
    float width, height;
    bool isHovered;
    ButtonType type;
};

struct SeedData
{
    int cost;
    float growTime;
    int waterNeeded;
    const char* name;
};

static SeedData seedDatabase[SEED_COUNT] =
{
    { 10, 5.0f, 1, "Apple Seed" },
    { 15, 7.0f, 2, "Pear Seed" },
    { 20, 9.0f, 3, "Banana Seed" }
};

// The current seed page shown in the panel
static int currentSeedIndex = 0;

static std::vector<MenuButton> menuButtons;
static MenuButton plotPlusButton;

// Crate bins
extern std::vector<FruitBasket> gFruitBaskets;

static CrateLayoutConfig gCrateCfg{};
static bool gCrateCfgInitialized = false;


// ------------------------
// Upgrades
// ------------------------
static int upgradesStartIndex = 0;
static const int MAX_VISIBLE_UPGRADES = 3;
static const float UPGRADES_PANEL_W = 450.0f;
static const float UPGRADES_PANEL_H = 280.0f;
static const float UPGRADES_PANEL_X = -530.0f;
static const float UPGRADES_PANEL_Y = -160.0f;

float ScaleX = 1600.0f / 1920.0f;
float ScaleY = 900.0f / 1080.0f;

static const float UP_BOX_X = -480.0f;
static const float UP_BOX_Y = -180.0f;
static const float UP_BOX_W = 450.0f;
static const float UP_BOX_H = 280.0f;

static const float UP_LIST_TOP_OFFSET = +80.0f;
static const float UP_ROW_SPACING = 70.0f;
static const float UP_ROW_W_MARGIN = 20.0f;
static const float UP_ROW_H = 70.0f;

static const float UP_HOVER_INSET_L = 15.0f;
static const float UP_HOVER_INSET_R = 12.0f;
static const float UP_HOVER_INSET_TB = 8.0f;
static const float UP_HOVER_Y_NUDGE = 15.0f;
static const float UP_HOVER_X_OFFSET = -50.0f;


// -------------------------
// Inventory panel
// -------------------------

enum InvTab { TAB_FRUITS = 0, TAB_SEEDS = 1 };
static InvTab gActiveInvTab = TAB_FRUITS;

static const float INV_PANEL_W = 520.0f;
static const float INV_PANEL_H = 680.0f;

static const float INV_TAB_LEFT_PAD = 48.0f;
static const float INV_TAB_TOP_PAD = 120.0f;
static const float INV_TAB_SPACING_X = 96.0f;

static const float INV_FRUIT_W = 64.0f;
static const float INV_FRUIT_H = 64.0f;
static const float INV_FRUIT_HL_W = 72.0f;
static const float INV_FRUIT_HL_H = 72.0f;

static const float INV_SEED_W = 64.0f;
static const float INV_SEED_H = 64.0f;
static const float INV_SEED_HL_W = 72.0f;
static const float INV_SEED_HL_H = 72.0f;

static const float INV_SLD_LABEL_LEFT_XPAD = -10.0f;
static const float INV_SLD_LABEL_RIGHT_XPAD = 6.0f;
static const float INV_SLD_LABEL_Y_OFFSET = -20.0f;

static const float INV_SLD_VALUE_Y_OFFSET = 30.0f;
static const float INV_SLD_VALUE_SCALE = 0.5f;

static const float INV_SLD_LEFT_PAD = 110.0f;
static const float INV_SLD_TOP_PAD = 590.0f;
static const float INV_SLD_TRACK_W = 230.0f;
static const float INV_SLD_FILL_H = 14.0f;

static const float INV_SLD_KNOB_W = 35.0f;
static const float INV_SLD_KNOB_H = 48.0f;

static int  gInvSliderValue = 1;
static bool gInvSliderDragging = false;

static const float INV_SLD_FILL_Y_OFFSET = -10.0f;
static const float INV_SLD_TRACK_CAP_INSET = -60.0f;
static const float INV_SLD_FILL_THICKNESS = 10.0f;

static const float INV_SLD_KNOB_ANCHOR_Y = 10.0f;
static const float INV_SLD_TEXT_FROM_PNG_CENTER_Y = 5.0f;
static const float INV_SLD_TEXT_X_OFFSET = -5.0f;

enum InvItem
{
    INV_ITEM_NONE = -1,
    INV_ITEM_APPLE = 0,
    INV_ITEM_APPLE_SEED = 1
};

static int gSelectedInvItem = INV_ITEM_NONE;
static bool gInvConfirmOpen = false;
static bool gHoverYes = false;
static bool gHoverNo = false;
static bool gHoverApple = false;
static bool gHoverTrash = false;

static const float INV_ITEM_X = -180.0f;
static const float INV_ITEM_Y = 90.0f;
static const float INV_ITEM_SIZE = 90.0f;

static const float INV_TRASH_X = 180.0f;
static const float INV_TRASH_Y = -250.0f;
static const float INV_TRASH_SIZE = 48.0f;

// -------------------------
// Settings panel state
// -------------------------
bool gSoundEnabled = true;
bool gMusicEnabled = true;

static bool gHoverSoundToggle = false;
static bool gHoverMusicToggle = false;
static bool gHoverSettingsExit = false;
static bool gHoverSettingsClose = false;

void MainBGM_SetEnabled(bool enabled);


// -------------------------
// Settings layout
// -------------------------
static const float SET_PANEL_W = 962.0f;
static const float SET_PANEL_H = 609.0f;
static const float SET_PANEL_X = 200.0f;
static const float SET_PANEL_Y = 0.0f;

static const float SET_SOUND_Y = 70.0f;
static const float SET_MUSIC_Y = 15.0f;

static const float SET_TOGGLE_X = 130.0f;
static const float SET_TOGGLE_W = 235.0f;
static const float SET_TOGGLE_H = 57.0f;

static const float SET_CLOSE_X = 230.0f;
static const float SET_CLOSE_Y = 150.0f;
static const float SET_CLOSE_SIZE = 28.0f;

// =========================================================
// TRANSFER SYSTEM - Inventory <-> Crate
// =========================================================

// Transfer panel state
static bool gTransferPanelOpen = false;
static int  gTransferFruitType = -1;   // 0=apple, 1=pear, 2=banana
static int  gTransferAmount = 1;
static bool gTransferSliderDragging = false;

// Two modes:
//   TRANSFER_TO_CRATE    = inventory -> crate (restock stall)
//   TRANSFER_FROM_CRATE  = crate -> inventory (retrieve unsold)
enum TransferMode { TRANSFER_TO_CRATE = 0, TRANSFER_FROM_CRATE = 1 };
static TransferMode gTransferMode = TRANSFER_TO_CRATE;

// Transfer panel layout constants
static const float TR_PANEL_W = 340.0f;
static const float TR_PANEL_H = 260.0f;
static const float TR_PANEL_X = 0.0f;
static const float TR_PANEL_Y = 0.0f;

// Mode toggle buttons (inside panel)
static const float TR_BTN_W = 130.0f;
static const float TR_BTN_H = 36.0f;
static const float TR_BTN_Y = TR_PANEL_Y + TR_PANEL_H * 0.5f - 50.0f;  // near top
static const float TR_BTN_LEFT_X = TR_PANEL_X - TR_BTN_W * 0.5f - 4.0f;
static const float TR_BTN_RIGHT_X = TR_PANEL_X + TR_BTN_W * 0.5f + 4.0f;

// Slider track inside transfer panel
static const float TR_SLD_LEFT_PAD = 40.0f;
static const float TR_SLD_TRACK_W = TR_PANEL_W - TR_SLD_LEFT_PAD * 2.0f;
static const float TR_SLD_Y_OFFSET = -30.0f;   // below centre
static const float TR_SLD_H = 12.0f;
static const float TR_KNOB_W = 20.0f;
static const float TR_KNOB_H = 20.0f;

// Confirm button
static const float TR_CONFIRM_W = 120.0f;
static const float TR_CONFIRM_H = 36.0f;
static const float TR_CONFIRM_Y = TR_PANEL_Y - TR_PANEL_H * 0.5f + 34.0f; // near bottom
static const float TR_CANCEL_X = TR_PANEL_X + TR_CONFIRM_W * 0.5f + 8.0f;
static const float TR_CONFIRM_X = TR_PANEL_X - TR_CONFIRM_W * 0.5f - 8.0f;

// Hover flags for transfer panel
static bool gHoverTRModeLeft = false;
static bool gHoverTRModeRight = false;
static bool gHoverTRConfirm = false;
static bool gHoverTRCancel = false;

// ---- Helper: open the transfer panel for a given crate/fruit ----
static void OpenTransferPanel(int fruitType)
{
    gTransferFruitType = fruitType;
    gTransferPanelOpen = true;
    gTransferMode = TRANSFER_TO_CRATE;   // default: stock the stall
    gTransferAmount = 1;
    gTransferSliderDragging = false;
}

// ---- Execute the transfer ----
static void ExecuteTransfer()
{
    if (gTransferFruitType < 0 || gTransferFruitType > 2) return;
    int amount = gTransferAmount;
    if (amount <= 0) return;

    if (gTransferMode == TRANSFER_TO_CRATE)
    {
        // Inventory -> Crate
        // Verify inventory has enough of this fruit type
        int available = 0;
        switch (gTransferFruitType) {
        case 0: available = GetAppleCount();  break;
        case 1: available = GetPearCount();   break;
        case 2: available = GetBananaCount(); break;
        }
        if (amount > available) amount = available;

        // Verify crate has space
        int crateSpace = Crate_GetMaxStock() - Crate_GetFruitCount(gTransferFruitType);
        if (crateSpace <= 0)
        {
            printf("Transfer failed: crate %d is full.\n", gTransferFruitType);
            return;
        }
        if (amount > crateSpace) amount = crateSpace;

        // Execute
        Inventory_RemoveFruitTyped(static_cast<u8>(amount), static_cast<u8>(gTransferFruitType));
        Crate_AddFruitTyped(gTransferFruitType, amount);
        printf("Transferred %d fruit(type %d) from Inventory -> Crate.\n", amount, gTransferFruitType);
    }
    else // TRANSFER_FROM_CRATE
    {
        // Crate -> Inventory
        int crateStock = Crate_GetFruitCount(gTransferFruitType);
        if (amount > crateStock) amount = crateStock;

        // Verify inventory has space
        int invSpace = GetInventoryLimit() - GetFruitCount();
        if (invSpace <= 0)
        {
            printf("Transfer failed: inventory is full.\n");
            return;
        }
        if (amount > invSpace) amount = invSpace;

        // Execute
        Crate_RemoveFruitTyped(gTransferFruitType, amount);
        Inventory_AddFruit(static_cast<u8>(amount), static_cast<u8>(gTransferFruitType));
        printf("Transferred %d fruit(type %d) from Crate -> Inventory.\n", amount, gTransferFruitType);
    }
}

// ---- Max transferable amount given current state ----
static int GetTransferMax()
{
    if (gTransferFruitType < 0 || gTransferFruitType > 2) return 0;

    if (gTransferMode == TRANSFER_TO_CRATE)
    {
        int available = 0;
        switch (gTransferFruitType) {
        case 0: available = GetAppleCount();  break;
        case 1: available = GetPearCount();   break;
        case 2: available = GetBananaCount(); break;
        }
        int crateSpace = Crate_GetMaxStock() - Crate_GetFruitCount(gTransferFruitType);
        return (std::min)(available, crateSpace);
    }
    else
    {
        int crateStock = Crate_GetFruitCount(gTransferFruitType);
        int invSpace = GetInventoryLimit() - GetFruitCount();
        return (std::min)(crateStock, invSpace);
    }
}

// =========================================================
// END TRANSFER SYSTEM
// =========================================================


void UI_Init()
{
    menuTexture = AEGfxTextureLoad("Assets/MenuMockup.PNG");
    seedsTexture = AEGfxTextureLoad("Assets/SeedsPanel.png");

    inventoryIcon = AEGfxTextureLoad("Assets/Inventory.png");
    inventoryBG = AEGfxTextureLoad("Assets/InventoryPanelBG.png");
    invFruit = AEGfxTextureLoad("Assets/InvFruit.png");
    invFruitHighlight = AEGfxTextureLoad("Assets/InvFruitHighlight.png");
    invSeed = AEGfxTextureLoad("Assets/InvSeed.png");
    invSeedHighlight = AEGfxTextureLoad("Assets/InvSeedHighlight.png");
    invSliderKnob = AEGfxTextureLoad("Assets/InvSliderKnob.png");
    invSliderFill = AEGfxTextureLoad("Assets/InvSliderFill.png");
    appleIcon = AEGfxTextureLoad("Assets/HarvestApple.png");
    trashIcon = AEGfxTextureLoad("Assets/Trash.png");

    confirmBG = AEGfxTextureLoad("Assets/ConfirmBG.png");
    confirmYes = AEGfxTextureLoad("Assets/Yes.png");
    confirmNo = AEGfxTextureLoad("Assets/No.png");

    collectionIcon = AEGfxTextureLoad("Assets/Collection.png");
    collectionBG = AEGfxTextureLoad("Assets/collectionBG.png");

    settingsIcon = AEGfxTextureLoad("Assets/Settings.png");
    settingsBG = AEGfxTextureLoad("Assets/Settings_BG.png");
    settingsOn = AEGfxTextureLoad("Assets/Settings_ON.png");
    settingsOff = AEGfxTextureLoad("Assets/Settings_OFF.png");


    appleSeedIcon = AEGfxTextureLoad("Assets/AppleSeed.png");
    pearSeedIcon = AEGfxTextureLoad("Assets/PearSeed.png");
    appleSeedInfo = AEGfxTextureLoad("Assets/AppleSeedInfo.png");
    plotSlotTexture = AEGfxTextureLoad("Assets/Plot1.png");
    leftArrowTexture = AEGfxTextureLoad("Assets/ArrowLeft.png");
    rightArrowTexture = AEGfxTextureLoad("Assets/ArrowRight.png");

    // --- Menu Buttons ---
    menuButtons.clear();
    float menuCenterX = -530.0f;
    float buttonSize = 100.0f;
    float buttonSpacing = 140.0f;
    float buttonY = -360.0f;

    ButtonType buttonOrder[] = { BUTTON_INVENTORY, BUTTON_COLLECTION, BUTTON_SETTINGS };
    for (int i = 0; i < 3; ++i)
    {
        menuButtons.push_back({ menuCenterX + (i - 1) * buttonSpacing, buttonY, buttonSize, buttonSize, false, buttonOrder[i] });
    }


    //plot setup
    plotPlusButton.x = -630.0f;
    plotPlusButton.y = 150.0f;
    plotPlusButton.width = 120.0f;
    plotPlusButton.height = 120.0f;

    float slotSize = 120.0f;
    float spacing = 150.0f;

    int cols = 2;
    int rows = 2;

    float panelCenterX = -510.0f;
    float panelCenterY = 150.0f;
    float totalWidth = (cols - 1) * spacing + slotSize;
    float totalHeight = (rows - 1) * spacing + slotSize;

    float startX = panelCenterX - totalWidth * 0.5f + slotSize * 0.5f;
    float startY = panelCenterY + totalHeight * 0.5f - slotSize * 0.5f;

    plotSlots.clear();

    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < cols; col++)
        {
            PlotSlot slot{};
            slot.x = startX + col * spacing;
            slot.y = startY - row * spacing;
            slot.width = slotSize;
            slot.height = slotSize;

            plotSlots.push_back(slot);
        }
    }

    (void)Upgrades_GetList();
}

void UI_Input()
{
    if (AEInputCheckTriggered(AEVK_M))
        menuOpen = !menuOpen;

    if (menuOpen)
        UI_UpdateButtons();
}

void UI_UpdateButtons()
{
    int mx, my;
    AEInputGetCursorPosition(&mx, &my);

    float worldX = static_cast<float>(mx) - 800.0f;
    float worldY = 450.0f - static_cast<float>(my);

    bool clickThisFrame = AEInputCheckTriggered(AEVK_LBUTTON);
    bool clickConsumed = false;

    // -------------------------------------------------
    // TRANSFER PANEL INPUT (highest priority when open)
    // -------------------------------------------------
    if (gTransferPanelOpen)
    {
        auto PtInRect = [](float px, float py, float cx, float cy, float hw, float hh) -> bool
            {
                return px >= cx - hw && px <= cx + hw && py >= cy - hh && py <= cy + hh;
            };

        // Mode toggle — "To Stall" button (left)
        gHoverTRModeLeft = PtInRect(worldX, worldY, TR_BTN_LEFT_X, TR_BTN_Y, TR_BTN_W * 0.5f, TR_BTN_H * 0.5f);
        // Mode toggle — "To Inventory" button (right)
        gHoverTRModeRight = PtInRect(worldX, worldY, TR_BTN_RIGHT_X, TR_BTN_Y, TR_BTN_W * 0.5f, TR_BTN_H * 0.5f);

        // Confirm button
        gHoverTRConfirm = PtInRect(worldX, worldY, TR_CONFIRM_X, TR_CONFIRM_Y, TR_CONFIRM_W * 0.5f, TR_CONFIRM_H * 0.5f);
        // Cancel button
        gHoverTRCancel = PtInRect(worldX, worldY, TR_CANCEL_X, TR_CONFIRM_Y, TR_CONFIRM_W * 0.5f, TR_CONFIRM_H * 0.5f);

        // --- Slider ---
        const float trSliderX0 = TR_PANEL_X - TR_SLD_TRACK_W * 0.5f;
        const float trSliderX1 = TR_PANEL_X + TR_SLD_TRACK_W * 0.5f;
        const float trSliderY = TR_PANEL_Y + TR_SLD_Y_OFFSET;

        const int trMax = GetTransferMax();
        const int trMin = (trMax > 0) ? 1 : 0;

        // Clamp current value to valid range
        gTransferAmount = (std::max)(trMin, (std::min)(trMax, gTransferAmount));

        float trT = (trMax > trMin) ? (float)(gTransferAmount - trMin) / (float)(trMax - trMin) : 0.0f;
        float trKnobX = trSliderX0 + trT * (trSliderX1 - trSliderX0);

        bool overTRKnob = PtInRect(worldX, worldY, trKnobX, trSliderY, TR_KNOB_W * 0.5f, TR_KNOB_H * 0.5f);
        bool overTRTrack = PtInRect(worldX, worldY, TR_PANEL_X, trSliderY,
            TR_SLD_TRACK_W * 0.5f, (std::max)(TR_SLD_H, 10.0f));

        const bool mouseDown = AEInputCheckCurr(AEVK_LBUTTON) != 0;
        const bool mousePress = AEInputCheckTriggered(AEVK_LBUTTON) != 0;

        if (mousePress && (overTRTrack || overTRKnob) && trMax >= trMin && (trSliderX1 > trSliderX0))
        {
            float cx = (worldX < trSliderX0) ? trSliderX0 : (worldX > trSliderX1 ? trSliderX1 : worldX);
            float tt = (cx - trSliderX0) / (trSliderX1 - trSliderX0);
            int nv = (int)std::round((float)trMin + tt * (float)(trMax - trMin));
            gTransferAmount = (std::max)(trMin, (std::min)(trMax, nv));
            gTransferSliderDragging = true;
        }
        if (!mouseDown) gTransferSliderDragging = false;
        if (gTransferSliderDragging && trMax >= trMin && (trSliderX1 > trSliderX0))
        {
            float cx = (worldX < trSliderX0) ? trSliderX0 : (worldX > trSliderX1 ? trSliderX1 : worldX);
            float tt = (cx - trSliderX0) / (trSliderX1 - trSliderX0);
            int nv = (int)std::round((float)trMin + tt * (float)(trMax - trMin));
            gTransferAmount = (std::max)(trMin, (std::min)(trMax, nv));
        }

        if (clickThisFrame)
        {
            if (gHoverTRModeLeft)
            {
                gTransferMode = TRANSFER_TO_CRATE;
                gTransferAmount = 1;
                clickConsumed = true;
            }
            else if (gHoverTRModeRight)
            {
                gTransferMode = TRANSFER_FROM_CRATE;
                gTransferAmount = 1;
                clickConsumed = true;
            }
            else if (gHoverTRConfirm)
            {
                ExecuteTransfer();
                gTransferPanelOpen = false;
                gTransferFruitType = -1;
                clickConsumed = true;
            }
            else if (gHoverTRCancel)
            {
                gTransferPanelOpen = false;
                gTransferFruitType = -1;
                clickConsumed = true;
            }
            else
            {
                // Click outside panel -> close it
                bool insidePanel = PtInRect(worldX, worldY, TR_PANEL_X, TR_PANEL_Y,
                    TR_PANEL_W * 0.5f, TR_PANEL_H * 0.5f);
                if (!insidePanel)
                {
                    gTransferPanelOpen = false;
                    gTransferFruitType = -1;
                    clickConsumed = true;
                }
            }
        }

        // Block all other input while transfer panel is open
        return;
    }

    // -------------------------------------------------
    // CRATE CLICK — open transfer panel on left-click
    // -------------------------------------------------
    if (clickThisFrame && !clickConsumed)
    {
        const auto& baskets = GetFruitBaskets();
        for (int i = 0; i < (int)baskets.size(); i++)
        {
            const FruitBasket& b = baskets[i];
            bool over =
                worldX >= b.x - b.width * 0.5f &&
                worldX <= b.x + b.width * 0.5f &&
                worldY >= b.y - b.height * 0.5f &&
                worldY <= b.y + b.height * 0.5f;

            if (over && Crate_IsUnlocked(i))
            {
                OpenTransferPanel(i);   // i == fruitType (0=apple,1=pear,2=banana)
                clickConsumed = true;
                break;
            }
        }
    }

    // -------------------------------------------------
    // MENU BUTTONS
    // -------------------------------------------------
    for (auto& button : menuButtons)
    {
        button.isHovered =
            worldX >= button.x - button.width * 0.5f &&
            worldX <= button.x + button.width * 0.5f &&
            worldY >= button.y - button.height * 0.5f &&
            worldY <= button.y + button.height * 0.5f;

        if (button.isHovered && clickThisFrame)
        {
            clickConsumed = true;

            if (popupOpen && activePopupIndex == button.type)
            {
                popupOpen = false;
                activePopupIndex = -1;
            }
            else
            {
                popupOpen = true;
                activePopupIndex = button.type;
            }
        }

    }

    // -------------------------------------------------
    // PLOT "+" BUTTON (TOGGLE PANEL)
    // -------------------------------------------------
    plotPlusButton.isHovered =
        worldX >= plotPlusButton.x - plotPlusButton.width * 0.5f &&
        worldX <= plotPlusButton.x + plotPlusButton.width * 0.5f &&
        worldY >= plotPlusButton.y - plotPlusButton.height * 0.5f &&
        worldY <= plotPlusButton.y + plotPlusButton.height * 0.5f;

    if (plotPlusButton.isHovered && AEInputCheckTriggered(AEVK_LBUTTON))
    {
        seedsPopupOpen = !seedsPopupOpen;

        if (!seedsPopupOpen)
            selectedSeed = -1;
    }

    // -------------------------------------------------
    // SEED SELECTION (ONLY IF PANEL OPEN)
    // -------------------------------------------------
    if (seedsPopupOpen)
    {
        float panelX = -70.0f;
        float panelY = 20.0f;
        float seedY = panelY + 120.0f;
        float seedW = 100.0f;
        float seedH = 100.0f;

        bool overSeed =
            worldX >= panelX - seedW * 0.5f &&
            worldX <= panelX + seedW * 0.5f &&
            worldY >= seedY - seedH * 0.5f &&
            worldY <= seedY + seedH * 0.5f;

        float arrowOffsetX = 150.0f;
        float arrowY = panelY + 150.0f;
        float arrowSize = 60.0f;

        bool overLeft =
            worldX >= (panelX - arrowOffsetX - arrowSize) &&
            worldX <= (panelX - arrowOffsetX + arrowSize) &&
            worldY >= (arrowY - arrowSize) &&
            worldY <= (arrowY + arrowSize);

        bool overRight =
            worldX >= (panelX + arrowOffsetX - arrowSize) &&
            worldX <= (panelX + arrowOffsetX + arrowSize) &&
            worldY >= (arrowY - arrowSize) &&
            worldY <= (arrowY + arrowSize);

        if (clickThisFrame)
        {
            if (overLeft)
            {
                currentSeedIndex--;
                if (currentSeedIndex < 0)
                    currentSeedIndex = SEED_COUNT - 1;

                clickConsumed = true;
                return;
            }

            if (overRight)
            {
                currentSeedIndex++;
                if (currentSeedIndex >= SEED_COUNT)
                    currentSeedIndex = 0;

                clickConsumed = true;
                return;
            }
        }


        if (overSeed)
            hoveredSeed = currentSeedIndex;
        else
            hoveredSeed = -1;
        if (overSeed && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            selectedSeed = currentSeedIndex;

            if (activePlotIndex != -1)
            {
                int plotToPlant = activePlotIndex;

                Farm_PlantSeed(plotToPlant, currentSeedIndex);
                Inventory_RemoveSeed(1, currentSeedIndex);

                std::cout << "Planted seed type: " << currentSeedIndex
                    << " on plot: " << plotToPlant << "\n";

                seedsPopupOpen = false;
                activePlotIndex = -1;
            }
        }
    }

    // -------------------------------------------------
    // PLOT SLOT HOVER
    // -------------------------------------------------

    hoveredPlotIndex = -1;

    for (size_t i = 0; i < plotSlots.size(); i++)
    {
        PlotSlot& slot = plotSlots[i];

        bool isOver =
            worldX >= slot.x - slot.width * 0.5f &&
            worldX <= slot.x + slot.width * 0.5f &&
            worldY >= slot.y - slot.height * 0.5f &&
            worldY <= slot.y + slot.height * 0.5f;

        if (isOver)
        {
            hoveredPlotIndex = static_cast<int>(i);

            if (Farm_IsPlotLocked(static_cast<int>(i)))
                break;

            if (AEInputCheckTriggered(AEVK_LBUTTON))
            {
                if (seedsPopupOpen && activePlotIndex == i)
                {
                    seedsPopupOpen = false;
                    selectedSeed = -1;
                    activePlotIndex = -1;
                }
                else
                {
                    seedsPopupOpen = true;
                    activePlotIndex = static_cast<int>(i);
                    selectedSeed = currentSeedIndex;
                }
            }

            break;
        }
    }

    // DELETE SEED BUTTON (CLICK LOGIC ONLY)
    for (size_t i = 0; i < plotSlots.size(); i++)
    {
        if (!Farm_IsPlotPlanted(static_cast<int>(i)))
            continue;

        float xSize = 25.0f;

        float offsetX = -45.0f;
        float offsetY = 45.0f;

        float xPos = plotSlots[i].x + offsetX;
        float yPos = plotSlots[i].y + offsetY;

        bool overDelete =
            worldX >= xPos - xSize * 0.5f &&
            worldX <= xPos + xSize * 0.5f &&
            worldY >= yPos - xSize * 0.5f &&
            worldY <= yPos + xSize * 0.5f;

        if (overDelete && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            Farm_ClearPlot(static_cast<int>(i));
            break;
        }
    }
    float upgradesPanelW = UPGRADES_PANEL_W;

    float panelX = UPGRADES_PANEL_X;
    float panelY = UPGRADES_PANEL_Y;

    float spacingUp = 70.0f;
    float startYUp = panelY + 60.0f;

    auto& upgrades = Upgrades_GetList();

    int shownUp = 0;
    for (size_t i = upgradesStartIndex; i < upgrades.size() && shownUp < MAX_VISIBLE_UPGRADES; ++i)
    {
        auto& u = upgrades[i];
        if (u.purchased) continue;

        float y = startYUp - shownUp * spacingUp;

        float boxW = upgradesPanelW - 40.0f;
        float boxH = 50.0f;

        bool over =
            worldX >= panelX - boxW * 0.5f &&
            worldX <= panelX + boxW * 0.5f &&
            worldY >= y - boxH * 0.5f &&
            worldY <= y + boxH * 0.5f;

        if (over && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            if (Upgrades_CanPurchase(u, Economy_GetTotalMoney()))
            {
                Upgrades_Purchase(u.id);
            }
            else
            {
                std::cout << "Cannot afford upgrade " << static_cast<int>(u.id)
                    << " (cost=" << u.cost << ", money=" << Economy_GetTotalMoney() << ")\n";
            }
            break;
        }

        ++shownUp;
    }

    // ================= Inventory header icons input =================
    if (popupOpen && activePopupIndex == BUTTON_INVENTORY)
    {
        const float invpanelX = 0.0f, invpanelY = 0.0f;
        const float leftEdge = invpanelX - INV_PANEL_W * 0.5f;
        const float topEdge = invpanelY + INV_PANEL_H * 0.5f;

        const float slotW = INV_FRUIT_W;
        const float slotH = INV_FRUIT_H;

        const float fruitCx = leftEdge + INV_TAB_LEFT_PAD + slotW * 0.5f;
        const float fruitCy = topEdge - INV_TAB_TOP_PAD - slotH * 0.5f;

        const float seedCx = fruitCx + INV_TAB_SPACING_X;
        const float seedCy = fruitCy;

        const float hw = slotW * 0.5f;
        const float hh = slotH * 0.5f;

        auto PtIn = [&](float px, float py, float cx, float cy, float halfW, float halfH) -> bool {
            return px >= cx - halfW && px <= cx + halfW &&
                py >= cy - halfH && py <= cy + halfH;
            };

        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            if (PtIn(worldX, worldY, fruitCx, fruitCy, hw, hh))
            {
                gActiveInvTab = TAB_FRUITS;
            }
            else if (PtIn(worldX, worldY, seedCx, seedCy, hw, hh))
            {
                gActiveInvTab = TAB_SEEDS;
            }
        }
    }

    // ================= Inventory slider input =================
    if (popupOpen && activePopupIndex == BUTTON_INVENTORY)
    {
        const float panelCenterX = 0.0f, panelCenterY = 0.0f;
        const float leftEdge = panelCenterX - INV_PANEL_W * 0.5f;
        const float topEdge = panelCenterY + INV_PANEL_H * 0.5f;

        const float baseX0 = leftEdge + INV_SLD_LEFT_PAD;
        const float baseX1 = baseX0 + INV_SLD_TRACK_W;

        const float trackX0 = baseX0 + INV_SLD_TRACK_CAP_INSET;
        const float trackX1 = baseX1 - INV_SLD_TRACK_CAP_INSET;

        const float trackY = (topEdge - INV_SLD_TOP_PAD) + INV_SLD_FILL_Y_OFFSET;

        const int curCount = (gActiveInvTab == TAB_FRUITS)
            ? GetFruitCount()
            : GetSeedCount();


        const int minVal = (curCount > 0) ? 1 : 0;
        const int maxVal = curCount;

        gInvSliderValue = (std::max)(minVal, (std::min)(maxVal, gInvSliderValue));

        float t = (maxVal > minVal) ? float(gInvSliderValue - minVal) / float(maxVal - minVal) : 0.0f;
        float knobX = trackX0 + t * (trackX1 - trackX0);
        float knobY = trackY;

        auto PtIn = [](float px, float py, float cx, float cy, float hw, float hh)->bool {
            return (px >= cx - hw) && (px <= cx + hw) && (py >= cy - hh) && (py <= cy + hh);
            };

        const float knobHW = (std::max)(INV_SLD_KNOB_W * 0.6f, 18.0f);
        const float knobHH = (std::max)(INV_SLD_KNOB_H * 0.6f, 18.0f);
        const float trackHW = (trackX1 - trackX0) * 0.5f;
        const float trackHH = (std::max)(INV_SLD_FILL_THICKNESS * 0.8f, 10.0f);

        const bool overKnob = PtIn(worldX, worldY, knobX, knobY, knobHW, knobHH);
        const bool overTrack = PtIn(worldX, worldY, 0.5f * (trackX0 + trackX1), trackY, trackHW, trackHH);

        const bool mouseDown = AEInputCheckCurr(AEVK_LBUTTON) != 0;
        const bool mousePress = AEInputCheckTriggered(AEVK_LBUTTON) != 0;

        if (mousePress && overTrack && maxVal >= minVal && (trackX1 > trackX0))
        {
            float clampedX = (worldX < trackX0) ? trackX0 : ((worldX > trackX1) ? trackX1 : worldX);
            float tt = (clampedX - trackX0) / (trackX1 - trackX0);
            int newVal = (int)std::round((float)minVal + tt * (float)(maxVal - minVal));
            gInvSliderValue = (std::max)(minVal, (std::min)(maxVal, newVal));
            gInvSliderDragging = true;
        }
        else if (mousePress && overKnob)
        {
            gInvSliderDragging = true;
        }

        if (!mouseDown) gInvSliderDragging = false;

        if (gInvSliderDragging && maxVal >= minVal && (trackX1 > trackX0))
        {
            float clampedX = (worldX < trackX0) ? trackX0 : ((worldX > trackX1) ? trackX1 : worldX);
            float tt = (clampedX - trackX0) / (trackX1 - trackX0);
            int newVal = (int)std::round((float)minVal + tt * (float)(maxVal - minVal));
            gInvSliderValue = (std::max)(minVal, (std::min)(maxVal, newVal));
        }
    }

    // ================= Inventory item click =================
    if (popupOpen && activePopupIndex == BUTTON_INVENTORY)
    {

        const float invpanelX = 0.0f;
        const float invpanelY = 0.0f;

        float itemX = invpanelX + INV_ITEM_X;
        float itemY = invpanelY + INV_ITEM_Y;

        gHoverApple =
            worldX >= itemX - INV_ITEM_SIZE * 0.5f &&
            worldX <= itemX + INV_ITEM_SIZE * 0.5f &&
            worldY >= itemY - INV_ITEM_SIZE * 0.5f &&
            worldY <= itemY + INV_ITEM_SIZE * 0.5f;

        if (gHoverApple && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            gSelectedInvItem = (gActiveInvTab == TAB_FRUITS) ? INV_ITEM_APPLE : INV_ITEM_APPLE_SEED;

            gInvSliderValue = 1;
        }

    }
    // Trash
    if (popupOpen && activePopupIndex == BUTTON_INVENTORY && gSelectedInvItem != INV_ITEM_NONE)
    {
        const float invpanelX = 0.0f;
        const float invpanelY = 0.0f;

        float tx = invpanelX + INV_TRASH_X;
        float ty = invpanelY + INV_TRASH_Y;

        gHoverTrash =
            worldX >= tx - INV_TRASH_SIZE * 0.5f &&
            worldX <= tx + INV_TRASH_SIZE * 0.5f &&
            worldY >= ty - INV_TRASH_SIZE * 0.5f &&
            worldY <= ty + INV_TRASH_SIZE * 0.5f;

        if (gHoverTrash && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            if (gSelectedInvItem != INV_ITEM_NONE)
                gInvConfirmOpen = true;
        }
    }

    //Confirmation
    if (gInvConfirmOpen)
    {
        auto Btn = [&](float x, float y)
            {
                return worldX >= x - 40 && worldX <= x + 40 &&
                    worldY >= y - 20 && worldY <= y + 20;
            };

        // YES
        if (Btn(-60, -40) && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            if (gSelectedInvItem == INV_ITEM_APPLE)
                Inventory_RemoveFruit(static_cast<u8>(gInvSliderValue));
            else
                Inventory_RemoveSeed(static_cast<u8>(gInvSliderValue), static_cast<u8>(gSelectedInvItem - INV_ITEM_APPLE_SEED));

            gInvConfirmOpen = false;
            gSelectedInvItem = INV_ITEM_NONE;
        }

        // NO
        if (Btn(60, -40) && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            gInvConfirmOpen = false;
        }

        const float btnW = 80.0f;
        const float btnH = 40.0f;

        const float yesX = -60.0f;
        const float yesY = -40.0f;

        const float noX = 60.0f;
        const float noY = -40.0f;

        gHoverYes =
            worldX >= yesX - btnW * 0.5f &&
            worldX <= yesX + btnW * 0.5f &&
            worldY >= yesY - btnH * 0.5f &&
            worldY <= yesY + btnH * 0.5f;

        gHoverNo =
            worldX >= noX - btnW * 0.5f &&
            worldX <= noX + btnW * 0.5f &&
            worldY >= noY - btnH * 0.5f &&
            worldY <= noY + btnH * 0.5f;

    }

    // ================= Settings input =================
    if (popupOpen && activePopupIndex == BUTTON_SETTINGS)
    {
        // SOUND toggle
        gHoverSoundToggle =
            worldX >= (SET_PANEL_X + SET_TOGGLE_X) - SET_TOGGLE_W * 0.5f &&
            worldX <= (SET_PANEL_X + SET_TOGGLE_X) + SET_TOGGLE_W * 0.5f &&
            worldY >= (SET_PANEL_Y + SET_SOUND_Y + 45.0f) - SET_TOGGLE_H * 0.5f &&
            worldY <= (SET_PANEL_Y + SET_SOUND_Y + 45.0f) + SET_TOGGLE_H * 0.5f;

        if (gHoverSoundToggle && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            gSoundEnabled = !gSoundEnabled;
            UIAudio_EnableSFX(gSoundEnabled);
            UIAudio_PlayToggle();
        }

        // MUSIC toggle
        gHoverMusicToggle =
            worldX >= (SET_PANEL_X + SET_TOGGLE_X) - SET_TOGGLE_W * 0.5f &&
            worldX <= (SET_PANEL_X + SET_TOGGLE_X) + SET_TOGGLE_W * 0.5f &&
            worldY >= (SET_PANEL_Y + SET_MUSIC_Y + 5.0f) - SET_TOGGLE_H * 0.5f &&
            worldY <= (SET_PANEL_Y + SET_MUSIC_Y + 5.0f) + SET_TOGGLE_H * 0.5f;

        if (gHoverMusicToggle && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            gMusicEnabled = !gMusicEnabled;
            UIAudio_SetMusicEnabled(gMusicEnabled);
            MainBGM_SetEnabled(gMusicEnabled);
            UIAudio_PlayToggle();
        }
    }
    // -------------------------------------------------
    // CLOSE POPUP WHEN CLICKING OUTSIDE
    // -------------------------------------------------
    if (popupOpen && AEInputCheckTriggered(AEVK_LBUTTON))
    {
        bool clickInside = false;

        switch (activePopupIndex)
        {
        case BUTTON_INVENTORY:
        {
            clickInside =
                worldX >= -INV_PANEL_W * 0.5f &&
                worldX <= INV_PANEL_W * 0.5f &&
                worldY >= -INV_PANEL_H * 0.5f &&
                worldY <= INV_PANEL_H * 0.5f;
            break;
        }

        case BUTTON_COLLECTION:
        {
            const float panelCenterX = 180.0f;
            const float panelCenterY = 0.0f;

            const float panelHalfW = 800.0f * 0.5f;
            const float panelHalfH = 600.0f * 0.4f;

            clickInside =
                worldX >= panelCenterX - panelHalfW &&
                worldX <= panelCenterX + panelHalfW &&
                worldY >= panelCenterY - panelHalfH &&
                worldY <= panelCenterY + panelHalfH;

            break;
        }
        case BUTTON_SETTINGS:
        {
            float panelHalfW = (SET_PANEL_W * ScaleX) * 0.5f;
            float panelHalfH = (SET_PANEL_H * ScaleY) * 0.46f;

            clickInside =
                worldX >= SET_PANEL_X - panelHalfW &&
                worldX <= SET_PANEL_X + panelHalfW &&
                worldY >= SET_PANEL_Y - panelHalfH &&
                worldY <= SET_PANEL_Y + panelHalfH;
            break;
        }
        }

        if (clickInside)
        {
            clickConsumed = true;
        }

        if (!clickInside && !clickConsumed)
        {
            popupOpen = false;
            activePopupIndex = -1;
        }
    }

}


void UI_Draw()
{
    // Draw transfer panel first if open (drawn regardless of menuOpen)
    if (gTransferPanelOpen)
    {
        AEMtx33 scale, trans, transform;

        // --- Panel background (dark translucent box) ---
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(0.12f, 0.10f, 0.08f, 0.92f);

        AEMtx33Scale(&scale, TR_PANEL_W, TR_PANEL_H);
        AEMtx33Trans(&trans, TR_PANEL_X, TR_PANEL_Y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        // Panel border (slightly lighter)
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(0.80f, 0.65f, 0.20f, 0.60f);
        AEMtx33Scale(&scale, TR_PANEL_W + 4.0f, TR_PANEL_H + 4.0f);
        AEMtx33Trans(&trans, TR_PANEL_X, TR_PANEL_Y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        // Redraw inner background on top of border
        AEGfxSetColorToMultiply(0.12f, 0.10f, 0.08f, 0.92f);
        AEMtx33Scale(&scale, TR_PANEL_W, TR_PANEL_H);
        AEMtx33Trans(&trans, TR_PANEL_X, TR_PANEL_Y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        const float halfW = 800.0f;
        const float halfH = 450.0f;

        // --- Title ---
        static const char* fruitNames[] = { "Apple", "Pear", "Banana" };
        char titleBuf[64] = "Transfer Fruit";
        if (gTransferFruitType >= 0 && gTransferFruitType <= 2)
            sprintf_s(titleBuf, "Transfer %s", fruitNames[gTransferFruitType]);

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxPrint(fontId, titleBuf,
            (TR_PANEL_X - 70.0f) / halfW,
            (TR_PANEL_Y + TR_PANEL_H * 0.5f - 18.0f) / halfH,
            0.85f, 1.0f, 0.9f, 0.5f, 1.0f);

        // --- Mode toggle buttons ---
        // "To Stall" button
        {
            bool active = (gTransferMode == TRANSFER_TO_CRATE);
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            float r = active ? 0.85f : 0.30f;
            float g2 = active ? 0.65f : 0.30f;
            float b = active ? 0.15f : 0.30f;
            float al = (gHoverTRModeLeft && !active) ? 0.55f : (active ? 1.0f : 0.45f);
            AEGfxSetColorToMultiply(r, g2, b, al);
            AEMtx33Scale(&scale, TR_BTN_W, TR_BTN_H);
            AEMtx33Trans(&trans, TR_BTN_LEFT_X, TR_BTN_Y);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxPrint(fontId, "To Stall",
                (TR_BTN_LEFT_X - 38.0f) / halfW,
                (TR_BTN_Y - 8.0f) / halfH,
                0.7f, 1.0f, 1.0f, 0.8f, 1.0f);
        }

        // "To Inventory" button
        {
            bool active = (gTransferMode == TRANSFER_FROM_CRATE);
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            float r = active ? 0.20f : 0.30f;
            float g2 = active ? 0.65f : 0.30f;
            float b = active ? 0.85f : 0.30f;
            float al = (gHoverTRModeRight && !active) ? 0.55f : (active ? 1.0f : 0.45f);
            AEGfxSetColorToMultiply(r, g2, b, al);
            AEMtx33Scale(&scale, TR_BTN_W, TR_BTN_H);
            AEMtx33Trans(&trans, TR_BTN_RIGHT_X, TR_BTN_Y);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxPrint(fontId, "To Inventory",
                (TR_BTN_RIGHT_X - 55.0f) / halfW,
                (TR_BTN_Y - 8.0f) / halfH,
                0.7f, 1.0f, 1.0f, 0.8f, 1.0f);
        }

        // --- Stock info line ---
        {
            int invCount = 0;
            switch (gTransferFruitType) {
            case 0: invCount = GetAppleCount();  break;
            case 1: invCount = GetPearCount();   break;
            case 2: invCount = GetBananaCount(); break;
            }
            int crateCount = (gTransferFruitType >= 0) ? Crate_GetFruitCount(gTransferFruitType) : 0;
            int crateMax = Crate_GetMaxStock();
            int invMax = GetInventoryLimit();

            char infoBuf[96];
            sprintf_s(infoBuf, "Inv: %d/%d   Stall: %d/%d", invCount, invMax, crateCount, crateMax);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxPrint(fontId, infoBuf,
                (TR_PANEL_X - 135.0f) / halfW,
                (TR_PANEL_Y + 40.0f) / halfH,
                0.65f, 0.8f, 0.9f, 1.0f, 1.0f);
        }

        // --- Slider ---
        {
            const float trSliderX0 = TR_PANEL_X - TR_SLD_TRACK_W * 0.5f;
            const float trSliderX1 = TR_PANEL_X + TR_SLD_TRACK_W * 0.5f;
            const float trSliderY = TR_PANEL_Y + TR_SLD_Y_OFFSET;

            const int trMax = GetTransferMax();
            const int trMin = (trMax > 0) ? 1 : 0;
            gTransferAmount = (std::max)(trMin, (std::min)(trMax, gTransferAmount));

            float trT = (trMax > trMin)
                ? (float)(gTransferAmount - trMin) / (float)(trMax - trMin)
                : 0.0f;
            float trKnobX = trSliderX0 + trT * (trSliderX1 - trSliderX0);

            // Track background
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(0.4f, 0.4f, 0.4f, 0.8f);
            AEMtx33Scale(&scale, TR_SLD_TRACK_W, TR_SLD_H);
            AEMtx33Trans(&trans, TR_PANEL_X, trSliderY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            // Fill
            float fillW = (trSliderX1 - trSliderX0) * trT;
            if (fillW > 0.5f)
            {
                AEGfxSetColorToMultiply(0.9f, 0.75f, 0.2f, 1.0f);
                AEMtx33Scale(&scale, fillW, TR_SLD_H);
                AEMtx33Trans(&trans, trSliderX0 + fillW * 0.5f, trSliderY);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }

            // Knob
            AEGfxSetColorToMultiply(1.0f, 0.95f, 0.6f, 1.0f);
            AEMtx33Scale(&scale, TR_KNOB_W, TR_KNOB_H);
            AEMtx33Trans(&trans, trKnobX, trSliderY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            // Min / max labels
            char minLbl[8], maxLbl[8];
            sprintf_s(minLbl, "%d", trMin);
            sprintf_s(maxLbl, "%d", trMax);
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxPrint(fontId, minLbl, (trSliderX0 - 6.0f) / halfW, (trSliderY - 22.0f) / halfH, 0.7f, 0.8f, 0.8f, 0.8f, 1.0f);
            AEGfxPrint(fontId, maxLbl, (trSliderX1 - 6.0f) / halfW, (trSliderY - 22.0f) / halfH, 0.7f, 0.8f, 0.8f, 0.8f, 1.0f);

            // Current value above knob
            char valLbl[8];
            sprintf_s(valLbl, "%d", gTransferAmount);
            AEGfxPrint(fontId, valLbl, (trKnobX - 6.0f) / halfW, (trSliderY + 26.0f) / halfH, 0.8f, 1.0f, 1.0f, 0.6f, 1.0f);

            // "Amount:" label
            AEGfxPrint(fontId, "Amount:",
                (TR_PANEL_X - 135.0f) / halfW,
                (trSliderY + 45.0f) / halfH,
                0.7f, 0.85f, 0.85f, 0.85f, 1.0f);
        }

        // --- Direction arrow indicator (text) ---
        {
            const char* arrowTxt = (gTransferMode == TRANSFER_TO_CRATE)
                ? "Inventory  -->  Stall"
                : "Stall  -->  Inventory";

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxPrint(fontId, arrowTxt,
                (TR_PANEL_X - 95.0f) / halfW,
                (TR_PANEL_Y - 10.0f) / halfH,
                0.70f, 0.7f, 1.0f, 0.7f, 1.0f);
        }

        // --- Confirm button ---
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            float al = gHoverTRConfirm ? 1.0f : 0.80f;
            AEGfxSetColorToMultiply(0.20f, 0.70f, 0.25f, al);
            AEMtx33Scale(&scale, TR_CONFIRM_W, TR_CONFIRM_H);
            AEMtx33Trans(&trans, TR_CONFIRM_X, TR_CONFIRM_Y);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxPrint(fontId, "Confirm",
                (TR_CONFIRM_X - 34.0f) / halfW,
                (TR_CONFIRM_Y - 8.0f) / halfH,
                0.75f, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        // --- Cancel button ---
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            float al = gHoverTRCancel ? 1.0f : 0.80f;
            AEGfxSetColorToMultiply(0.70f, 0.20f, 0.20f, al);
            AEMtx33Scale(&scale, TR_CONFIRM_W, TR_CONFIRM_H);
            AEMtx33Trans(&trans, TR_CANCEL_X, TR_CONFIRM_Y);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxPrint(fontId, "Cancel",
                (TR_CANCEL_X - 30.0f) / halfW,
                (TR_CONFIRM_Y - 8.0f) / halfH,
                0.75f, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    // THEN menu stuff
    if (!menuOpen)
        return;


    AEMtx33 scale, trans, transform;

    // --- Menu Background ---
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxTextureSet(menuTexture, 0, 0);

    AEMtx33Scale(&scale, 480, 850);
    AEMtx33Trans(&trans, -770 + 240, 0);
    AEMtx33Concat(&transform, &trans, &scale);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

    // --- Gold number text ---
    char goldText[32];
    sprintf_s(goldText, "%d", Economy_GetTotalMoney());


    float x = -530.0f / 800.0f;
    float y = 350.0f / 450.0f;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(0, 0, 0, 1);

    AEGfxPrint(fontId, goldText, x, y, 1.2f, 0, 0, 0, 1);

    // --- Menu Buttons ---
    for (auto& button : menuButtons)
    {
        AEGfxTextureSet(
            button.type == BUTTON_INVENTORY ? inventoryIcon :
            button.type == BUTTON_COLLECTION ? collectionIcon :
            settingsIcon, 0, 0);

        AEGfxSetColorToMultiply(
            1.0f,
            button.isHovered ? 0.9f : 1.0f,
            button.isHovered ? 0.9f : 1.0f,
            1.0f);

        AEMtx33Scale(&scale, button.width, button.height);
        AEMtx33Trans(&trans, button.x, button.y);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
    }

    if (popupOpen)
    {
        switch (activePopupIndex)
        {
        case BUTTON_INVENTORY:

        {
            const float panelW = 520.0f;
            const float panelH = 680.0f;
            const float panelX = 0.0f;
            const float panelY = 0.0f;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxTextureSet(inventoryBG, 0, 0);

            AEMtx33Scale(&scale, panelW, panelH);
            AEMtx33Trans(&trans, panelX, panelY);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);


            // --- Fruits / Seeds icons ---
            {
                const float leftEdge = panelX - INV_PANEL_W * 0.5f;
                const float topEdge = panelY + INV_PANEL_H * 0.5f;

                const float slotW = INV_FRUIT_W;
                const float slotH = INV_FRUIT_H;

                const float fruitCx = leftEdge + INV_TAB_LEFT_PAD + slotW * 0.5f;
                const float fruitCy = topEdge - INV_TAB_TOP_PAD - slotH * 0.5f;

                const float seedCx = fruitCx + INV_TAB_SPACING_X;
                const float seedCy = fruitCy;

                const bool fruitActive = (gActiveInvTab == TAB_FRUITS);
                const bool seedActive = (gActiveInvTab == TAB_SEEDS);

                // ----- FRUIT -----
                {
                    const float w = fruitActive ? INV_FRUIT_HL_W : INV_FRUIT_W;
                    const float h = fruitActive ? INV_FRUIT_HL_H : INV_FRUIT_H;

                    AEGfxTextureSet(fruitActive ? invFruitHighlight : invFruit, 0, 0);
                    AEMtx33Scale(&scale, w, h);
                    AEMtx33Trans(&trans, fruitCx, fruitCy);
                    AEMtx33Concat(&transform, &trans, &scale);
                    AEGfxSetTransform(transform.m);
                    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
                }

                // ----- SEED -----
                {
                    const float w = seedActive ? INV_SEED_HL_W : INV_SEED_W;
                    const float h = seedActive ? INV_SEED_HL_H : INV_SEED_H;

                    AEGfxTextureSet(seedActive ? invSeedHighlight : invSeed, 0, 0);
                    AEMtx33Scale(&scale, w, h);
                    AEMtx33Trans(&trans, seedCx, seedCy);
                    AEMtx33Concat(&transform, &trans, &scale);
                    AEGfxSetTransform(transform.m);
                    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
                }
            }

            // --- Draw capacity text ---
            const int invcur = (gActiveInvTab == TAB_FRUITS) ? GetFruitCount() : GetSeedCount();
            const int invmax = GetInventoryLimit();

            char capText[16];
            sprintf_s(capText, "%d/%d", invcur, invmax);

            float textX = (panelX + panelW * 0.24f) / 800.0f;
            float textY = (panelY + panelH * 0.41f) / 450.0f;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(0, 0, 0, 1);
            AEGfxPrint(fontId, capText, textX, textY, 0.9f, 0, 0, 0, 1);

            // --- Slider ---
            const float panelCenterX = 0.0f, panelCenterY = 0.0f;
            const float leftEdge = panelCenterX - INV_PANEL_W * 0.5f;
            const float topEdge = panelCenterY + INV_PANEL_H * 0.5f;

            const float baseX0 = leftEdge + INV_SLD_LEFT_PAD;
            const float baseX1 = baseX0 + INV_SLD_TRACK_W;

            const float trackX0 = baseX0 + INV_SLD_TRACK_CAP_INSET;
            const float trackX1 = baseX1 - INV_SLD_TRACK_CAP_INSET;

            const float trackY = (topEdge - INV_SLD_TOP_PAD) + INV_SLD_FILL_Y_OFFSET;

            const int curCount = (gActiveInvTab == TAB_FRUITS)
                ? GetFruitCount()
                : GetSeedCount();
            const int minVal = (curCount > 0) ? 1 : 0;
            const int maxVal = curCount;

            gInvSliderValue = (std::max)(minVal, (std::min)(maxVal, gInvSliderValue));

            const float t = (maxVal > minVal)
                ? (float)(gInvSliderValue - minVal) / (float)(maxVal - minVal)
                : 0.0f;
            const float knobX = trackX0 + (trackX1 - trackX0) * t;
            const float knobY = trackY;
            const float stripeH = INV_SLD_FILL_THICKNESS;
            const float fillW = (trackX1 - trackX0) * t;
            const float pngCenterY = knobY + INV_SLD_KNOB_ANCHOR_Y;

            if (fillW > 0.5f) {
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                if (invSliderFill) {
                    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                    AEGfxSetColorToMultiply(1.00f, 0.95f, 0.80f, 1.0f);
                    AEGfxTextureSet(invSliderFill, 0, 0);
                }
                else {
                    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                    AEGfxSetColorToMultiply(1.0f, 0.92f, 0.6f, 1.0f);
                }
                AEMtx33Scale(&scale, fillW, stripeH);
                AEMtx33Trans(&trans, trackX0 + 0.5f * fillW, trackY);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }

            if (invSliderKnob)
            {
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1, 1, 1, 1);
                AEGfxTextureSet(invSliderKnob, 0, 0);

                AEMtx33Scale(&scale, INV_SLD_KNOB_W, INV_SLD_KNOB_H);
                AEMtx33Trans(&trans, knobX, pngCenterY);
                AEMtx33Concat(&transform, &trans, &scale);

                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }

            char lb[8], rb[8];
            sprintf_s(lb, "%d", minVal);
            sprintf_s(rb, "%d", maxVal);
            const float halfW = 800.0f, halfH = 450.0f;

            const float leftLabelX = (trackX0 + INV_SLD_LABEL_LEFT_XPAD) / halfW;
            const float leftLabelY = (trackY + INV_SLD_LABEL_Y_OFFSET) / halfH;
            const float rightLabelX = (trackX1 - INV_SLD_LABEL_RIGHT_XPAD) / halfW;
            const float rightLabelY = leftLabelY;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(0, 0, 0, 1);
            AEGfxPrint(fontId, lb, leftLabelX, leftLabelY, 0.8f, 0, 0, 0, 1);
            AEGfxPrint(fontId, rb, rightLabelX, rightLabelY, 0.8f, 0, 0, 0, 1);
            {
                char valTxt[8];
                sprintf_s(valTxt, "%d", gInvSliderValue);

                const float vhalfW = 800.0f;
                const float vhalfH = 450.0f;

                const float textWorldX = knobX + INV_SLD_TEXT_X_OFFSET;
                const float textWorldY = knobY + INV_SLD_KNOB_ANCHOR_Y + INV_SLD_TEXT_FROM_PNG_CENTER_Y;

                float charWidth = 0.012f;
                int len = static_cast<int>(strlen(valTxt));
                float adjX = textWorldX / vhalfW - charWidth * (len - 1) * 0.5f;
                float vtextY = textWorldY / vhalfH;

                AEGfxSetColorToMultiply(0, 0, 0, 1);
                AEGfxPrint(fontId, valTxt, adjX, vtextY, 0.8f, 0, 0, 0, 1);
            }

            // ================= Inventory items =================
            {

                const float itemX = panelX + INV_ITEM_X;
                const float itemY = panelY + INV_ITEM_Y;

                const float iconSize = 90.0f;

                if (gActiveInvTab == TAB_FRUITS)
                {
                    if (GetFruitCount() > 0)
                    {

                        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                        AEGfxSetColorToMultiply(1, 1, 1, 1);
                        AEGfxTextureSet(appleIcon, 0, 0);

                        AEMtx33Scale(&scale, iconSize, iconSize);
                        AEMtx33Trans(&trans, itemX, itemY);
                        AEMtx33Concat(&transform, &trans, &scale);
                        AEGfxSetTransform(transform.m);
                        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                        if (gSelectedInvItem == INV_ITEM_APPLE)
                        {
                            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                            AEGfxSetColorToMultiply(1.0f, 0.75f, 0.2f, 0.4f);

                            AEMtx33Scale(&scale, iconSize + 10.0f, iconSize + 10.0f);
                            AEMtx33Trans(&trans, itemX, itemY);
                            AEMtx33Concat(&transform, &trans, &scale);
                            AEGfxSetTransform(transform.m);
                            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                            AEGfxSetColorToMultiply(1, 1, 1, 1);
                            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                        }

                        char cnt[8];
                        sprintf_s(cnt, "%d", GetFruitCount());
                        AEGfxPrint(fontId, cnt, (itemX + 40) / 800.0f, (itemY - 40) / 450.0f, 0.7f, 0, 0, 0, 1);
                    }
                }
                else // SEEDS
                {
                    if (Inventory_GetSeedCount(SEED_APPLE) > 0)
                    {
                        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                        AEGfxSetColorToMultiply(1, 1, 1, 1);
                        AEGfxTextureSet(appleSeedIcon, 0, 0);

                        AEMtx33Scale(&scale, iconSize, iconSize);
                        AEMtx33Trans(&trans, itemX, itemY);
                        AEMtx33Concat(&transform, &trans, &scale);
                        AEGfxSetTransform(transform.m);
                        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                        if (gSelectedInvItem == INV_ITEM_APPLE_SEED)
                        {
                            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                            AEGfxSetColorToMultiply(0.3f, 1.0f, 0.4f, 0.4f);

                            AEMtx33Scale(&scale, iconSize + 10.0f, iconSize + 10.0f);
                            AEMtx33Trans(&trans, itemX, itemY);
                            AEMtx33Concat(&transform, &trans, &scale);
                            AEGfxSetTransform(transform.m);
                            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                            AEGfxSetColorToMultiply(1, 1, 1, 1);
                            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                        }

                        char cnt[8];
                        sprintf_s(cnt, "%d", Inventory_GetSeedCount(SEED_APPLE));
                        AEGfxPrint(fontId, cnt, (itemX + 20) / 800.0f, (itemY - 35) / 450.0f, 0.7f, 1, 1, 1, 1);

                    }
                }
            }

            // ================= Trash button =================
            if (gSelectedInvItem != INV_ITEM_NONE)
            {
                float tx = panelX + INV_TRASH_X;
                float ty = panelY + INV_TRASH_Y;

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1, 1, 1, 1);

                AEGfxTextureSet(trashIcon, 0, 0);
                AEMtx33Scale(&scale, INV_TRASH_SIZE, INV_TRASH_SIZE);
                AEMtx33Trans(&trans, tx, ty);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }

            // Confirmation popup
            if (gInvConfirmOpen)
            {
                AEGfxTextureSet(confirmBG, 0, 0);
                AEMtx33Scale(&scale, 300, 180);
                AEMtx33Trans(&trans, 0, 0);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                AEGfxSetColorToMultiply(
                    1.0f,
                    gHoverYes ? 0.9f : 1.0f,
                    gHoverYes ? 0.9f : 1.0f,
                    1.0f
                );

                AEGfxTextureSet(confirmYes, 0, 0);
                AEMtx33Scale(&scale, 80, 40);
                AEMtx33Trans(&trans, -60, -40);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                AEGfxSetColorToMultiply(
                    1.0f,
                    gHoverNo ? 0.9f : 1.0f,
                    gHoverNo ? 0.9f : 1.0f,
                    1.0f
                );

                AEGfxTextureSet(confirmNo, 0, 0);
                AEMtx33Scale(&scale, 80, 40);
                AEMtx33Trans(&trans, 60, -40);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }


            break;
        }

        case BUTTON_COLLECTION:
        {
            const float panelW = 800.0f;
            const float panelH = 600.0f;
            const float panelX = 180.0f;
            const float panelY = 0.0f;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxTextureSet(collectionBG, 0, 0);

            AEMtx33Scale(&scale, panelW, panelH);
            AEMtx33Trans(&trans, panelX, panelY);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            break;
        }
        case BUTTON_SETTINGS:
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxTextureSet(settingsBG, 0, 0);

            AEMtx33Scale(&scale, SET_PANEL_W * ScaleX, SET_PANEL_H * ScaleY);
            AEMtx33Trans(&trans, SET_PANEL_X, SET_PANEL_Y);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            // SOUND toggle
            AEGfxSetColorToMultiply(1, gHoverSoundToggle ? 0.85f : 1, gHoverSoundToggle ? 0.85f : 1, 1);
            AEGfxTextureSet(gSoundEnabled ? settingsOn : settingsOff, 0, 0);

            AEMtx33Scale(&scale, SET_TOGGLE_W * ScaleX, SET_TOGGLE_H * ScaleY);
            AEMtx33Trans(&trans, SET_TOGGLE_X + SET_PANEL_X, SET_SOUND_Y + SET_PANEL_Y + 45.0f);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            // MUSIC toggle
            AEGfxSetColorToMultiply(1, gHoverMusicToggle ? 0.85f : 1, gHoverMusicToggle ? 0.85f : 1, 1);
            AEGfxTextureSet(gMusicEnabled ? settingsOn : settingsOff, 0, 0);

            AEMtx33Scale(&scale, SET_TOGGLE_W * ScaleX, SET_TOGGLE_H * ScaleY);
            AEMtx33Trans(&trans, SET_TOGGLE_X + SET_PANEL_X, SET_MUSIC_Y + SET_PANEL_Y + 5.0f);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            break;
        }
        }
    }


    if (popupOpen && AEInputCheckTriggered(AEVK_Q))
    {
        popupOpen = false;
    }

    // --- Upgrades Panel ---
    float upgradesPanelW = UPGRADES_PANEL_W;
    float upgradesPanelH = UPGRADES_PANEL_H;
    float upgradesPanelX = UPGRADES_PANEL_X;
    float upgradesPanelY = UPGRADES_PANEL_Y;
    AEMtx33Scale(&scale, upgradesPanelW, upgradesPanelH);
    AEMtx33Trans(&trans, upgradesPanelX, upgradesPanelY);
    AEMtx33Concat(&transform, &trans, &scale);

    float upgStartY = upgradesPanelY + 60.0f;
    float upgSpacing = 70.0f;

    int visibleSlot = 0;
    auto& upgradesList = Upgrades_GetList();

    for (size_t i = upgradesStartIndex;
        i < upgradesList.size() && visibleSlot < MAX_VISIBLE_UPGRADES;
        ++i)
    {
        if (upgradesList[i].purchased)
            continue;

        if (!upgradesList[i].texture) {
            printf("Upgrade %d missing texture!\n", static_cast<int>(i));
        }

        float rowY = upgStartY - visibleSlot * upgSpacing;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(upgradesList[i].texture, 0, 0);

        AEMtx33Scale(&scale, 369 * ScaleX, 70 * ScaleY);
        AEMtx33Trans(&trans, upgradesPanelX, rowY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        visibleSlot++;
    }

    // --- Seeds Panel ---
    if (seedsPopupOpen)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxSetTransparency(1.0f);

        float seedspanelX = -70.0f;
        float seedspanelY = 20.0f;
        float seedY = seedspanelY + 115.0f;

        AEGfxTextureSet(seedsTexture, 0, 0);
        AEMtx33Scale(&scale, 400, 550);
        AEMtx33Trans(&trans, seedspanelX, seedspanelY);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        // Left arrow
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxTextureSet(leftArrowTexture, 0, 0);
        AEMtx33Scale(&scale, 40, 40);
        AEMtx33Trans(&trans, seedspanelX - 150.0f, seedspanelY + 150.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        // Right arrow
        AEGfxTextureSet(rightArrowTexture, 0, 0);
        AEMtx33Scale(&scale, 40, 40);
        AEMtx33Trans(&trans, seedspanelX + 150.0f, seedspanelY + 150.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        if (currentSeedIndex == SEED_APPLE)
        {
            if (hoveredSeed == SEED_APPLE)
            {
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 0.55f, 0.0f, 0.9f);

                AEMtx33Scale(&scale, 112, 112);
                AEMtx33Trans(&trans, seedspanelX, seedY);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetColorToMultiply(1, 1, 1, 1);
            }

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxTextureSet(appleSeedIcon, 0, 0);

            AEMtx33Scale(&scale, 100, 100);
            AEMtx33Trans(&trans, seedspanelX, seedY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            int seedCount = Inventory_GetSeedCount(SEED_APPLE);
            char seedCountText[8];
            sprintf_s(seedCountText, "%d", seedCount);

            float badgeX = seedspanelX + 35.0f;
            float badgeY = seedY - 30.0f;
            float textBadgeX = badgeX - ((seedCount < 10) ? 4.0f : 8.0f);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxPrint(fontId, seedCountText, textBadgeX / 800.0f, badgeY / 450.0f, 0.7f, 1, 1, 1, 1);

            const float panelLeft = seedspanelX - 200.0f;
            const float panelTop = seedspanelY + 275.0f;
            const float scaleTexX = 400.0f / 616.0f;
            const float scaleTexY = 550.0f / 662.0f;

            const float coinCx = panelLeft + 118.0f * scaleTexX;
            const float coinCy = panelTop - 462.0f * scaleTexY;
            const float waterCx = panelLeft + 398.0f * scaleTexX;
            const float waterCy = coinCy;
            const float clockCx = coinCx;
            const float clockCy = panelTop - 525.0f * scaleTexY;

            const float tOff = 21.0f;
            const float tr = 0.25f, tg = 0.13f, tb = 0.02f;

            const float brownBoxCx = panelLeft + 308.0f * scaleTexX;
            const float brownBoxCy = panelTop - 385.0f * scaleTexY;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);

            SeedData& data = seedDatabase[SEED_APPLE];

            char costText[32];
            sprintf_s(costText, "%d / min", data.cost);

            char waterText[32];
            sprintf_s(waterText, "%d", data.waterNeeded);

            char growText[32];
            sprintf_s(growText, "%.0f mins", data.growTime);

            AEGfxPrint(fontId, data.name,
                (brownBoxCx - 80.0f) / 800.0f, brownBoxCy / 450.0f,
                1.1f, tr, tg, tb, 1);

            AEGfxPrint(fontId, costText,
                (coinCx + tOff) / 800.0f, coinCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            AEGfxPrint(fontId, waterText,
                (waterCx + tOff) / 800.0f, waterCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            AEGfxPrint(fontId, growText,
                (clockCx + tOff) / 800.0f, clockCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            const float descBoxTop = panelTop - 585.0f * scaleTexY + 10.0f;
            const float descScale = 0.5f;
            const float lineH = 13.0f;
            const float descX = (panelLeft + 68.0f) / 800.0f;

            AEGfxPrint(fontId, "The apple seed -- simple, but my favorite.",
                descX, (descBoxTop - lineH * 0.5f) / 450.0f, descScale, tr, tg, tb, 1);
            AEGfxPrint(fontId, "Used to give as a kid: 'Plant it, watch it grow.'",
                descX, (descBoxTop - lineH * 1.5f) / 450.0f, descScale, tr, tg, tb, 1);
            AEGfxPrint(fontId, "Funny how the smallest things become the best.",
                descX, (descBoxTop - lineH * 2.5f) / 450.0f, descScale, tr, tg, tb, 1);
        }
        else if (currentSeedIndex == 1)
        {
            if (hoveredSeed == 1)
            {
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(0.4f, 0.8f, 0.2f, 0.9f);

                AEMtx33Scale(&scale, 112, 112);
                AEMtx33Trans(&trans, seedspanelX, seedY);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetColorToMultiply(1, 1, 1, 1);
            }

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxTextureSet(pearSeedIcon, 0, 0);

            AEMtx33Scale(&scale, 100, 100);
            AEMtx33Trans(&trans, seedspanelX, seedY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            int pearSeedCount = Inventory_GetSeedCount(SEED_PEAR);
            char pearSeedCountText[8];
            sprintf_s(pearSeedCountText, "%d", pearSeedCount);

            float badgeX = seedspanelX + 35.0f;
            float badgeY = seedY - 30.0f;
            float textBadgeX = badgeX - ((pearSeedCount < 10) ? 4.0f : 8.0f);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxPrint(fontId, pearSeedCountText, textBadgeX / 800.0f, badgeY / 450.0f, 0.7f, 1, 1, 1, 1);

            const float panelLeft = seedspanelX - 200.0f;
            const float panelTop = seedspanelY + 275.0f;
            const float scaleTexX = 400.0f / 616.0f;
            const float scaleTexY = 550.0f / 662.0f;

            const float coinCx = panelLeft + 118.0f * scaleTexX;
            const float coinCy = panelTop - 462.0f * scaleTexY;
            const float waterCx = panelLeft + 398.0f * scaleTexX;
            const float waterCy = coinCy;
            const float clockCx = coinCx;
            const float clockCy = panelTop - 525.0f * scaleTexY;
            const float tOff = 21.0f;
            const float tr = 0.25f, tg = 0.13f, tb = 0.02f;
            const float brownBoxCx = panelLeft + 308.0f * scaleTexX;
            const float brownBoxCy = panelTop - 385.0f * scaleTexY;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);

            SeedData& data = seedDatabase[SEED_PEAR];

            char costText[32];
            sprintf_s(costText, "%d / min", data.cost);

            char waterText[32];
            sprintf_s(waterText, "%d", data.waterNeeded);

            char growText[32];
            sprintf_s(growText, "%.0f mins", data.growTime);

            AEGfxPrint(fontId, data.name,
                (brownBoxCx - 80.0f) / 800.0f, brownBoxCy / 450.0f,
                1.1f, tr, tg, tb, 1);

            AEGfxPrint(fontId, costText,
                (coinCx + tOff) / 800.0f, coinCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            AEGfxPrint(fontId, waterText,
                (waterCx + tOff) / 800.0f, waterCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            AEGfxPrint(fontId, growText,
                (clockCx + tOff) / 800.0f, clockCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            const float descBoxTop = panelTop - 585.0f * scaleTexY + 10.0f;
            const float descScale = 0.5f;
            const float lineH = 13.0f;
            const float descX = (panelLeft + 68.0f) / 800.0f;

            AEGfxPrint(fontId, "The pear seed -- takes patience, but worth it.",
                descX, (descBoxTop - lineH * 0.5f) / 450.0f, descScale, tr, tg, tb, 1);
            AEGfxPrint(fontId, "Soft, sweet fruit. Grandma's favorite.",
                descX, (descBoxTop - lineH * 1.5f) / 450.0f, descScale, tr, tg, tb, 1);
            AEGfxPrint(fontId, "Give it time and it will never disappoint.",
                descX, (descBoxTop - lineH * 2.5f) / 450.0f, descScale, tr, tg, tb, 1);
        }
    }

    //Plot Slots
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxTextureSet(plotSlotTexture, 0, 0);

    for (size_t i = 0; i < plotSlots.size(); i++)
    {
        PlotSlot& slot = plotSlots[i];

        AEMtx33Scale(&scale, slot.width, slot.height);
        AEMtx33Trans(&trans, slot.x, slot.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        if ((int)i == hoveredPlotIndex)
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(0.2f, 0.8f, 0.3f, 0.55f);

            AEMtx33Scale(&scale, slot.width, slot.height);
            AEMtx33Trans(&trans, slot.x, slot.y);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
        }
    }

}

bool UI_IsMenuOpen()
{
    return menuOpen;
}

// ============================
// Crate hover + tints
// ============================

static void UI_ResetCrateConfigToDefaults()
{
    gCrateCfg.bins[0] = { 0.285f, 0.740f, 0.200f, 0.135f };
    gCrateCfg.bins[1] = { 0.475f, 0.740f, 0.170f, 0.135f };
    gCrateCfg.bins[2] = { 0.660f, 0.740f, 0.200f, 0.135f };

    gCrateCfg.tipWidth = 200.0f;
    gCrateCfg.tipHeight = 100.0f;
    gCrateCfg.tipMargin = 24.0f;
    gCrateCfg.tipCenterOnCrate = true;
}

static inline void UI_EnsureCrateCfg()
{
    if (!gCrateCfgInitialized) {
        UI_ResetCrateConfigToDefaults();
        gCrateCfgInitialized = true;
    }
}

const CrateLayoutConfig& UI_GetCrateLayoutConfig()
{
    UI_EnsureCrateCfg();
    return gCrateCfg;
}

void UI_SetCrateLayoutConfig(const CrateLayoutConfig& cfg)
{
    gCrateCfg = cfg;
    gCrateCfgInitialized = true;
}

void UI_RebuildCrateHitboxesFromStall(float stallX, float stallY, float stallW, float stallH)
{
    UI_EnsureCrateCfg();

    gFruitBaskets.clear();


    auto uvToWorldRect = [stallX, stallY, stallW, stallH](float uC, float vC, float uW, float vH) -> FruitBasket
        {
            FruitBasket b{};
            const float xLocal = (uC - 0.5f) * stallW;
            const float yLocal = (0.5f - vC) * stallH;
            b.x = stallX + xLocal;
            b.y = stallY + yLocal;
            b.width = uW * stallW;
            b.height = vH * stallH;
            return b;
        };


    for (int i = 0; i < 3; ++i) {
        const auto& r = gCrateCfg.bins[i];

        FruitBasket b = uvToWorldRect(r.uCenter, r.vCenter, r.uWidth, r.vHeight);

        b.fruitType = (fruitType)i;
        b.stock = 0;

        gFruitBaskets.push_back(b);
    }
}


static inline void GetWorldMouse(float& worldX, float& worldY)
{
    int mx, my;
    AEInputGetCursorPosition(&mx, &my);

    const float halfW = 1600.0f * 0.5f;
    const float halfH = 900.0f * 0.5f;

    worldX = static_cast<float>(mx) - halfW;
    worldY = halfH - static_cast<float>(my);
}

static bool IsMouseOverBasket(const FruitBasket& basket)
{
    float worldX, worldY;
    GetWorldMouse(worldX, worldY);

    return worldX >= basket.x - basket.width * 0.5f &&
        worldX <= basket.x + basket.width * 0.5f &&
        worldY >= basket.y - basket.height * 0.5f &&
        worldY <= basket.y + basket.height * 0.5f;
}


static inline float clampf(float v, float lo, float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static void DrawTooltipClampedAt(float cx, float cy, const char* text,
    float w, float h)
{
    const float halfW = 1600.0f * 0.5f;
    const float halfH = 900.0f * 0.5f;

    const float minX = -halfW + w * 0.5f + 6.0f;
    const float maxX = halfW - w * 0.5f - 6.0f;
    const float minY = -halfH + h * 0.5f + 6.0f;
    const float maxY = halfH - h * 0.5f - 6.0f;

    const float x = clampf(cx, minX, maxX);
    const float y = clampf(cy, minY, maxY);

    AEMtx33 scale, trans, transform;
    AEMtx33Scale(&scale, w, h);
    AEMtx33Trans(&trans, x, y);
    AEMtx33Concat(&transform, &trans, &scale);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(0.10f, 0.10f, 0.10f, 0.85f);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

    const float xText = (x - w * 0.45f) / halfW;
    const float yText = (y + h * 0.25f) / halfH;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxPrint(fontId, text, xText, yText, 1.0f, 1, 1, 1, 1);
}

void UI_DrawFruitBasketTooltips()
{
    // Don't show crate tooltips while transfer panel is open
    if (gTransferPanelOpen) return;

    UI_EnsureCrateCfg();
    const auto& C = UI_GetCrateLayoutConfig();
    const auto& baskets = GetFruitBaskets();

    int mx, my;
    AEInputGetCursorPosition(&mx, &my);
    const float halfW = 1600.0f * 0.5f;
    const float halfH = 900.0f * 0.5f;
    const float worldX = (float)mx - halfW;
    const float worldY = halfH - (float)my;

    for (const auto& b : baskets)
    {
        const bool isHover =
            worldX >= b.x - b.width * 0.5f &&
            worldX <= b.x + b.width * 0.5f &&
            worldY >= b.y - b.height * 0.5f &&
            worldY <= b.y + b.height * 0.5f;

        if (!isHover) continue;

        float tipX = C.tipCenterOnCrate ? b.x : (b.x - b.width * 0.5f + C.tipWidth * 0.5f);
        float tipY = (b.y - b.height * 0.5f) - C.tipMargin - (C.tipHeight * 0.5f);

        if (tipY - C.tipHeight * 0.5f < -halfH + 6.0f) {
            tipY = (b.y + b.height * 0.5f) + C.tipMargin + (C.tipHeight * 0.5f);
        }

        // Make the tooltip a bit taller to fit the extra "Click to Transfer" hint
        float extTipH = C.tipHeight + 22.0f;
        DrawTooltipClampedAt(tipX, tipY, "", C.tipWidth, extTipH);

        const char* fruitName = "Unknown";
        char stockBuf[32];
        char invBuf[32];
        char hintBuf[32] = "Click to Transfer";

        int liveStock = Crate_GetFruitCount(b.fruitType);

        switch (b.fruitType)
        {
        case FRUIT_APPLE:
            fruitName = "Apple";
            snprintf(stockBuf, sizeof(stockBuf), "Stock: %d", liveStock);
            snprintf(invBuf, sizeof(invBuf), "Inventory: %d", GetAppleCount());
            break;

        case FRUIT_PEAR:
            fruitName = "Pear";
            snprintf(stockBuf, sizeof(stockBuf), "Stock: %d", liveStock);
            snprintf(invBuf, sizeof(invBuf), "Inventory: %d", GetPearCount());
            break;

        case FRUIT_BANANA:
            fruitName = "Banana";
            snprintf(stockBuf, sizeof(stockBuf), "Stock: %d", liveStock);
            snprintf(invBuf, sizeof(invBuf), "Inventory: %d", GetBananaCount());
            break;
        }

        const float textStartY = tipY + extTipH * 0.35f;
        const float lineSpacing = 22.0f;
        const float xTextNorm = (tipX - C.tipWidth * 0.45f) / halfW;

        AEGfxPrint(fontId, fruitName, xTextNorm, textStartY / halfH, 1.0f, 1, 1, 1, 1);
        AEGfxPrint(fontId, stockBuf, xTextNorm, (textStartY - lineSpacing) / halfH, 1.0f, 1, 1, 1, 1);
        AEGfxPrint(fontId, invBuf, xTextNorm, (textStartY - 2.0f * lineSpacing) / halfH, 1.0f, 1, 1, 1, 1);
        AEGfxPrint(fontId, hintBuf, xTextNorm, (textStartY - 3.0f * lineSpacing) / halfH, 0.75f, 1.0f, 0.9f, 0.3f, 1);

        break;
    }
}

void UI_DrawPlotTooltips()
{
    int mx, my;
    AEInputGetCursorPosition(&mx, &my);

    const float halfW = 1600.0f * 0.5f;
    const float halfH = 900.0f * 0.5f;
    const float worldX = (float)mx - halfW;
    const float worldY = halfH - (float)my;

    extern std::vector<FarmPlot> farmPlots;

    for (size_t i = 0; i < plotSlots.size(); i++)
    {
        const PlotSlot& slot = plotSlots[i];

        if (Farm_IsPlotLocked(static_cast<int>(i)))
            continue;

        if (!Farm_IsPlotPlanted(static_cast<int>(i)))
            continue;

        bool isOver = worldX >= slot.x - slot.width * 0.5f &&
            worldX <= slot.x + slot.width * 0.5f &&
            worldY >= slot.y - slot.height * 0.5f &&
            worldY <= slot.y + slot.height * 0.5f;

        if (isOver)
        {
            const FarmPlot& plot = farmPlots[i];
            float growTime = Farm_GetGrowTime();

            char line1[64] = "";
            char line2[64] = "";
            int lineCount = 1;

            if (plot.isReady)
            {
                sprintf_s(line1, "Ready to harvest!");
                sprintf_s(line2, "Press SPACE");
                lineCount = 2;

                float tipX = slot.x;
                float tipY = slot.y + slot.height * 0.7f;

                const float tooltipW = 200.0f;
                const float tooltipH = (lineCount == 2) ? 70.0f : 40.0f;

                AEMtx33 scale, trans, transform;
                AEMtx33Scale(&scale, tooltipW, tooltipH);
                AEMtx33Trans(&trans, tipX, tipY);
                AEMtx33Concat(&transform, &trans, &scale);

                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 0.85f);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 0.5f, 1.0f);

                float startY = tipY + (lineCount == 2 ? 10.0f : 0.0f);

                float nx = (tipX - tooltipW * 0.4f) / halfW;
                float ny = (startY + 8.0f) / halfH;
                AEGfxPrint(fontId, line1, nx, ny, 0.65f, 1.0f, 1.0f, 0.5f, 0.9f);

                if (lineCount == 2)
                {
                    ny = (startY - 18.0f) / halfH;
                    AEGfxPrint(fontId, line2, nx, ny, 0.65f, 1.0f, 1.0f, 0.5f, 0.9f);
                }
            }
            else if (plot.isPlanted && !plot.growthFrozen)
            {
                float remaining = growTime - plot.growTimer;
                if (remaining < 0.0f) remaining = 0.0f;

                sprintf_s(line1, "Growing...");

                if (remaining >= 60.0f)
                {
                    int minutes = (int)(remaining / 60.0f);
                    int seconds = (int)remaining % 60;
                    sprintf_s(line2, "%d:%02d remaining", minutes, seconds);
                }
                else
                {
                    sprintf_s(line2, "%.0f sec remaining", remaining);
                }
                lineCount = 2;

                float tipX = slot.x;
                float tipY = slot.y + slot.height * 0.7f;

                const float tooltipW = 200.0f;
                const float tooltipH = 70.0f;

                AEMtx33 scale, trans, transform;
                AEMtx33Scale(&scale, tooltipW, tooltipH);
                AEMtx33Trans(&trans, tipX, tipY);
                AEMtx33Concat(&transform, &trans, &scale);

                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 0.85f);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(0.8f, 0.9f, 1.0f, 1.0f);

                float startY = tipY + 10.0f;

                float nx = (tipX - tooltipW * 0.4f) / halfW;
                float ny = (startY + 8.0f) / halfH;
                AEGfxPrint(fontId, line1, nx, ny, 0.65f, 0.8f, 0.9f, 1.0f, 0.9f);

                ny = (startY - 18.0f) / halfH;
                AEGfxPrint(fontId, line2, nx, ny, 0.65f, 0.8f, 0.9f, 1.0f, 0.9f);
            }
            else if (plot.growthFrozen)
            {
                sprintf_s(line1, "Growth frozen!");
                sprintf_s(line2, "Need rhythm game");
                lineCount = 2;

                float tipX = slot.x;
                float tipY = slot.y + slot.height * 0.7f;

                const float tooltipW = 180.0f;
                const float tooltipH = 70.0f;

                AEMtx33 scale, trans, transform;
                AEMtx33Scale(&scale, tooltipW, tooltipH);
                AEMtx33Trans(&trans, tipX, tipY);
                AEMtx33Concat(&transform, &trans, &scale);

                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 0.85f);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 0.7f, 0.5f, 1.0f);

                float startY = tipY + 10.0f;

                float nx = (tipX - tooltipW * 0.4f) / halfW;
                float ny = (startY + 8.0f) / halfH;
                AEGfxPrint(fontId, line1, nx, ny, 0.65f, 1.0f, 0.7f, 0.5f, 0.9f);

                ny = (startY - 18.0f) / halfH;
                AEGfxPrint(fontId, line2, nx, ny, 0.65f, 1.0f, 0.7f, 0.5f, 0.9f);
            }

            break;
        }
    }
}

void UI_DrawCrateHoverTint_Yellow()
{
    // Don't show hover tint while transfer panel is open
    if (gTransferPanelOpen) return;

    AEMtx33 scale, trans, transform;
    const auto& baskets = GetFruitBaskets();

    for (const auto& b : baskets)
    {
        if (IsMouseOverBasket(b))
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(0.9f, 0.8f, 0.2f, 0.5f);

            AEMtx33Scale(&scale, b.width, b.height);
            AEMtx33Trans(&trans, b.x, b.y);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);

            break;
        }
    }
}

//FARM PLOTS

float UI_GetPlotCenterX()
{
    return plotPlusButton.x;
}

float UI_GetPlotCenterY()
{
    return plotPlusButton.y;
}

float UI_GetPlotSlotX(int index)
{
    if (index < 0 || index >= (int)plotSlots.size())
        return 0.0f;

    return plotSlots[index].x;
}

float UI_GetPlotSlotY(int index)
{
    if (index < 0 || index >= (int)plotSlots.size())
        return 0.0f;

    return plotSlots[index].y;
}

float UI_GetCrateSlotX(int index)
{
    const float crateBaseX = -300.0f;
    const float crateSpacing = 100.0f;
    return crateBaseX + index * crateSpacing;
}

float UI_GetCrateSlotY(int index)
{
    (void)index;
    return -150.0f;
}

void UI_Exit()
{
    AEGfxTextureUnload(menuTexture);
    AEGfxTextureUnload(seedsTexture);

    AEGfxTextureUnload(inventoryIcon);
    AEGfxTextureUnload(inventoryBG);
    AEGfxTextureUnload(invFruit);
    AEGfxTextureUnload(invFruitHighlight);
    AEGfxTextureUnload(invSeed);
    AEGfxTextureUnload(invSeedHighlight);
    AEGfxTextureUnload(invSliderKnob);
    AEGfxTextureUnload(invSliderFill);
    AEGfxTextureUnload(trashIcon);
    AEGfxTextureUnload(confirmBG);
    AEGfxTextureUnload(confirmYes);
    AEGfxTextureUnload(confirmNo);
    AEGfxTextureUnload(appleIcon);

    AEGfxTextureUnload(collectionIcon);
    AEGfxTextureUnload(collectionBG);

    AEGfxTextureUnload(settingsIcon);
    AEGfxTextureUnload(settingsBG);
    AEGfxTextureUnload(settingsOn);
    AEGfxTextureUnload(settingsOff);

    AEGfxTextureUnload(appleSeedIcon);
    AEGfxTextureUnload(pearSeedIcon);
    AEGfxTextureUnload(appleSeedInfo);
    AEGfxTextureUnload(plotSlotTexture);
    AEGfxTextureUnload(leftArrowTexture);
    AEGfxTextureUnload(rightArrowTexture);

    Upgrades_Unload();
}