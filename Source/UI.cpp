#include "UI.h"
#include "Farm.h"
#include "Economy.h"
#include "AEEngine.h"
#include <vector>
#include <iostream>
#include <algorithm>

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

static AEGfxTexture* collectionIcon = nullptr;

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

// Crate bins
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
static const float UP_BOX_X = -480.0f;   // center X of the Upgrades box
static const float UP_BOX_Y = -180.0f;   // center Y of the Upgrades box 
static const float UP_BOX_W = 450.0f;    // width  of the Upgrades box interior
static const float UP_BOX_H = 280.0f;    // height of the Upgrades box interior

// List layout inside upgrade box 
static const float UP_LIST_TOP_OFFSET = +80.0f; // distance from center Y to first row center (positive is up)
static const float UP_ROW_SPACING = 70.0f;  // vertical distance between row centers
static const float UP_ROW_W_MARGIN = 20.0f;  // left/right padding from box edge
static const float UP_ROW_H = 70.0f;  // row height (for hover highlight)

// Upgrades highlight tuning

static const float UP_HOVER_INSET_L = 15.0f;  // px inset from left edge of the row
static const float UP_HOVER_INSET_R = 12.0f;  // px inset from right edge of the row 
static const float UP_HOVER_INSET_TB = 8.0f;   // px inset from top/bottom (bigger = thinner bar)
static const float UP_HOVER_Y_NUDGE = 15.0f;  // px vertical nudge (negative = slightly lower)
static const float UP_HOVER_X_OFFSET = -50.0f;  // px horizontal nudge without moving text (negative = left)

// =========================
// Inventory header icons (under title, left-aligned)
// =========================
enum InvTab { TAB_FRUITS = 0, TAB_SEEDS = 1 };
static InvTab gActiveInvTab = TAB_FRUITS;   // default tab

// Your panel is drawn at center (0,0) with size 520x680 in UI_Draw()
static const float INV_PANEL_W = 520.0f;
static const float INV_PANEL_H = 680.0f;

// ---- POSITION CONTROLS (tweak these to move icons) ----
// Move both icons LEFT/RIGHT  -> change INV_TAB_LEFT_PAD
// Move both icons UP/DOWN     -> change INV_TAB_TOP_PAD   (bigger value moves them LOWER)
// Change gap between icons    -> change INV_TAB_SPACING_X
static const float INV_TAB_LEFT_PAD = 48.0f;   // px from panel LEFT edge to left icon's LEFT edge
static const float INV_TAB_TOP_PAD = 120.0f;  // px from panel TOP edge down to icon's TOP edge
static const float INV_TAB_SPACING_X = 96.0f;   // horizontal gap between Fruit and Seed

// Icon draw size 

static const float INV_FRUIT_W = 64.0f;
static const float INV_FRUIT_H = 64.0f;
static const float INV_FRUIT_HL_W = 72.0f;
static const float INV_FRUIT_HL_H = 72.0f;

static const float INV_SEED_W = 64.0f;
static const float INV_SEED_H = 64.0f;
static const float INV_SEED_HL_W = 72.0f;
static const float INV_SEED_HL_H = 72.0f;


// Highlight artwork correction (if highlight looks slightly shifted): position-only offset
// (Do NOT scale; we keep size the same—just nudge when highlighted.)
static const float INV_HL_OFFSET_X = 0.0f;  // try -2.0f to shift left, +2.0f to shift right
static const float INV_HL_OFFSET_Y = 0.0f;  // try -2.0f up, +2.0f down



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

    collectionIcon = AEGfxTextureLoad("Assets/Collection.png");

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

    plotSlots.clear();

    //plot setup

    float slotSize = 120.0f;
    float spacing = 150.0f;

    int cols = 2;
    int rows = 2;

    // Center of the Plot panel
    float panelCenterX = -510.0f;   // tweak slightly if needed
    float panelCenterY = 150.0f;    // tweak slightly if needed
    float totalWidth = (cols - 1) * spacing + slotSize;
    float totalHeight = (rows - 1) * spacing + slotSize;

    float startX = panelCenterX - totalWidth * 0.5f + slotSize * 0.5f;
    float startY = panelCenterY + totalHeight * 0.5f - slotSize * 0.5f;

    plotSlots.clear();

    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < cols; col++)
        {
            PlotSlot slot = {};
            slot.x = startX + col * spacing;
            slot.y = startY - row * spacing;
            slot.width = slotSize;
            slot.height = slotSize;

            plotSlots.push_back(slot);
        }
    }

    // --- Upgrades ---
    upgrades = { {"Speed Boost", 100, false}, {"Crate Storage", 150, false}, {"Faster Growth", 200, false}, {"Quality Fruits", 300, false}, {"Stall Revamp", 500, false} };


}

