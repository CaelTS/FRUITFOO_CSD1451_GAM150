#include <crtdbg.h>
#include "AEEngine.h"
#include "GameStateManager.h"

// NextScreen-specific resources
static AEGfxVertexList* pMeshNextObject = NULL;
extern s8 fontId;

void NextScreen_Load() {
    // Load NextScreen-specific resources here
    // Example: s_pMeshNextObject = CreateMesh();
}

void NextScreen_Initialize() {
  
}

void NextScreen_Update() {
    if (AEInputCheckTriggered(AEVK_M)) {
        next = GS_MAIN_SCREEN;
    }
}

void NextScreen_Render() {
    AEGfxSetBackgroundColor(0.1f, 0.3f, 0.2f);
    if (fontId >= 0) {
        AEGfxPrint(fontId, "NEXT SCREEN", -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        AEGfxPrint(fontId, "Press M to go back", -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void NextScreen_Free() {
    // Free NextScreen-specific resources
    if (pMeshNextObject) {
        AEGfxMeshFree(pMeshNextObject);
        pMeshNextObject = NULL;
    }
}

void NextScreen_Unload() {
    // Unload NextScreen-specific resources
}