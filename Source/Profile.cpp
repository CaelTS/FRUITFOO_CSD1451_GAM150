#include <crtdbg.h>
#include "AEEngine.h"
#include "Transition.h"
#include "GameStateManager.h"

extern AEGfxVertexList* g_pMeshFullScreen;
extern s8 fontId;

// UI Textures
AEGfxTexture* pTexButtonLong = NULL;
AEGfxTexture* pTexButtonLongPressed = NULL;
AEGfxTexture* pTexButtonSquare = NULL;
AEGfxTexture* pTexInputRect = NULL;
AEGfxTexture* pTexCrossIcon = NULL;
AEGfxTexture* pTexPanel = NULL;

// Profile data structure
constexpr auto MAX_PROFILES = 3;
constexpr auto PROFILE_NAME_MAX_LEN = 32;

typedef struct {
    bool exists;
    char name[PROFILE_NAME_MAX_LEN];
    int level;
    int score;
} Profile;

static Profile profiles[MAX_PROFILES] = {
    { true, "Player 1", 5, 1250 },
    { false, "", 0, 0 },
    { false, "", 0, 0 }
};

// Popup state
static bool  popupActive = false;
static int   popupSlotIndex = -1;           // which empty slot triggered the popup
static char  popupInputBuf[PROFILE_NAME_MAX_LEN] = "";
static int   popupInputLen = 0;

bool ProfileScreen_IsPopupActive() { return popupActive; }

// UI Layout constants - Centered
static const float SCREEN_WIDTH = 1600.0f;
static const float SCREEN_HEIGHT = 900.0f;
static const float BUTTON_WIDTH_PX = 400.0f;
static const float BUTTON_HEIGHT_PX = 80.0f;
static const float DELETE_BUTTON_SIZE_PX = 40.0f;
static const float PROFILE_SPACING_PX = 120.0f;
static const float START_Y_PX = 150.0f;

// ProfileScreen-specific resources
static AEGfxVertexList* pMeshButtonLong = NULL;
static AEGfxVertexList* pMeshButtonSquare = NULL;
static AEGfxVertexList* pMeshNextObject = NULL;

// Helper function to convert pixels to NDC
static float PixelsToNDC_X(float pixels) {
    return pixels / (SCREEN_WIDTH * 0.5f);
}

static float PixelsToNDC_Y(float pixels) {
    return pixels / (SCREEN_HEIGHT * 0.5f);
}

// Helper function to draw textured quad with color tint
static void DrawTexturedQuad(AEGfxTexture* texture, AEGfxVertexList* mesh,
    float x, float y, float width, float height,
    float r, float g, float b, float a) {
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(r, g, b, a);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxTextureSet(texture, 0, 0);

    AEMtx33 trans, scale, transform;
    // Convert NDC sizes back to pixel-space for the transform matrix
    AEMtx33Scale(&scale, width * (SCREEN_WIDTH * 0.5f), height * (SCREEN_HEIGHT * 0.5f));
    AEMtx33Trans(&trans, x * (SCREEN_WIDTH * 0.5f), y * (SCREEN_HEIGHT * 0.5f));
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);

    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

// Helper function to draw colored quad
static void DrawColoredQuad(float x, float y, float width, float height,
    float r, float g, float b, float a) {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(r, g, b, a);

    AEMtx33 trans, scale, transform;
    // Convert NDC sizes back to pixel-space for the transform matrix
    AEMtx33Scale(&scale, width * (SCREEN_WIDTH * 0.5f), height * (SCREEN_HEIGHT * 0.5f));
    AEMtx33Trans(&trans, x * (SCREEN_WIDTH * 0.5f), y * (SCREEN_HEIGHT * 0.5f));
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);

    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
}

void ProfileScreen_Load() {
    // Load Textures
    pTexButtonLong = AEGfxTextureLoad("Assets/buttonLong_brown.png");
    pTexButtonLongPressed = AEGfxTextureLoad("Assets/buttonLong_brown_pressed.png");
    pTexButtonSquare = AEGfxTextureLoad("Assets/buttonSquare_brown.png");
    pTexInputRect = AEGfxTextureLoad("Assets/input_outline_rectangle.png");
    pTexCrossIcon = AEGfxTextureLoad("Assets/iconCross_blue.png");
    pTexPanel = AEGfxTextureLoad("Assets/panel_brown.png");

    if (!pTexButtonLong) OutputDebugStringA("ERROR: Failed to load 'Assets/buttonLong_brown.png'.\n");
    if (!pTexButtonLongPressed) OutputDebugStringA("ERROR: Failed to load 'buttonLong_brown_pressed.png'.\n");
    if (!pTexButtonSquare) OutputDebugStringA("ERROR: Failed to load 'buttonSquare_brown.png'.\n");
    if (!pTexInputRect) OutputDebugStringA("ERROR: Failed to load 'input_outline_rectangle.png'.\n");
    if (!pTexCrossIcon) OutputDebugStringA("ERROR: Failed to load 'iconCross_blue.png'.\n");
    if (!pTexPanel)     OutputDebugStringA("ERROR: Failed to load 'panel_brown.png'.\n");
}

