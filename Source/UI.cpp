#include "UI.h" // Add this at the top to define FruitBasket and GetFruitBaskets
#include "Farm.h"
#include "AEEngine.h"
#include <vector>



static bool menuOpen = false;
static AEGfxTexture* menuTexture = nullptr;
extern AEGfxVertexList* g_pMeshFullScreen;
extern s8 fontId;

static bool popupOpen = false;
static int activePopupIndex = -1;
static AEGfxTexture* inventoryIcon = nullptr;
static AEGfxTexture* collectionIcon = nullptr;
static AEGfxTexture* settingsIcon = nullptr;

//for seed panel
static float dragOffsetX = 0.0f;
static float dragOffsetY = 0.0f;
static float doubleClickTimer = 0.0f;
static int lastClickedSeed = -1;

enum SeedType
{
    SEED_APPLE = 0,
   // SEED_PEAR,
  //  SEED_BANANA,
    SEED_COUNT = 1
};

struct SeedData
{
    AEGfxTexture* icon;
    float offsetY;   // position inside panel
};

static SeedData seedData[SEED_COUNT];

static bool seedsPopupOpen = false;
static AEGfxTexture* seedsTexture = nullptr;




struct FruitInfo
{
    const char* name;
    const char* line1;
    const char* line2;
};


static FruitInfo fruitInfo[3] =
{
    { "Apple",  "Sweet red fruit",  "Price: 10 gold" },
    { "Pear",   "Juicy green fruit", "Price: 10 gold" },
    { "Banana", "Soft yellow fruit", "Price: 10 gold" }
};

enum ButtonType
{
    BUTTON_INVENTORY,
    BUTTON_COLLECTION,
    BUTTON_SETTINGS
};

struct MenuButton
{
    float x;
    float y;
    float width;
    float height;
    bool isHovered;
    ButtonType type;
};
static std::vector<MenuButton> menuButtons;

static MenuButton plotPlusButton;

void UI_Init()
{
    //Menu background
    menuTexture = AEGfxTextureLoad("Assets/MenuMockup.PNG");
    if (!menuTexture)
        printf("ERROR: MenuMockup.png failed to load!\n");

    else
        printf("MenuMockup.png loaded successfully!\n");

    seedsTexture = AEGfxTextureLoad("Assets/SeedsPanel.png");
    if (!seedsTexture)
        printf("ERROR: SeedsPanel.png failed to load!\n");
    else
        printf("SeedsPanel.png loaded successfully!\n");
    seedData[SEED_APPLE].icon = AEGfxTextureLoad("Assets/AppleSeed.png");
 //   seedData[SEED_PEAR].icon = AEGfxTextureLoad("Assets/PearSeed.png");
  //  seedData[SEED_BANANA].icon = AEGfxTextureLoad("Assets/BananaSeed.png");

    seedData[SEED_APPLE].offsetY = 120.0f;
  //  seedData[SEED_PEAR].offsetY = 20.0f;
  //  seedData[SEED_BANANA].offsetY = -80.0f;
    //Menu Buttons 
    menuButtons.clear();

    float buttonY = -350.0f; // near bottom of window
    float spacingX = 150.0f;  // horizontal space between buttons
    float startX = -675.0f; // leftmost button


    // Plot + button (adjust if needed)
    plotPlusButton.x = -630.0f;
    plotPlusButton.y = 150.0f;
    plotPlusButton.width = 120.0f;
    plotPlusButton.height = 120.0f;
    plotPlusButton.isHovered = false;


    for (int i = 0; i < 3; ++i)
    {
        MenuButton button;
        button.x = startX + i * spacingX; // move right for each button
        button.y = buttonY;
        button.width = 96.0f;
        button.height = 96.0f;
        button.isHovered = false;

        if (i == 0) button.type = BUTTON_INVENTORY;
        if (i == 1) button.type = BUTTON_COLLECTION;
        if (i == 2) button.type = BUTTON_SETTINGS;

        menuButtons.push_back(button);
    }

    inventoryIcon = AEGfxTextureLoad("Assets/Inventory.png");
    collectionIcon = AEGfxTextureLoad("Assets/Collection.png");
    settingsIcon = AEGfxTextureLoad("Assets/Settings.png");

}


void UI_Input()
{
    if (AEInputCheckTriggered(AEVK_M))
    {
        menuOpen = !menuOpen;
    }

    // Update buttons when menu is open
    if (menuOpen)
        UI_UpdateButtons();

    // Close popup with ESC
    if (popupOpen && AEInputCheckTriggered(AEVK_Q))
    {
        popupOpen = false;
        activePopupIndex = -1;
    }

}

