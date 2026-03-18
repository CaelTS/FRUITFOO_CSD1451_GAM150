#define NOMINMAX

#include "UI.h"
#include "Farm.h"
#include "Economy.h"
#include "Inventory.h"
#include "Crate.h"
#include "AEEngine.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cmath>



extern AEGfxVertexList* g_pMeshFullScreen;
extern s8 fontId;

static bool menuOpen = false;
static bool popupOpen = false;
static bool seedsPopupOpen = false;

static int activePopupIndex = -1;
static int selectedSeed = -1;
static int hoveredSeed = -1;
static int infoSeed = -1;
static int hoveredPlotIndex = -1;
static int activePlotIndex = -1;

float UI_GetPlotCenterX();
float UI_GetPlotCenterY();
struct PlotSlot
{
    float x, y;
    float width, height;
};
static std::vector<PlotSlot> plotSlots;
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

static AEGfxTexture* collectionIcon = nullptr;
static AEGfxTexture* collectionBG = nullptr;

static AEGfxTexture* settingsIcon = nullptr;

static AEGfxTexture* appleSeedIcon = nullptr;
static AEGfxTexture* appleSeedInfo = nullptr;

enum ButtonType
{
    BUTTON_INVENTORY,
    BUTTON_COLLECTION,
    BUTTON_SETTINGS
};

enum SeedType
{
    SEED_APPLE = 0,
    SEED_COUNT = 1
};

struct MenuButton
{
    float x, y;
    float width, height;
    bool isHovered;
    ButtonType type;
};

static std::vector<MenuButton> menuButtons;
static MenuButton plotPlusButton;

extern std::vector<FruitBasket> gFruitBaskets;

static CrateLayoutConfig gCrateCfg{};
static bool gCrateCfgInitialized = false;


// ------------------------
// Upgrades
// ------------------------
struct Upgrade { std::string name; int cost; bool purchased; };
static std::vector<Upgrade> upgrades;
static int upgradesStartIndex = 0;