void ProfileScreen_Initialize() {
    fontId = AEGfxCreateFont("Assets/liberation-mono.ttf", 26);
    if (fontId < 0)
        OutputDebugStringA("ERROR: Failed to load 'Assets/liberation-mono.ttf'.\n");

    // Create mesh for long buttons (profile slots)
    AEGfxMeshStart();
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f
    );
    AEGfxTriAdd(
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f
    );
    pMeshButtonLong = AEGfxMeshEnd();

    // Create mesh for square buttons (delete buttons)
    AEGfxMeshStart();
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f
    );
    AEGfxTriAdd(
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f
    );
    pMeshButtonSquare = AEGfxMeshEnd();
}

void ProfileScreen_Update() {
    // --- Popup active: handle text input ---
    if (popupActive) {
        // Confirm with Enter
        if (AEInputCheckTriggered(AEVK_RETURN)) {
            if (popupInputLen > 0 && popupSlotIndex >= 0 && popupSlotIndex < MAX_PROFILES) {
                strncpy_s(profiles[popupSlotIndex].name, PROFILE_NAME_MAX_LEN,
                    popupInputBuf, _TRUNCATE);
                profiles[popupSlotIndex].level = 1;
                profiles[popupSlotIndex].score = 0;
                profiles[popupSlotIndex].exists = true;
            }
            popupActive = false;
            popupSlotIndex = -1;
            popupInputBuf[0] = '\0';
            popupInputLen = 0;
        }
        // Cancel with Escape
        else if (AEInputCheckTriggered(AEVK_ESCAPE)) {
            popupActive = false;
            popupSlotIndex = -1;
            popupInputBuf[0] = '\0';
            popupInputLen = 0;
        }
        // Backspace
        else if (AEInputCheckTriggered(AEVK_BACK) && popupInputLen > 0) {
            popupInputLen--;
            popupInputBuf[popupInputLen] = '\0';
        }
        else {
            // Printable ASCII characters (letters, numbers, spaces, symbols)
            for (u8 key = 32; key < 127; key++) {
                if (AEInputCheckTriggered(key)) {
                    bool shift = AEInputCheckCurr(AEVK_RSHIFT) || AEInputCheckCurr(AEVK_LSHIFT);
                    char c = (char)key;

                    // Letters: apply shift for uppercase
                    if (c >= 'A' && c <= 'Z') {
                        if (!shift) c = c + 32; // lowercase by default
                    }
                    // Digits row with shift = symbols
                    else if (shift) {
                        switch (c) {
                        case '1': c = '!'; break; case '2': c = '@'; break;
                        case '3': c = '#'; break; case '4': c = '$'; break;
                        case '5': c = '%'; break; case '6': c = '^'; break;
                        case '7': c = '&'; break; case '8': c = '*'; break;
                        case '9': c = '('; break; case '0': c = ')'; break;
                        case '-': c = '_'; break; case '=': c = '+'; break;
                        default: break;
                        }
                    }

                    if (popupInputLen < PROFILE_NAME_MAX_LEN - 1) {
                        popupInputBuf[popupInputLen++] = c;
                        popupInputBuf[popupInputLen] = '\0';
                    }
                }
            }
        }
        return; // Block all other input while popup is open
    }

    // --- Normal update: Escape to go back ---
    // Guard against popupActive so closing the popup's ESC never leaks through
    if (!popupActive && AEInputCheckTriggered(AEVK_ESCAPE)) {
        next = GS_MAIN_SCREEN;
    }

    // Mouse click detection for "New Profile" empty slots
    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    // Convert mouse from window pixels to NDC
    float mNDC_X = ((float)mouseX / (SCREEN_WIDTH * 0.5f)) - 1.0f;
    float mNDC_Y = 1.0f - ((float)mouseY / (SCREEN_HEIGHT * 0.5f));

    float buttonW = PixelsToNDC_X(BUTTON_WIDTH_PX);
    float buttonH = PixelsToNDC_Y(BUTTON_HEIGHT_PX);
    float spacing = PixelsToNDC_Y(PROFILE_SPACING_PX);
    float startY = PixelsToNDC_Y(START_Y_PX);

    if (AEInputCheckTriggered(VK_LBUTTON)) {
        for (int i = 0; i < MAX_PROFILES; i++) {
            if (!profiles[i].exists) {
                float yPos = startY - (i * spacing);
                float halfW = buttonW * 0.5f;
                float halfH = buttonH * 0.5f;
                if (mNDC_X >= -halfW && mNDC_X <= halfW &&
                    mNDC_Y >= yPos - halfH && mNDC_Y <= yPos + halfH) {
                    // Open popup for this slot
                    popupActive = true;
                    popupSlotIndex = i;
                    popupInputBuf[0] = '\0';
                    popupInputLen = 0;
                    break;
                }
            }
        }
    }
}

