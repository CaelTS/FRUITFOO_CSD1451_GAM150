#include "UI.h"
#include "AEEngine.h"

static bool menuOpen = false;
static AEGfxTexture* menuTexture = nullptr;
extern AEGfxVertexList* g_pMeshFullScreen;


void UI_Init()
{
    menuTexture = AEGfxTextureLoad("Assets/MenuMockup.png");
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

    float menuWidth = 600.0f;
    float menuHeight = 400.0f;

    // Center of screen in YOUR engine
    float x = 0.0f;
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