static const int MAX_VISIBLE_UPGRADES = 3;
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

    collectionIcon = AEGfxTextureLoad("Assets/Collection.png");
    collectionBG = AEGfxTextureLoad("Assets/collectionBG.png");

    settingsIcon = AEGfxTextureLoad("Assets/Settings.png");

    appleSeedIcon = AEGfxTextureLoad("Assets/AppleSeed.png");
    appleSeedInfo = AEGfxTextureLoad("Assets/AppleSeedInfo.png");
    plotSlotTexture = AEGfxTextureLoad("Assets/Plot1.png");

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

    upgrades = { {"Speed Boost", 100, false}, {"Crate Storage", 150, false}, {"Faster Growth", 200, false}, {"Quality Fruits", 300, false}, {"Stall Revamp", 500, false} };
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

        if (button.isHovered && AEInputCheckTriggered(AEVK_LBUTTON))
        {
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
        float panelX = -100.0f;
        float panelY = 0.0f;
        float seedY = panelY + 120.0f;
        float seedW = 100.0f;
        float seedH = 100.0f;

        bool overSeed =
            worldX >= panelX - seedW * 0.5f &&
            worldX <= panelX + seedW * 0.5f &&
            worldY >= seedY - seedH * 0.5f &&
            worldY <= seedY + seedH * 0.5f;

        if (overSeed)
            hoveredSeed = SEED_APPLE;
        else
            hoveredSeed = -1;

        // GUARD: don't plant while farm prompt is visible
        if (overSeed && AEInputCheckTriggered(AEVK_LBUTTON) &&
            !Farm_IsRhythmPaused() && !Farm_IsWaitingForRhythm())
        {
            selectedSeed = SEED_APPLE;

            if (activePlotIndex != -1)
            {
                int plotToPlant = activePlotIndex;

                Farm_PlantSeed(plotToPlant, SEED_APPLE);

                std::cout << "Planted on plot: " << plotToPlant << "\n";

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
        // Skip locked plots - don't interact with them at all
        if (Farm_IsPlotLocked(static_cast<int>(i)))
            continue;

        PlotSlot& slot = plotSlots[i];
        bool isOver =
            worldX >= slot.x - slot.width * 0.5f &&
            worldX <= slot.x + slot.width * 0.5f &&
            worldY >= slot.y - slot.height * 0.5f &&
            worldY <= slot.y + slot.height * 0.5f;

        if (isOver)
        {
            hoveredPlotIndex = static_cast<int>(i);

            // GUARD: don't open seed panel while farm prompt is visible
            if (AEInputCheckTriggered(AEVK_LBUTTON) &&
                !Farm_IsRhythmPaused() && !Farm_IsWaitingForRhythm())
            {
                if (seedsPopupOpen && activePlotIndex == (int)i)
                {
                    seedsPopupOpen = false;
                    selectedSeed = -1;
                    activePlotIndex = -1;
                }
                else
                {
                    seedsPopupOpen = true;
                    activePlotIndex = static_cast<int>(i);
                    selectedSeed = SEED_APPLE;
                }
            }

            break;
        }
    }

    // DELETE SEED BUTTON
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

    // --- Upgrades ---
    const float panelX = UP_BOX_X;
    const float panelY = UP_BOX_Y;
    const float upgradesW = UP_BOX_W;

    const float startYUp = panelY + UP_LIST_TOP_OFFSET;
    const float spacingUp = UP_ROW_SPACING;
    const float boxW = upgradesW - (2.0f * UP_ROW_W_MARGIN);
    const float boxH = UP_ROW_H;

    int shownUp = 0;

    for (size_t i = upgradesStartIndex; i < upgrades.size() && shownUp < MAX_VISIBLE_UPGRADES; ++i)
    {
        auto& u = upgrades[(int)i];
        if (u.purchased) continue;

        const float rowCenterY = startYUp - shownUp * spacingUp;

        const float rowW = boxW;
        const float rowH = boxH;
        const float hoverW = rowW - (UP_HOVER_INSET_L + UP_HOVER_INSET_R);
        const float hoverH = rowH - 2.0f * UP_HOVER_INSET_TB;
        const float hoverX = panelX + 0.5f * (UP_HOVER_INSET_L - UP_HOVER_INSET_R) + UP_HOVER_X_OFFSET;
        const float hoverY = rowCenterY + UP_HOVER_Y_NUDGE;

        const bool over =
            worldX >= hoverX - hoverW * 0.5f && worldX <= hoverX + hoverW * 0.5f &&
            worldY >= hoverY - hoverH * 0.5f && worldY <= hoverY + hoverH * 0.5f;

        if (over && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            u.purchased = true;
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
                gActiveInvTab = TAB_FRUITS;
            else if (PtIn(worldX, worldY, seedCx, seedCy, hw, hh))
                gActiveInvTab = TAB_SEEDS;
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
}


void UI_Draw()
{
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
        float popupW = 400.0f;
        float popupH = 250.0f;
        float popupX = 0.0f;
        float popupY = 0.0f;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 0.95f);

        AEMtx33Scale(&scale, popupW, popupH);
        AEMtx33Trans(&trans, popupX, popupY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1, 1, 1, 1);

        float xText = (popupX - popupW * 0.45f) / 800.0f;
        float yText = (popupY + popupH * 0.25f) / 450.0f;

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

            const int invcur = (gActiveInvTab == TAB_FRUITS) ? GetFruitCount() : GetSeedCount();
            const int invmax = GetInventoryLimit();

            char capText[16];
            sprintf_s(capText, "%d/%d", invcur, invmax);

            float textX = (panelX + panelW * 0.24f) / 800.0f;
            float textY = (panelY + panelH * 0.41f) / 450.0f;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(0, 0, 0, 1);
            AEGfxPrint(fontId, capText, textX, textY, 0.9f, 0, 0, 0, 1);

            const float panelCenterX = 0.0f, panelCenterY = 0.0f;
            const float leftEdge = panelCenterX - INV_PANEL_W * 0.5f;
            const float topEdge = panelCenterY + INV_PANEL_H * 0.5f;

            const float baseX0 = leftEdge + INV_SLD_LEFT_PAD;
            const float baseX1 = baseX0 + INV_SLD_TRACK_W;

            const float trackX0 = baseX0 + INV_SLD_TRACK_CAP_INSET;
            const float trackX1 = baseX1 - INV_SLD_TRACK_CAP_INSET;

            const float trackY = (topEdge - INV_SLD_TOP_PAD) + INV_SLD_FILL_Y_OFFSET;

            const int curCount = (gActiveInvTab == TAB_FRUITS) ? GetFruitCount() : GetSeedCount();
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
            AEGfxPrint(fontId, "Settings", xText, yText, 1.0f, 1, 1, 1, 1);
            AEGfxPrint(fontId, "Game options here.", xText, yText - 0.08f, 0.8f, 1, 1, 1, 1);
            break;
        }
        }
    }

    if (popupOpen && AEInputCheckTriggered(AEVK_Q))
        popupOpen = false;

    // --- Upgrades Panel ---
    const float panelX = UP_BOX_X;
    const float panelY = UP_BOX_Y;
    const float upgradesW = UP_BOX_W;

    const float startYUp = panelY + UP_LIST_TOP_OFFSET;
    const float spacingUp = UP_ROW_SPACING;
    const float boxW = upgradesW - (2.0f * UP_ROW_W_MARGIN);
    const float boxH = UP_ROW_H;

    int mx, my;
    AEInputGetCursorPosition(&mx, &my);

    float worldX = static_cast<float>(mx) - 800.0f;
    float worldY = 450.0f - static_cast<float>(my);

    int visibleSlot = 0;

    for (size_t i = upgradesStartIndex; i < upgrades.size() && visibleSlot < MAX_VISIBLE_UPGRADES; ++i)
    {
        if (upgrades[i].purchased) continue;

        const float rowCenterY = startYUp - visibleSlot * spacingUp;

        const float rowW = boxW;
        const float rowH = boxH;
        const float hoverW = rowW - (UP_HOVER_INSET_L + UP_HOVER_INSET_R);
        const float hoverH = rowH - 2.0f * UP_HOVER_INSET_TB;
        const float hoverX = panelX + 0.5f * (UP_HOVER_INSET_L - UP_HOVER_INSET_R) + UP_HOVER_X_OFFSET;
        const float hoverY = rowCenterY + UP_HOVER_Y_NUDGE;

        const bool isHover =
            worldX >= hoverX - hoverW * 0.5f && worldX <= hoverX + hoverW * 0.5f &&
            worldY >= hoverY - hoverH * 0.5f && worldY <= hoverY + hoverH * 0.5f;

        if (isHover)
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(0.95f, 0.85f, 0.25f, 0.55f);

            AEMtx33Scale(&scale, hoverW, hoverH);
            AEMtx33Trans(&trans, hoverX, hoverY);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
        }

        char buf[128];
        sprintf_s(buf, "%s - %d Gold", upgrades[i].name.c_str(), upgrades[i].cost);

        const float xText = (panelX - (upgradesW * 0.5f) + UP_ROW_W_MARGIN) / 800.0f;
        const float yText = (rowCenterY + boxH * 0.1f) / 450.0f;

        AEGfxPrint(fontId, buf, xText, yText, 0.8f, 0, 0, 0, 1);

        ++visibleSlot;
    }

    // --- Seeds Panel ---
    if (seedsPopupOpen)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxSetTransparency(1.0f);
        float seedspanelX = -100.0f;
        float seedspanelY = 0.0f;

        AEGfxTextureSet(seedsTexture, 0, 0);
        AEMtx33Scale(&scale, 400, 550);
        AEMtx33Trans(&trans, seedspanelX, seedspanelY);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        float seedY = seedspanelY + 120.0f;

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
    }

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

        if (i == hoveredPlotIndex)
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

    // --- Apple Info ---
    if (seedsPopupOpen && selectedSeed == SEED_APPLE)
    {
        float seedsCenterX = -100.0f;
        float seedsCenterY = 0.0f;

        float infoW = 380.0f;
        float infoH = 340.0f;
        float infoX = seedsCenterX;
        float infoY = seedsCenterY - 110.0f;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxTextureSet(appleSeedInfo, 0, 0);

        AEMtx33Scale(&scale, infoW, infoH);
        AEMtx33Trans(&trans, infoX, infoY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
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

static void DrawTooltipClampedAt(float cx, float cy, const char* text, float w, float h)
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

        if (tipY - C.tipHeight * 0.5f < -halfH + 6.0f)
            tipY = (b.y + b.height * 0.5f) + C.tipMargin + (C.tipHeight * 0.5f);

        DrawTooltipClampedAt(tipX, tipY, "", C.tipWidth, C.tipHeight);

        // ---------------------------------------------------------------
        // Live data from Crate and Inventory systems
        // ---------------------------------------------------------------
        const char* fruitName = "Unknown";
        static char stockBuf[32];
        static char inventoryBuf[32];

        int crateIndex = (int)b.fruitType;  // 0=apple, 1=pear, 2=banana
        int crateCount = Crate_GetFruitCount(crateIndex);
        if (crateCount < 0) crateCount = 0; // locked crate returns -1

        int invCount = 0;
        switch (b.fruitType)
        {
        case FRUIT_APPLE:
            fruitName = "Apple";
            invCount = GetAppleCount();
            break;
        case FRUIT_PEAR:
            fruitName = "Pear";
            invCount = GetPearCount();
            break;
        case FRUIT_BANANA:
            fruitName = "Banana";
            invCount = GetBananaCount();
            break;
        }

        sprintf_s(stockBuf, "Crate:     %d", crateCount);
        sprintf_s(inventoryBuf, "Inventory: %d", invCount);

        const char* stockText = stockBuf;
        const char* inventoryText = inventoryBuf;
        // ---------------------------------------------------------------

        const float textStartY = tipY + C.tipHeight * 0.25f;
        const float lineSpacing = 25.0f;
        const float xTextNorm = (tipX - C.tipWidth * 0.45f) / halfW;

        AEGfxPrint(fontId, fruitName, xTextNorm, textStartY / halfH, 1.0f, 1, 1, 1, 1);
        AEGfxPrint(fontId, stockText, xTextNorm, (textStartY - lineSpacing) / halfH, 1.0f, 1, 1, 1, 1);
        AEGfxPrint(fontId, inventoryText, xTextNorm, (textStartY - 2.0f * lineSpacing) / halfH, 1.0f, 1, 1, 1, 1);

        break;
    }
}