void ProfileScreen_Render() {
    // Lighter background to contrast with brown buttons
    AEGfxSetBackgroundColor(0.08f, 0.06f, 0.04f);

    // Title - Centered at top
    if (fontId >= 0) {
        AEGfxPrint(fontId, "SELECT PROFILE", -0.25f, 0.75f, 1.0f, 1.0f, 0.8f, 0.4f, 1.0f);
    }

    // Calculate center positions
    float buttonW = PixelsToNDC_X(BUTTON_WIDTH_PX);
    float buttonH = PixelsToNDC_Y(BUTTON_HEIGHT_PX);
    float deleteSize = PixelsToNDC_X(DELETE_BUTTON_SIZE_PX);
    float spacing = PixelsToNDC_Y(PROFILE_SPACING_PX);
    float startY = PixelsToNDC_Y(START_Y_PX);

    // Render up to 3 profile slots - Centered horizontally
    for (int i = 0; i < MAX_PROFILES; i++) {
        float yPos = startY - (i * spacing);

        if (profiles[i].exists) {
            // Shadow behind button
            DrawColoredQuad(0.0f, yPos - 0.005f, buttonW + 0.01f, buttonH + 0.01f,
                0.0f, 0.0f, 0.0f, 0.5f);

            // Brown long button - full brightness for existing profile
            DrawTexturedQuad(pTexButtonLong, pMeshButtonLong,
                0.0f, yPos, buttonW, buttonH,
                1.0f, 1.0f, 1.0f, 1.0f);

            // Profile name - inside button, slightly above center
            if (fontId >= 0) {
                AEGfxPrint(fontId, profiles[i].name,
                    -0.10f, yPos + 0.020f,
                    0.75f, 1.0f, 0.95f, 0.8f, 1.0f);

                // Level and score - inside button, slightly below name
                char infoText[64];
                sprintf_s(infoText, sizeof(infoText), "Lvl:%d  Score:%d",
                    profiles[i].level, profiles[i].score);
                AEGfxPrint(fontId, infoText,
                    -0.13f, yPos - 0.030f,
                    0.5f, 0.85f, 0.75f, 0.6f, 1.0f);
            }

            // Delete (X) square button - right of long button
            float deleteH = deleteSize * (SCREEN_WIDTH / SCREEN_HEIGHT);
            float deleteX = buttonW * 0.5f + deleteSize * 1.0f;

            DrawColoredQuad(deleteX + 0.003f, yPos - 0.003f,
                deleteSize + 0.006f, deleteH + 0.006f,
                0.0f, 0.0f, 0.0f, 0.5f);

            DrawTexturedQuad(pTexButtonSquare, pMeshButtonSquare,
                deleteX, yPos, deleteSize, deleteH,
                1.0f, 1.0f, 1.0f, 1.0f);

            DrawTexturedQuad(pTexCrossIcon, pMeshButtonSquare,
                deleteX, yPos, deleteSize * 0.55f, deleteH * 0.55f,
                1.0f, 1.0f, 1.0f, 1.0f);
        }
        else {
            // Empty Slot - brown button darkened to indicate create action
            DrawColoredQuad(0.0f, yPos - 0.005f, buttonW + 0.01f, buttonH + 0.01f,
                0.0f, 0.0f, 0.0f, 0.3f);

            DrawTexturedQuad(pTexButtonLong, pMeshButtonLong,
                0.0f, yPos, buttonW, buttonH,
                0.6f, 0.5f, 0.4f, 1.0f);

            if (fontId >= 0) {
                AEGfxPrint(fontId, "+ NEW PROFILE",
                    -0.13f, yPos - 0.015f,
                    0.7f, 0.95f, 0.88f, 0.75f, 1.0f);
            }
        }
    }

    // Back instruction - centered at bottom
    if (fontId >= 0) {
        AEGfxPrint(fontId, "Press Esc to go back", -0.22f, -0.85f, 0.7f, 0.7f, 0.7f, 0.7f, 1.0f);
    }

    // --- Popup overlay ---
    if (popupActive) {
        // Dim background
        DrawColoredQuad(0.0f, 0.0f, 2.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.6f);

        // Panel size and position
        float panelW = PixelsToNDC_X(520.0f);
        float panelH = PixelsToNDC_Y(300.0f);

        // Panel background (brown panel texture)
        DrawTexturedQuad(pTexPanel, pMeshButtonLong,
            0.0f, 0.05f, panelW, panelH,
            1.0f, 1.0f, 1.0f, 1.0f);

        if (fontId >= 0) {
            // Reset render state before text so prior DrawTexturedQuad/DrawColoredQuad
            // calls don't leave a stale color multiplier that hides the text
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            // Title inside panel
            AEGfxPrint(fontId, "ENTER PROFILE NAME",
                -0.22f, 0.22f,
                0.75f, 1.0f, 0.9f, 0.6f, 1.0f);

            // Input field outline rectangle
            float inputW = PixelsToNDC_X(360.0f);
            float inputH = PixelsToNDC_Y(55.0f);
            DrawTexturedQuad(pTexInputRect, pMeshButtonLong,
                0.0f, 0.04f, inputW, inputH,
                1.0f, 1.0f, 1.0f, 1.0f);

            // Reset again after DrawTexturedQuad before printing typed text
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            // Typed text inside the input box (with blinking cursor bar)
            char displayText[PROFILE_NAME_MAX_LEN + 2];
            sprintf_s(displayText, sizeof(displayText), "%s|", popupInputBuf);
            AEGfxPrint(fontId, displayText,
                -0.20f, 0.025f,
                0.75f, 1.0f, 1.0f, 1.0f, 1.0f);

            // Reset before hint text too
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            // Hint text below input
            AEGfxPrint(fontId, "Press ENTER to confirm, ESC to cancel",
                -0.35f, -0.11f,
                0.45f, 0.75f, 0.75f, 0.75f, 1.0f);
        }
    }

    // Transition overlay
    if (TR_IsActive()) {
        float alpha = TR_GetAlpha();

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(0, 0, 0, alpha);

        AEMtx33 trans, scale, transform;
        // Fullscreen quad covers NDC space [-1,1], so scale = screen pixel size
        AEMtx33Scale(&scale, SCREEN_WIDTH, SCREEN_HEIGHT);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
    }
}

