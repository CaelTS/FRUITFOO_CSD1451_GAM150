#include "UI.h"
#include "Farm.h"
#include "AEEngine.h"
#include <vector>

extern AEGfxVertexList* g_pMeshFullScreen;
extern s8 fontId;

static bool menuOpen = false;
static bool popupOpen = false;
static bool seedsPopupOpen = false;

static int activePopupIndex = -1;
static int selectedSeed = -1;

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


void UI_Init()
{
    menuTexture = AEGfxTextureLoad("Assets/MenuMockup.PNG");
    seedsTexture = AEGfxTextureLoad("Assets/SeedsPanel.png");
    inventoryIcon = AEGfxTextureLoad("Assets/Inventory.png");
    collectionIcon = AEGfxTextureLoad("Assets/Collection.png");
    settingsIcon = AEGfxTextureLoad("Assets/Settings.png");
    appleSeedIcon = AEGfxTextureLoad("Assets/AppleSeed.png");
    appleSeedInfo = AEGfxTextureLoad("Assets/AppleSeedInfo.png");

    menuButtons.clear();

    float startX = -675.0f;
    float spacing = 150.0f;
    float buttonY = -350.0f;

    for (int i = 0; i < 3; i++)
    {
        MenuButton b;
        b.x = startX + i * spacing;
        b.y = buttonY;
        b.width = 96;
        b.height = 96;
        b.isHovered = false;
        b.type = (ButtonType)i;
        menuButtons.push_back(b);
    }

    plotPlusButton.x = -630.0f;
    plotPlusButton.y = 150.0f;
    plotPlusButton.width = 120.0f;
    plotPlusButton.height = 120.0f;
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

    float worldX = mx - 800.0f;
    float worldY = 450.0f - my;

    // Menu buttons
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

    // Plot +
    plotPlusButton.isHovered =
        worldX >= plotPlusButton.x - plotPlusButton.width * 0.5f &&
        worldX <= plotPlusButton.x + plotPlusButton.width * 0.5f &&
        worldY >= plotPlusButton.y - plotPlusButton.height * 0.5f &&
        worldY <= plotPlusButton.y + plotPlusButton.height * 0.5f;

    if (plotPlusButton.isHovered && AEInputCheckTriggered(AEVK_LBUTTON))
        seedsPopupOpen = !seedsPopupOpen;

    // Seed selection
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

        if (overSeed && AEInputCheckTriggered(AEVK_LBUTTON))
            selectedSeed = SEED_APPLE;

        if (AEInputCheckTriggered(AEVK_LBUTTON) && !overSeed)
            selectedSeed = -1;
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

    // --- Seeds Panel ---
    if (seedsPopupOpen)
    {
        float panelX = -100.0f;
        float panelY = 0.0f;

        AEGfxTextureSet(seedsTexture, 0, 0);
        AEMtx33Scale(&scale, 400, 550);
        AEMtx33Trans(&trans, panelX, panelY);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        float seedY = panelY + 120.0f;

        // Highlight
        if (selectedSeed == SEED_APPLE)
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetColorToMultiply(1.0f, 0.9f, 0.3f, 0.5f);

            AEMtx33Scale(&scale, 120, 120);
            AEMtx33Trans(&trans, panelX, seedY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
        }

        // Draw seed
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxTextureSet(appleSeedIcon, 0, 0);

        AEMtx33Scale(&scale, 100, 100);
        AEMtx33Trans(&trans, panelX, seedY);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
    }

    // --- Apple Info ---
    if (selectedSeed == SEED_APPLE)
    {
        float panelX = -100.0f;
        float panelY = 0.0f;

        float seedY = panelY + 120.0f;

        float infoW = 380.0f;
        float infoH = 340.0f;

        float infoX = panelX;
        float infoY = seedY - 220.0f;   // positioned under the seed


        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
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

