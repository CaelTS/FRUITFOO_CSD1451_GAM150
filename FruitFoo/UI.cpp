#include "UI.h" // Add this at the top to define FruitBasket and GetFruitBaskets

#include "AEEngine.h"
#include <vector>

static bool menuOpen = false;
static AEGfxTexture* menuTexture = nullptr;
extern AEGfxVertexList* g_pMeshFullScreen;

struct FruitInfo
{
    const char* name;
    const char* description;
};

static FruitInfo fruitInfo[3] =
{
    { "Apple",  "Sweet red fruit\nPrice: 10 gold" },
    { "Pear",   "Juicy green fruit\nPrice: 10 gold" },
    { "Banana", "Soft yellow fruit\nPrice: 10 gold" }
};


void UI_Init()
{
    menuTexture = AEGfxTextureLoad("Assets/MenuMockup.PNG");
    if (!menuTexture)
        printf("ERROR: MenuMockup.png failed to load!\n");

    else
        printf("MenuMockup.png loaded successfully!\n");
}


void UI_Input()
{
    if (AEInputCheckTriggered(AEVK_M))
    {
        menuOpen = !menuOpen;
    }
}

void UI_Draw()
{
    if (!menuOpen)
        return;

    float menuWidth = 300.0f;
    float menuHeight = 660.0f;

    // Center of screen in YOUR engine
    float x = -800.0f + menuWidth / 2.0f;
    float y = 0.0f;

    AEMtx33 scale, trans, transform;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxTextureSet(menuTexture, 0, 0);

    AEMtx33Scale(&scale, menuWidth, menuHeight);
    AEMtx33Trans(&trans, x, y);
    AEMtx33Concat(&transform, &trans, &scale);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
}


void UI_Exit()
{
    if (menuTexture)
    {
        AEGfxTextureUnload(menuTexture);
        menuTexture = nullptr;
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

static void DrawTooltip(float x, float y, const char* title, const char* desc)
{
    float w = 220.0f;
    float h = 90.0f;

    AEMtx33 scale, trans, transform;
    AEMtx33Scale(&scale, w, h);
    AEMtx33Trans(&trans, x, y);
    AEMtx33Concat(&transform, &trans, &scale);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 0.9f);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

    // AEGfxPrint expects 9 arguments: fontId, str, x, y, scale, r, g, b, a
    AEGfxPrint(0, title, (x - w * 0.45f) / 800.0f,
        (y + h * 0.15f) / 450.0f, 1, 1, 1, 1, 1);

    AEGfxPrint(0, desc, (x - w * 0.45f) / 800.0f,
        (y - h * 0.15f) / 450.0f, 0.9f, 0.9f, 0.9f, 0.9f, 1);
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

            DrawTooltip(
                worldX,
                worldY,
                fruitInfo[basket.fruitType].name,
                fruitInfo[basket.fruitType].description
            );
            break;
        }
    }
}