void UI_DrawCrateHoverTint_Yellow()
{
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

float UI_GetPlotCenterX() { return plotPlusButton.x; }
float UI_GetPlotCenterY() { return plotPlusButton.y; }

float UI_GetPlotSlotX(int index)
{
    if (index < 0 || index >= (int)plotSlots.size()) return 0.0f;
    return plotSlots[index].x;
}

float UI_GetPlotSlotY(int index)
{
    if (index < 0 || index >= (int)plotSlots.size()) return 0.0f;
    return plotSlots[index].y;
}

float UI_GetCrateSlotX(int index)
{
    const auto& baskets = GetFruitBaskets();
    if (index < 0 || index >= (int)baskets.size()) return 0.0f;
    return baskets[index].x;
}

float UI_GetCrateSlotY(int index)
{
    const auto& baskets = GetFruitBaskets();
    if (index < 0 || index >= (int)baskets.size()) return 0.0f;
    return baskets[index].y;
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
    AEGfxTextureUnload(collectionIcon);
    AEGfxTextureUnload(collectionBG);
    AEGfxTextureUnload(settingsIcon);
    AEGfxTextureUnload(appleSeedIcon);
    AEGfxTextureUnload(appleSeedInfo);
    AEGfxTextureUnload(plotSlotTexture);
}