void UI_Input()
{
    if (AEInputCheckTriggered(AEVK_M))
        menuOpen = !menuOpen;

    if (menuOpen)
    {
        // Close any open popup with Q - handled here, not in Draw
        if (popupOpen && AEInputCheckTriggered(AEVK_Q))
            popupOpen = false;

        UI_UpdateButtons();
    }
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
            popupOpen = true;
            activePopupIndex = button.type;
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

        // If closing the panel, reset selection
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


        // Hover detection
        if (overSeed)
            hoveredSeed = SEED_APPLE;
        else
            hoveredSeed = -1;
        if (overSeed && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            selectedSeed = SEED_APPLE;

            if (activePlotIndex != -1)
            {
                int plotToPlant = activePlotIndex;

                Farm_PlantSeed(plotToPlant, SEED_APPLE);

                std::cout << "Planted on plot: " << plotToPlant << "\n";

                seedsPopupOpen = false;
                activePlotIndex = -1;
                //selectedSeed = 1;
            }
        }
    }

    // -------------------------------------------------
// PLOT SLOT HOVER
// -------------------------------------------------

    hoveredPlotIndex = -1;

    for (int i = 0; i < plotSlots.size(); i++)
    {
        PlotSlot& slot = plotSlots[i];

        bool isOver =
            worldX >= slot.x - slot.width * 0.5f &&
            worldX <= slot.x + slot.width * 0.5f &&
            worldY >= slot.y - slot.height * 0.5f &&
            worldY <= slot.y + slot.height * 0.5f;

        if (isOver)
        {
            hoveredPlotIndex = i;

            if (AEInputCheckTriggered(AEVK_LBUTTON))
            {
                // Toggle panel if clicking same plot again
                if (seedsPopupOpen && activePlotIndex == i)
                {
                    seedsPopupOpen = false;
                    selectedSeed = -1;      // hide info
                    activePlotIndex = -1;
                }
                else
                {
                    seedsPopupOpen = true;
                    activePlotIndex = i;
                    selectedSeed = SEED_APPLE;   // SHOW INFO IMMEDIATELY
                }
            }

            break;  // stop checking other slots
        }
    }

    // DELETE SEED BUTTON (CLICK LOGIC ONLY)
    for (int i = 0; i < plotSlots.size(); i++)
    {
        if (!Farm_IsPlotPlanted(i))
            continue;

        float xSize = 25.0f;

        float offsetX = -45.0f;   // SAME as Farm_Render
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
            Farm_ClearPlot(i);
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

        // Same rect as draw()
        const float rowW = boxW;
        const float rowH = boxH;
        const float hoverW = rowW - (UP_HOVER_INSET_L + UP_HOVER_INSET_R);
        const float hoverH = rowH - 2.0f * UP_HOVER_INSET_TB;
        const float hoverX = panelX + 0.5f * (UP_HOVER_INSET_L - UP_HOVER_INSET_R) + UP_HOVER_X_OFFSET;

        const float hoverY = rowCenterY + UP_HOVER_Y_NUDGE;

        ;

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
        // Panel is drawn at (0,0) with size INV_PANEL_W x INV_PANEL_H in UI_Draw()
        const float panelX = 0.0f, panelY = 0.0f;
        const float leftEdge = panelX - INV_PANEL_W * 0.5f;
        const float topEdge = panelY + INV_PANEL_H * 0.5f;

        // Use your NORMAL icon size as the "slot" (stable hitbox)
        // If you switched to per-PNG sizes, keep using the normal sizes for hitboxes.
        const float slotW = INV_FRUIT_W;
        const float slotH = INV_FRUIT_H;

        // Compute centers from the same paddings you use in UI_Draw()
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
                gActiveInvTab = TAB_SEEDS; // <- NOTE: assignment, not comparison
            }
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


    float x = -530.0f / 800.0f;  // normalize X by 800.0f
    float y = 350.0f / 450.0f;  // normalize Y by 450.0f

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



        // Draw dark background
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 0.95f);

        AEMtx33Scale(&scale, popupW, popupH);
        AEMtx33Trans(&trans, popupX, popupY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        // Reset render state before printing text
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1, 1, 1, 1);

        // Text position
        float xText = (popupX - popupW * 0.45f) / 800.0f;
        float yText = (popupY + popupH * 0.25f) / 450.0f;

        switch (activePopupIndex)
        {
        case BUTTON_INVENTORY:
        {
            const float panelW = 520.0f;
            const float panelH = 680.0f;
            const float panelX = 0.0f;   // centered
            const float panelY = 0.0f;   // centered

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxTextureSet(inventoryBG, 0, 0);

            AEMtx33Scale(&scale, panelW, panelH);
            AEMtx33Trans(&trans, panelX, panelY);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);


            // --- Fruits / Seeds icons (left-aligned under "Inventory") ---
            {
                AEMtx33 scale, trans, transform;

                const float panelX = 0.0f, panelY = 0.0f;
                const float leftEdge = panelX - INV_PANEL_W * 0.5f;
                const float topEdge = panelY + INV_PANEL_H * 0.5f;

                // Slot (base) size; both icons are drawn at this size
                const float slotW = INV_FRUIT_W;
                const float slotH = INV_FRUIT_H;

                // Centers computed from padding (so it's aligned to the left under the title)
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
                    AEMtx33Trans(&trans, fruitCx, fruitCy); // same center
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


            // --- Draw capacity text "current/limit" ---
            const int invcur = (gActiveInvTab == TAB_FRUITS) ? Economy_GetFruitCount() : Economy_GetSeedCount();// from Economy.cpp
            const int invmax = Economy_GetInventoryLimit();  // from Economy.cpp

            char capText[16];
            sprintf_s(capText, "%d/%d", invcur, invmax);

            // Position inside the small rounded header box on your PNG.
            // Nudge the multipliers a little if it’s off by a few pixels.
            float textX = (panelX + panelW * 0.24f) / 800.0f;
            float textY = (panelY + panelH * 0.41f) / 450.0f;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(0, 0, 0, 1); // dark text to match your PNG style
            AEGfxPrint(fontId, capText, textX, textY, 0.9f, 0, 0, 0, 1);


            break;
        }

        case BUTTON_COLLECTION:
            AEGfxPrint(fontId, "Collection", xText, yText, 1.0f, 1, 1, 1, 1);
            AEGfxPrint(fontId, "Your discovered fruits.",
                xText, yText - 0.08f,
                0.8f, 1, 1, 1, 1);
            break;

        case BUTTON_SETTINGS:
            AEGfxPrint(fontId, "Settings", xText, yText, 1.0f, 1, 1, 1, 1);
            AEGfxPrint(fontId, "Game options here.",
                xText, yText - 0.08f,
                0.8f, 1, 1, 1, 1);
            break;
        }
    }
    // Note: Q to close popup is handled in UI_Input(), not here

    // --- Upgrades Panel ---

    const float panelX = UP_BOX_X;
    const float panelY = UP_BOX_Y;
    const float upgradesW = UP_BOX_W;

    const float startYUp = panelY + UP_LIST_TOP_OFFSET;
    const float spacingUp = UP_ROW_SPACING;
    const float boxW = upgradesW - (2.0f * UP_ROW_W_MARGIN);
    const float boxH = UP_ROW_H;


    // Draw upgrades text & hover highlight
    int mx, my;
    AEInputGetCursorPosition(&mx, &my);

    float worldX = static_cast<float>(mx) - 800.0f;
    float worldY = 450.0f - static_cast<float>(my);



    // --- Upgrades Text & Hover Highlight ---
    int visibleSlot = 0;

    for (size_t i = upgradesStartIndex; i < upgrades.size() && visibleSlot < MAX_VISIBLE_UPGRADES; ++i)
    {
        if (upgrades[i].purchased) continue;

        const float rowCenterY = startYUp - visibleSlot * spacingUp;

        // Final hover rect derived from your boxW/boxH + insets/offsets
        const float rowW = boxW;
        const float rowH = boxH;
        const float hoverW = rowW - (UP_HOVER_INSET_L + UP_HOVER_INSET_R);
        const float hoverH = rowH - 2.0f * UP_HOVER_INSET_TB;
        const float hoverX = panelX + 0.5f * (UP_HOVER_INSET_L - UP_HOVER_INSET_R) + UP_HOVER_X_OFFSET;
        const float hoverY = rowCenterY + UP_HOVER_Y_NUDGE;

        const bool isHover =
            worldX >= hoverX - hoverW * 0.5f && worldX <= hoverX + hoverW * 0.5f &&
            worldY >= hoverY - hoverH * 0.5f && worldY <= hoverY + hoverH * 0.5f;

        // Draw highlight (only if hovered)
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


        // Row text 
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
        float seedsPanelX = -100.0f;
        float seedsPanelY = 0.0f;

        AEGfxTextureSet(seedsTexture, 0, 0);
        AEMtx33Scale(&scale, 400, 550);
        AEMtx33Trans(&trans, seedsPanelX, seedsPanelY);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        float seedY = seedsPanelY + 120.0f;

        // Highlight
        if (hoveredSeed == SEED_APPLE)
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 0.55f, 0.0f, 0.9f);

            AEMtx33Scale(&scale, 112, 112);   // slightly larger than icon
            AEMtx33Trans(&trans, seedsPanelX, seedY);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            // Reset after drawing
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
        }

        // Draw seed
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxTextureSet(appleSeedIcon, 0, 0);

        AEMtx33Scale(&scale, 100, 100);
        AEMtx33Trans(&trans, seedsPanelX, seedY);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
    }

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxTextureSet(plotSlotTexture, 0, 0);

    for (int i = 0; i < plotSlots.size(); i++)
    {
        PlotSlot& slot = plotSlots[i];

        // Draw slot
        AEMtx33Scale(&scale, slot.width, slot.height);
        AEMtx33Trans(&trans, slot.x, slot.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        // Hover overlay
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

        // PERFECT horizontal center
        float infoX = seedsCenterX;

        // Move it lower inside the seeds panel
        float infoY = seedsCenterY - 110.0f;  // adjust this number

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
    // tuned values
    gCrateCfg.bins[0] = { 0.285f, 0.740f, 0.200f, 0.135f }; // Left bin
    gCrateCfg.bins[1] = { 0.475f, 0.740f, 0.170f, 0.135f }; // Middle bin
    gCrateCfg.bins[2] = { 0.660f, 0.740f, 0.200f, 0.135f }; // Right bin

    // Tooltip defaults
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

// ---- builder ( uses config) ----
void UI_RebuildCrateHitboxesFromStall(float stallX, float stallY, float stallW, float stallH)
{
    UI_EnsureCrateCfg();

    gFruitBaskets.clear();


    auto uvToWorldRect = [stallX, stallY, stallW, stallH](float uC, float vC, float uW, float vH) -> FruitBasket
        {
            FruitBasket b{};
            const float xLocal = (uC - 0.5f) * stallW;  // mesh centered
            const float yLocal = (0.5f - vC) * stallH;  // invert V (texture top = 0)
            b.x = stallX + xLocal;
            b.y = stallY + yLocal;
            b.width = uW * stallW;
            b.height = vH * stallH;
            return b;
        };


    for (int i = 0; i < 3; ++i) {
        const auto& r = gCrateCfg.bins[i];

        FruitBasket b = uvToWorldRect(r.uCenter, r.vCenter, r.uWidth, r.vHeight);

        b.fruitType = (fruitType)i;   // 0 Apple, 1 Pear, 2 Banana

        gFruitBaskets.push_back(b);
    }
}


// Convert mouse to world coordinates (window: 1600x900)
static inline void GetWorldMouse(float& worldX, float& worldY)
{
    int mx, my;
    AEInputGetCursorPosition(&mx, &my);

    const float halfW = 1600.0f * 0.5f;
    const float halfH = 900.0f * 0.5f;

    worldX = static_cast<float>(mx) - halfW;
    worldY = halfH - static_cast<float>(my);
}

// AABB test for a basket/crate
static bool IsMouseOverBasket(const FruitBasket& basket)
{
    float worldX, worldY;
    GetWorldMouse(worldX, worldY);

    return worldX >= basket.x - basket.width * 0.5f &&
        worldX <= basket.x + basket.width * 0.5f &&
        worldY >= basket.y - basket.height * 0.5f &&
        worldY <= basket.y + basket.height * 0.5f;
}


// --- Safe clamp ---
static inline float clampf(float v, float lo, float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

// Draw a tooltip clamped to the screen, centered at world (cx, cy)
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
    AEGfxSetColorToMultiply(0.10f, 0.10f, 0.10f, 0.85f); // panel color
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

    // AE text uses coords normalized by half sizes
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

        // Tooltip panel position
        float tipX = C.tipCenterOnCrate ? b.x : (b.x - b.width * 0.5f + C.tipWidth * 0.5f);
        float tipY = (b.y - b.height * 0.5f) - C.tipMargin - (C.tipHeight * 0.5f);

        if (tipY - C.tipHeight * 0.5f < -halfH + 6.0f) {
            tipY = (b.y + b.height * 0.5f) + C.tipMargin + (C.tipHeight * 0.5f);
        }

        // Panel background
        DrawTooltipClampedAt(tipX, tipY, "", C.tipWidth, C.tipHeight);

        // --- Text for the fruit ---
        const char* fruitName = "Unknown";
        const char* stockText = "Stock: ?";
        const char* inventoryText = "Inventory: ?";

        switch (b.fruitType)
        {
        case FRUIT_APPLE:
            fruitName = "Apple";
            stockText = "Stock: 3";        // demo
            inventoryText = "Inventory: 10"; // demo
            break;
        case FRUIT_PEAR:
            fruitName = "Pear";
            stockText = "Stock: 2";
            inventoryText = "Inventory: 5";
            break;
        case FRUIT_BANANA:
            fruitName = "Banana";
            stockText = "Stock: 4";
            inventoryText = "Inventory: 8";
            break;
        }

        // --- Draw the text inside the panel ---
        const float textStartY = tipY + C.tipHeight * 0.25f; // start a bit below top
        const float lineSpacing = 25.0f;

        const float xTextNorm = (tipX - C.tipWidth * 0.45f) / halfW;

        // Fruit name at top
        const float yNameNorm = textStartY / halfH;
        AEGfxPrint(fontId, fruitName, xTextNorm, yNameNorm, 1.0f, 1, 1, 1, 1);

        // Stock underneath
        const float yStockNorm = (textStartY - lineSpacing) / halfH;
        AEGfxPrint(fontId, stockText, xTextNorm, yStockNorm, 1.0f, 1, 1, 1, 1);

        // Inventory underneath
        const float yInvNorm = (textStartY - 2.0f * lineSpacing) / halfH;
        AEGfxPrint(fontId, inventoryText, xTextNorm, yInvNorm, 1.0f, 1, 1, 1, 1);

        break; // only show one tooltip at a time
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
    if (index < 0 || index >= plotSlots.size())
        return 0.0f;

    return plotSlots[index].x;
}

float UI_GetPlotSlotY(int index)
{
    if (index < 0 || index >= plotSlots.size())
        return 0.0f;

    return plotSlots[index].y;
}

void UI_Exit()
{
    // free textures here
    AEGfxTextureUnload(menuTexture);
    AEGfxTextureUnload(seedsTexture);

    AEGfxTextureUnload(inventoryIcon);
    AEGfxTextureUnload(inventoryBG);
    AEGfxTextureUnload(invFruit);
    AEGfxTextureUnload(invFruitHighlight);
    AEGfxTextureUnload(invSeed);
    AEGfxTextureUnload(invSeedHighlight);

    AEGfxTextureUnload(collectionIcon);

    AEGfxTextureUnload(settingsIcon);

    AEGfxTextureUnload(appleSeedIcon);
    AEGfxTextureUnload(appleSeedInfo);
    AEGfxTextureUnload(plotSlotTexture);
}