void UI_UpdateButtons()
{

    float dt = (float)AEFrameRateControllerGetFrameTime();
    doubleClickTimer += dt;
    if (menuOpen)
    {
        // ---- MENU DRAWING CODE HERE ----
    }

    int mx, my;
    AEInputGetCursorPosition(&mx, &my);

    float worldX = static_cast<float>(mx) - 800.0f;
    float worldY = 450.0f - static_cast<float>(my);

    for (int i = 0; i < (int)menuButtons.size(); ++i)
    {
        auto& button = menuButtons[i];

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

    // -----------------------------
    // Plot + Button Click Detection
    // -----------------------------

    plotPlusButton.isHovered =
        worldX >= plotPlusButton.x - plotPlusButton.width * 0.5f &&
        worldX <= plotPlusButton.x + plotPlusButton.width * 0.5f &&
        worldY >= plotPlusButton.y - plotPlusButton.height * 0.5f &&
        worldY <= plotPlusButton.y + plotPlusButton.height * 0.5f;

    if (plotPlusButton.isHovered && AEInputCheckTriggered(AEVK_LBUTTON))
    {
        seedsPopupOpen = !seedsPopupOpen; // toggle
    }

    // Close only if clicking outside panel entirely
    if (seedsPopupOpen && AEInputCheckTriggered(AEVK_LBUTTON))
    {
        float popupX = -100.0f;
        float popupY = 0.0f;
        float panelW = 400.0f;
        float panelH = 550.0f;

        bool insidePanel =
            worldX >= popupX - panelW * 0.5f &&
            worldX <= popupX + panelW * 0.5f &&
            worldY >= popupY - panelH * 0.5f &&
            worldY <= popupY + panelH * 0.5f;

        if (!insidePanel && !plotPlusButton.isHovered)
        {
            seedsPopupOpen = false;
        }
    }

    // -----------------------------
// Start Dragging Seed
// -----------------------------
    if (seedsPopupOpen)
    {
        float popupX = -100.0f;
        float popupY = 0.0f;

        for (int i = 0; i < SEED_COUNT; i++)
        {
            float seedX = popupX;
            float seedY = popupY + seedData[i].offsetY;

            float seedW = 100.0f;
            float seedH = 100.0f;

            bool overSeed =
                worldX >= seedX - seedW * 0.5f &&
                worldX <= seedX + seedW * 0.5f &&
                worldY >= seedY - seedH * 0.5f &&
                worldY <= seedY + seedH * 0.5f;

            if (overSeed && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                if (lastClickedSeed == i && doubleClickTimer < 0.3f)
                {
                    // DOUBLE CLICK DETECTED
                    Farm_PlantSeed(i);
                    doubleClickTimer = 0.0f;
                }
                else
                {
                    // First click
                    lastClickedSeed = i;
                    doubleClickTimer = 0.0f;
                    
                }
            }
        }
    }
        

    
    }


    void UI_Draw()
    {
        if (!menuOpen)
            return;

        AEMtx33 scale, trans, transform;

        float menuWidth = 480.0f;
        float menuHeight = 830.0f;

        float x = -770.0f + menuWidth / 2.0f;
        float y = 0.0f;

        // -------------------------
        // Draw Menu Background
        // -------------------------
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxTextureSet(menuTexture, 0, 0);

        AEMtx33Scale(&scale, menuWidth, menuHeight);
        AEMtx33Trans(&trans, x, y);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        // -------------------------
        // Menu Buttons
        // -------------------------
        for (auto& button : menuButtons)
        {
            AEMtx33Scale(&scale, button.width, button.height);
            AEMtx33Trans(&trans, button.x, button.y);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(
                button.isHovered ? 1.0f : 1.0f,
                button.isHovered ? 0.9f : 1.0f,
                button.isHovered ? 0.9f : 1.0f,
                1.0f
            );

            switch (button.type)
            {
            case BUTTON_INVENTORY:  AEGfxTextureSet(inventoryIcon, 0, 0); break;
            case BUTTON_COLLECTION: AEGfxTextureSet(collectionIcon, 0, 0); break;
            case BUTTON_SETTINGS:   AEGfxTextureSet(settingsIcon, 0, 0); break;
            }

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
        }

        // -------------------------
        // Popup
        // -------------------------
        if (popupOpen)
        {
            float w = 400.0f;
            float h = 250.0f;
            float popupX = 0.0f;
            float popupY = 0.0f;

            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetColorToMultiply(0.15f, 0.15f, 0.15f, 0.95f);

            AEMtx33Scale(&scale, w, h);
            AEMtx33Trans(&trans, popupX, popupY);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            float xText = (popupX - w * 0.45f) / 800.0f;
            float yText = (popupY + h * 0.25f) / 450.0f;

            switch (activePopupIndex)
            {
            case BUTTON_INVENTORY:
                AEGfxPrint(fontId, "Inventory", xText, yText, 1.0f, 1, 1, 1, 1);
                break;
            case BUTTON_COLLECTION:
                AEGfxPrint(fontId, "Collection", xText, yText, 1.0f, 1, 1, 1, 1);
                break;
            case BUTTON_SETTINGS:
                AEGfxPrint(fontId, "Settings", xText, yText, 1.0f, 1, 1, 1, 1);
                break;
            }
        }



        // -------------------------
        // Seeds Panel
        // -------------------------
        if (seedsPopupOpen)
        {
            float panelW = 400.0f;
            float panelH = 550.0f;
            float popupX = -100.0f;
            float popupY = 0.0f;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxTextureSet(seedsTexture, 0, 0);

            AEMtx33Scale(&scale, panelW, panelH);
            AEMtx33Trans(&trans, popupX, popupY);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            // -------------------------
 // Draw Seed Icons
 // -------------------------

            int mx, my;
            AEInputGetCursorPosition(&mx, &my);

            float worldX = static_cast<float>(mx) - 800.0f;
            float worldY = 450.0f - static_cast<float>(my);

            for (int i = 0; i < SEED_COUNT; i++)
            {
                if (!seedData[i].icon)
                    continue;

                float seedX = popupX;
                float seedY = popupY + seedData[i].offsetY;

                float seedW = 100.0f;
                float seedH = 100.0f;

                bool isHovered =
                    worldX >= seedX - seedW * 0.5f &&
                    worldX <= seedX + seedW * 0.5f &&
                    worldY >= seedY - seedH * 0.5f &&
                    worldY <= seedY + seedH * 0.5f;

                // ---- Highlight Background ----
                if (isHovered)
                {
                    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                    AEGfxSetColorToMultiply(1.0f, 0.9f, 0.3f, 0.5f);

                    AEMtx33Scale(&scale, seedW + 20.0f, seedH + 20.0f);
                    AEMtx33Trans(&trans, seedX, seedY);
                    AEMtx33Concat(&transform, &trans, &scale);

                    AEGfxSetTransform(transform.m);
                    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
                }

                // ---- Draw Seed Texture ----
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetColorToMultiply(1, 1, 1, 1);
                AEGfxTextureSet(seedData[i].icon, 0, 0);

                AEMtx33Scale(&scale, seedW, seedH);
                AEMtx33Trans(&trans, seedX, seedY);
                AEMtx33Concat(&transform, &trans, &scale);

                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }
        }
    }

void UI_Exit()
{
    if (menuTexture)
    {
        AEGfxTextureUnload(menuTexture);
        menuTexture = nullptr;
    }

    if (seedsTexture)
    {
        AEGfxTextureUnload(seedsTexture);
        seedsTexture = nullptr;
    }

    AEGfxTextureUnload(inventoryIcon);
    AEGfxTextureUnload(collectionIcon);
    AEGfxTextureUnload(settingsIcon);


    for (int i = 0; i < SEED_COUNT; i++)
    {
        if (seedData[i].icon)
        {
            AEGfxTextureUnload(seedData[i].icon);
            seedData[i].icon = nullptr;
        }
    }
}

bool UI_IsMenuOpen()
{
    return menuOpen;
}


static bool IsMouseOverBasket(const FruitBasket& basket)
{
    int mx, my; // Use int for AEInputGetCursorPosition
    AEInputGetCursorPosition(&mx, &my);

    float worldX = static_cast<float>(mx) - 800.0f;
    float worldY = 450.0f - static_cast<float>(my);

    return worldX >= basket.x - basket.width * 0.5f &&
        worldX <= basket.x + basket.width * 0.5f &&
        worldY >= basket.y - basket.height * 0.5f &&
        worldY <= basket.y + basket.height * 0.5f;
}

static void DrawTooltip(float x, float y, const char* title, const char* line1, const char* line2)

{
    float w = 300.0f;
    float h = 100.0f;

    AEMtx33 scale, trans, transform;
    AEMtx33Scale(&scale, w, h);
    AEMtx33Trans(&trans, x, y);
    AEMtx33Concat(&transform, &trans, &scale);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 0.9f);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

    // ---- TEXT POSITION (THIS WAS MISSING) ----
    float xText = (x - w * 0.45f) / 800.0f;
    float yText = (y + h * 0.25f) / 450.0f;

    // Title
    AEGfxPrint(fontId, title, xText, yText, 1.0f, 1, 1, 1, 1);

    // Line 1
    AEGfxPrint(fontId, line1, xText, yText - 0.08f, 0.9f, 0.9f, 0.9f, 0.9f, 1);

    // Line 2
    AEGfxPrint(fontId, line2, xText, yText - 0.16f, 0.9f, 0.9f, 0.9f, 0.9f, 1);

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
            int mx, my; // Use int for AEInputGetCursorPosition
            AEInputGetCursorPosition(&mx, &my);

            float worldX = static_cast<float>(mx) - 800.0f + 20.0f;
            float worldY = 450.0f - static_cast<float>(my) - 20.0f;

            if (basket.fruitType >= 0 && basket.fruitType < 3)
            {
                DrawTooltip(
                    worldX,
                    worldY,
                    fruitInfo[basket.fruitType].name,
                    fruitInfo[basket.fruitType].line1,
                    fruitInfo[basket.fruitType].line2
                );
            }

            break;
        }
    }
}

