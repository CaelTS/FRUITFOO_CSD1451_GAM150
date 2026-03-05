#include "UI.h"
#include "Farm.h"
#include "AEEngine.h"
#include <vector>
#include <iostream>

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

// ------------------------
// Upgrades
// ------------------------
struct Upgrade { std::string name; int cost; bool purchased; };
static std::vector<Upgrade> upgrades;
static int upgradesStartIndex = 0;
static const int MAX_VISIBLE_UPGRADES = 3;
static const float UPGRADES_PANEL_W = 450.0f;
static const float UPGRADES_PANEL_H = 280.0f;
static const float UPGRADES_PANEL_X = -530.0f;
static const float UPGRADES_PANEL_Y = -160.0f;


void UI_Init()
{
    menuTexture = AEGfxTextureLoad("Assets/MenuMockup.PNG");
    seedsTexture = AEGfxTextureLoad("Assets/SeedsPanel.png");
    inventoryIcon = AEGfxTextureLoad("Assets/Inventory.png");
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
 

 //plot setup
    plotPlusButton.x = -630.0f;
    plotPlusButton.y = 150.0f;
    plotPlusButton.width = 120.0f;
    plotPlusButton.height = 120.0f;

    plotSlots.clear();

   

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
            PlotSlot slot{};
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
                    activePlotIndex = static_cast<int>(i);
                    selectedSeed = SEED_APPLE;   // SHOW INFO IMMEDIATELY
                }
            }

            break;  // stop checking other slots
        }
    }

    // DELETE SEED BUTTON (CLICK LOGIC ONLY)
    for (size_t i = 0; i < plotSlots.size(); i++)
    {
        if (!Farm_IsPlotPlanted(static_cast<int>(i)))
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
            Farm_ClearPlot(static_cast<int>(i));
            break;
        }
    }

    // --- Upgrades ---
    float upgradesPanelW = UPGRADES_PANEL_W;

    float panelX = UPGRADES_PANEL_X;
    float panelY = UPGRADES_PANEL_Y;

    float spacingUp = 70.0f;
    float startYUp = panelY + 60.0f;  


    int shownUp = 0;
    for (size_t i = upgradesStartIndex; i < upgrades.size() && shownUp < MAX_VISIBLE_UPGRADES; ++i)
    {
        auto& u = upgrades[static_cast<int>(i)];
        if (u.purchased) continue;  // skip purchased upgrades

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
            u.purchased = true;

            break;  // stop after one click
        }

        ++shownUp;
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

    AEMtx33Scale(&scale, 480, 830);
    AEMtx33Trans(&trans, -770 + 240, 0);
    AEMtx33Concat(&transform, &trans, &scale);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

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
            AEGfxPrint(fontId, "Inventory", xText, yText, 1.0f, 1, 1, 1, 1);
            AEGfxPrint(fontId, "Your items appear here.",
                xText, yText - 0.08f,
                0.8f, 1, 1, 1, 1);
            break;

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


    if (popupOpen && AEInputCheckTriggered(AEVK_Q))
    {
        popupOpen = false;
    }

    // --- Upgrades Panel ---
    float upgradesPanelW = UPGRADES_PANEL_W;
    float upgradesPanelH = UPGRADES_PANEL_H;
    float panelX = UPGRADES_PANEL_X;
    float panelY = UPGRADES_PANEL_Y;
    AEMtx33Scale(&scale, upgradesPanelW, upgradesPanelH);
    AEMtx33Trans(&trans, panelX, panelY);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(0.2f, 0.2f, 0.2f, 1.0f);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

    // Draw Header
    float headerX = (panelX - upgradesPanelW * 0.45f) / 800.0f;
    float headerY = (panelY + upgradesPanelH * 0.5f - 40.0f) / 450.0f;

    AEGfxPrint(fontId, "Upgrades", headerX, headerY, 1.0f, 1, 1, 1, 1);

    // Draw upgrades text & hover highlight
    int mx, my;
    AEInputGetCursorPosition(&mx, &my);

    float worldX = static_cast<float>(mx) - 800.0f;
    float worldY = 450.0f - static_cast<float>(my);
    float startYUp = panelY + 60.0f;
    float spacingUp = 70.0f;


    // --- Upgrades Text & Hover Highlight ---
    int visibleSlot = 0;

    for (size_t i = upgradesStartIndex;i < upgrades.size() && visibleSlot < MAX_VISIBLE_UPGRADES;++i)
    {
        if (upgrades[i].purchased)
            continue;  // skip purchased ones

        float y = startYUp - visibleSlot * spacingUp;

        float boxW = upgradesPanelW - 40.0f;
        float boxH = 50.0f;

        bool isHover =
            worldX >= panelX - boxW * 0.5f &&
            worldX <= panelX + boxW * 0.5f &&
            worldY >= y - boxH * 0.5f &&
            worldY <= y + boxH * 0.5f;

        if (isHover)
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(0.9f, 0.8f, 0.2f, 0.5f);

            AEMtx33Scale(&scale, boxW, boxH);
            AEMtx33Trans(&trans, panelX, y);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
        }

        char buf[128];
        sprintf_s(buf, "%s - %d Gold",
            upgrades[i].name.c_str(),
            upgrades[i].cost);

        float xText = (panelX - upgradesPanelW * 0.45f) / 800.0f;
        float yText = (y + boxH * 0.1f) / 450.0f;

        AEGfxPrint(fontId, buf, xText, yText, 0.8f, 1, 1, 1, 1);

        visibleSlot++;
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

        // Highlight
        if (hoveredSeed == SEED_APPLE)
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 0.55f, 0.0f, 0.9f);

            AEMtx33Scale(&scale, 112, 112);   // slightly larger than icon
            AEMtx33Trans(&trans, seedspanelX, seedY);
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

static bool IsMouseOverBasket(const FruitBasket& basket)
{
    int mx, my;
    AEInputGetCursorPosition(&mx, &my);

    float worldX = static_cast<float>(mx) - 800.0f;
    float worldY = 450.0f - static_cast<float>(my);

    return worldX >= basket.x - basket.width * 0.5f &&
        worldX <= basket.x + basket.width * 0.5f &&
        worldY >= basket.y - basket.height * 0.5f &&
        worldY <= basket.y + basket.height * 0.5f;
}

static void DrawTooltip(float x, float y, const char* text)
{
    float w = 250.0f;
    float h = 80.0f;

    AEMtx33 scale, trans, transform;

    AEMtx33Scale(&scale, w, h);
    AEMtx33Trans(&trans, x, y);
    AEMtx33Concat(&transform, &trans, &scale);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 0.9f);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

    float xText = (x - w * 0.45f) / 800.0f;
    float yText = (y + h * 0.25f) / 450.0f;

    AEGfxPrint(fontId, text, xText, yText, 1.0f, 1, 1, 1, 1);
}


void UI_DrawFruitBasketTooltips()
{
    if (UI_IsMenuOpen())
        return;

    const auto& baskets = GetFruitBaskets();

    for (const auto& basket : baskets)
    {
        if (IsMouseOverBasket(basket))
        {
            int mx, my;
            AEInputGetCursorPosition(&mx, &my);

            float worldX = static_cast<float>(mx) - 800.0f + 20.0f;
            float worldY = 450.0f - static_cast<float>(my) - 20.0f;

            DrawTooltip(worldX, worldY, "Fruit Basket");
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
    AEGfxTextureUnload(collectionIcon);
    AEGfxTextureUnload(settingsIcon);
    AEGfxTextureUnload(appleSeedIcon);
    AEGfxTextureUnload(appleSeedInfo);
    AEGfxTextureUnload(plotSlotTexture);
}