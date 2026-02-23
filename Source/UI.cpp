#include "UI.h" // Add this at the top to define FruitBasket and GetFruitBaskets

#include "AEEngine.h"
#include <vector>

static bool menuOpen = false;
static AEGfxTexture* menuTexture = nullptr;
extern AEGfxVertexList* g_pMeshFullScreen;
extern s8 fontId;

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

    float menuWidth = 480.0f;
    float menuHeight = 830.0f;

    // Center of screen in YOUR engine
    float x = -760.0f + menuWidth / 2.0f;
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

            DrawTooltip(
                worldX,
                worldY,
                fruitInfo[basket.fruitType].name,
                fruitInfo[basket.fruitType].line1,
                fruitInfo[basket.fruitType].line2
            );

            break;
        }
    }
}