void ProfileScreen_Free() {
    // Free ProfileScreen-specific resources
    if (pMeshNextObject) {
        AEGfxMeshFree(pMeshNextObject);
        pMeshNextObject = NULL;
    }
    if (pMeshButtonLong) {
        AEGfxMeshFree(pMeshButtonLong);
        pMeshButtonLong = NULL;
    }
    if (pMeshButtonSquare) {
        AEGfxMeshFree(pMeshButtonSquare);
        pMeshButtonSquare = NULL;
    }
}

void ProfileScreen_Unload() {
    // Unload textures
    if (pTexButtonLong) {
        AEGfxTextureUnload(pTexButtonLong);
        pTexButtonLong = NULL;
    }
    if (pTexButtonLongPressed) {
        AEGfxTextureUnload(pTexButtonLongPressed);
        pTexButtonLongPressed = NULL;
    }
    if (pTexButtonSquare) {
        AEGfxTextureUnload(pTexButtonSquare);
        pTexButtonSquare = NULL;
    }
    if (pTexInputRect) {
        AEGfxTextureUnload(pTexInputRect);
        pTexInputRect = NULL;
    }
    if (pTexCrossIcon) {
        AEGfxTextureUnload(pTexCrossIcon);
        pTexCrossIcon = NULL;
    }
    if (pTexPanel) {
        AEGfxTextureUnload(pTexPanel);
        pTexPanel = NULL;
    }
}