#include "Splash.h"
#include "AEEngine.h"
#include "GameStateManager.h"

// ---------------------------------------------------------------
// Shared mesh (created in Main.cpp)
// ---------------------------------------------------------------
extern AEGfxVertexList* g_pMeshFullScreen;

// ---------------------------------------------------------------
// Assets
// ---------------------------------------------------------------
static AEGfxTexture* g_splashLogo = nullptr;

// ---------------------------------------------------------------
// Timing
// ---------------------------------------------------------------
static const float FADE_DURATION = 1.0f;   // seconds to fade in / fade out
static const float DISPLAY_DURATION = 4.0f;   // total stage duration

static float g_timer = 0.0f;
static float g_alpha = 0.0f;

// ---------------------------------------------------------------
// GSM: Load  (textures only - called once per state entry)
// ---------------------------------------------------------------
void Splash_Load()
{
    // Updated to use the new DigiPen Singapore splash screen
    g_splashLogo = AEGfxTextureLoad("Assets/digipen_splash.png");
}

// ---------------------------------------------------------------
// GSM: Initialize  (reset state - called after Load)
// ---------------------------------------------------------------
void Splash_Initialize()
{
    g_timer = 0.0f;
    g_alpha = 0.0f;
}

// ---------------------------------------------------------------
// GSM: Update
// ---------------------------------------------------------------
void Splash_Update()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();
    g_timer += dt;

    // Fade-in
    if (g_timer < FADE_DURATION)
    {
        g_alpha = (g_timer / FADE_DURATION) * 255.0f;
    }
    // Fade-out
    else if (g_timer > DISPLAY_DURATION - FADE_DURATION)
    {
        g_alpha = ((DISPLAY_DURATION - g_timer) / FADE_DURATION) * 255.0f;
    }
    // Fully visible
    else
    {
        g_alpha = 255.0f;
    }

    // Clamp
    if (g_alpha < 0.0f)   g_alpha = 0.0f;
    if (g_alpha > 255.0f) g_alpha = 255.0f;

    // Skip on any key / mouse press
    if (AEInputCheckTriggered(AEVK_LBUTTON) ||
        AEInputCheckTriggered(AEVK_RETURN) ||
        AEInputCheckTriggered(AEVK_SPACE))
    {
        g_timer = DISPLAY_DURATION; // jump straight to end
    }

    // Transition to main screen once done
    if (g_timer >= DISPLAY_DURATION)
    {
        nextState = GS_MAIN_SCREEN;
    }
}

// ---------------------------------------------------------------
// GSM: Draw
// ---------------------------------------------------------------
void Splash_Draw()
{
    // Black background (image already has black bg, this is a safety net)
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    if (!g_splashLogo || !g_pMeshFullScreen)
        return;

    // Image is 1600x900 - same as the window, drawn fullscreen
    float normalizedAlpha = g_alpha / 255.0f;

    AEMtx33 scale, trans, transform;
    AEMtx33Scale(&scale, 1600.0f, 900.0f);
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&transform, &trans, &scale);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, normalizedAlpha);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetTransparency(normalizedAlpha);
    AEGfxTextureSet(g_splashLogo, 0, 0);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
}

// ---------------------------------------------------------------
// GSM: Free  (called before Unload on state exit)
// ---------------------------------------------------------------
void Splash_Free()
{
    // Nothing to free that isn't owned by Unload
}

// ---------------------------------------------------------------
// GSM: Unload  (release textures)
// ---------------------------------------------------------------
void Splash_Unload()
{
    if (g_splashLogo)
    {
        AEGfxTextureUnload(g_splashLogo);
        g_splashLogo = nullptr;
    }
}