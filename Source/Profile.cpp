#include "AEEngine.h"
#include "Transition.h"
#include "GameStateManager.h"
#include "Profile.h"
#include <crtdbg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


extern AEGfxVertexList* g_pMeshFullScreen;   // from Main.cpp
extern s8 fontId;

// NextScreen-specific resources
static AEGfxVertexList* pMeshProfile = NULL;


void ProfileScreen_Load() {
    // Load ProfileScreen-specific resources here
    // Example: s_pMeshProfile = CreateMesh();
}

void ProfileScreen_Initialize() {
  
}

void ProfileScreen_Update() {
    if (AEInputCheckTriggered(AEVK_M)) {
        next = GS_MAIN_SCREEN;
    }
}

void ProfileScreen_Render() {
    AEGfxSetBackgroundColor(0.1f, 0.3f, 0.2f);
    if (fontId >= 0) {
        AEGfxPrint(fontId, "NEXT SCREEN", -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        AEGfxPrint(fontId, "Press M to go back", -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (TR_IsActive())
    {
        float alpha = TR_GetAlpha();

        // full-screen black quad
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(0, 0, 0, alpha);

        AEMtx33 trans, scale, transform;
        AEMtx33Scale(&scale, 1600.0f, 900.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
    }
}

void ProfileScreen_Free() {
    // Free ProfileScreen-specific resources
    if (pMeshProfile) {
        AEGfxMeshFree(pMeshProfile);
        pMeshProfile = NULL;
    }
}

void ProfileScreen_Unload() {
    // Unload ProfileScreen-specific resources
}