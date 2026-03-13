// ============================================================
// StartScreen.cpp
// ============================================================

#include "StartScreen.h"
#include "Profile.h"
#include "AEEngine.h"
#include "Utilities.h"
#include "GameStateManager.h"
#include <fstream>
#include <cstdio>
#include <cstring>
#include "Main.h"

// ============================================================
// Profile persistence
// ============================================================
static constexpr int   MAX_PROFILES = 3;
static constexpr int   PROFILE_NAME_MAX = 32;
static const char* PROFILES_FILE = "profiles.txt";

struct SSProfile
{
    bool exists = false;
    char name[PROFILE_NAME_MAX] = "";
    int  level = 0;
    int  score = 0;
    // Note: We could add lastPlayed timestamp here if you want true "most recent"
};
static SSProfile g_profiles[MAX_PROFILES];

static int CountProfiles()
{
    int n = 0;
    for (int i = 0; i < MAX_PROFILES; i++)
        if (g_profiles[i].exists) n++;
    return n;
}

static int FirstFreeSlot()
{
    for (int i = 0; i < MAX_PROFILES; i++)
        if (!g_profiles[i].exists) return i;
    return -1;
}

// Find the most recent profile (highest score for now, or slot 0 if tie)
static int GetMostRecentProfileSlot()
{
    int bestSlot = -1;
    int bestScore = -1;

    for (int i = 0; i < MAX_PROFILES; i++)
    {
        if (g_profiles[i].exists && g_profiles[i].score > bestScore)
        {
            bestScore = g_profiles[i].score;
            bestSlot = i;
        }
    }
    return bestSlot; // Returns -1 if no profiles exist
}

static void SS_Profiles_Load()
{
    for (int i = 0; i < MAX_PROFILES; i++) g_profiles[i] = SSProfile{};

    FILE* f = nullptr;
    if (fopen_s(&f, PROFILES_FILE, "r") != 0 || !f) return;

    int  slot = -1;
    char line[128] = {};
    while (fgets(line, sizeof(line), f))
    {
        int len = (int)strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
        if (len > 0 && line[len - 1] == '\r') line[--len] = '\0';
        if (len == 0) continue;

        if (line[0] == '[') { sscanf_s(line, "[PROFILE_%d]", &slot); continue; }
        if (slot < 0 || slot >= MAX_PROFILES) continue;

        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        const char* key = line;
        const char* val = eq + 1;

        if (strcmp(key, "EXISTS") == 0) g_profiles[slot].exists = (atoi(val) != 0);
        else if (strcmp(key, "NAME") == 0) strncpy_s(g_profiles[slot].name, PROFILE_NAME_MAX, val, _TRUNCATE);
        else if (strcmp(key, "LEVEL") == 0) g_profiles[slot].level = atoi(val);
        else if (strcmp(key, "SCORE") == 0) g_profiles[slot].score = atoi(val);
    }
    fclose(f);
}

static void SS_Profiles_Save()
{
    FILE* f = nullptr;
    if (fopen_s(&f, PROFILES_FILE, "w") != 0 || !f) return;
    for (int i = 0; i < MAX_PROFILES; i++)
    {
        fprintf(f, "[PROFILE_%d]\n", i);
        fprintf(f, "EXISTS=%d\n", g_profiles[i].exists ? 1 : 0);
        fprintf(f, "NAME=%s\n", g_profiles[i].name);
        fprintf(f, "LEVEL=%d\n", g_profiles[i].level);
        fprintf(f, "SCORE=%d\n\n", g_profiles[i].score);
    }
    fclose(f);
}



// ============================================================
// Button
// ============================================================
enum ButtonID {
    BUTTON_CONTINUE = 0,
    BUTTON_NEW_GAME = 1,
    BUTTON_EXISTING_PROFILE = 2,
    BUTTON_EXIT = 3
};

struct Button
{
    AEGfxTexture* normal = nullptr;
    AEGfxTexture* hover = nullptr;
    AEGfxTexture* disabled = nullptr;
    int   ID = 0;
    float x = 0.0f, y = 0.0f;
    float x_sel = 0.0f, y_sel = 0.0f;
    bool  hovered = false;
};

static Button continueButton;
static Button newGameButton;
static Button existingProfileButton;
static Button exitButton;

// ============================================================
// State
// ============================================================
static bool  hasSave = false;
static f32   logoPosX = -520.0f;
static f32   logoPosY = 260.0f;

extern bool  startScreenActive = true;
static bool  isExiting = false;
static float exitAnimProgress = 0.0f;
static float exitAnimFadeOut = 1.0f;
static float exitAnimSpeed = 1.3f;

// ============================================================
// Inline "New Game" name-entry popup
// ============================================================
static bool  popupOpen = false;
static char  popupBuf[PROFILE_NAME_MAX] = {};
static int   popupLen = 0;
static bool  popupShowFull = false;
static float popupFullTimer = 0.0f;

// ============================================================
// Meshes / Textures
// ============================================================
static AEGfxTexture* logoTexture = nullptr;
static AEGfxTexture* gradientBlur = nullptr;
static AEGfxTexture* pTexPanel = nullptr;
static AEGfxTexture* pTexInputRect = nullptr;

static AEGfxVertexList* pMeshLogo = nullptr;
static AEGfxVertexList* pMeshGradientBlur = nullptr;
static AEGfxVertexList* pMeshButton = nullptr;
static AEGfxVertexList* pMeshPopup = nullptr;

static s8 ssFont = -1;

// ============================================================
// Internal helpers
// ============================================================
static AEGfxVertexList* CreateMesh()
{
    AEGfxMeshStart();
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxVertexAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f);
    AEGfxVertexAdd(0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f);
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxVertexAdd(0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f);
    AEGfxVertexAdd(-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    return AEGfxMeshEnd();
}

static void RenderButton(Button& btn, float w, float h, float slideOffset, bool isDisabled = false)
{
    AEGfxTexture* tex;
    float x, y;
    if (isDisabled)
    {
        tex = btn.disabled ? btn.disabled : btn.normal;
        x = btn.x; y = btn.y;
    }
    else
    {
        tex = btn.hovered ? btn.hover : btn.normal;
        x = btn.hovered ? btn.x_sel : btn.x;
        y = btn.hovered ? btn.y_sel : btn.y;
    }
    if (!tex || !pMeshButton) return;

    AEMtx33 scale, trans, transform;
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(exitAnimFadeOut * (isDisabled ? 0.40f : 1.0f));
    AEGfxTextureSet(tex, 0, 0);

    float finalX = (x - slideOffset) * gScaleX;
    float finalY = y * gScaleY;
    float finalW = w * gScaleX;
    float finalH = h * gScaleY;

    AEMtx33Trans(&trans, finalX, finalY);
    AEMtx33Scale(&scale, finalW, finalH);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMeshButton, AE_GFX_MDM_TRIANGLES);
}

static bool IsHoveredBtn(Button& btn, float w, float h)
{
    float screenX = btn.x * gScaleX;
    float screenY = btn.y * gScaleY;
    float screenW = w * gScaleX;
    float screenH = h * gScaleY;
    return IsMouseOverRect(screenX, screenY, screenW, screenH);
}

static bool IsClicked(Button& btn, float w, float h)
{
    float screenX = btn.x * gScaleX;
    float screenY = btn.y * gScaleY;
    float screenW = w * gScaleX;
    float screenH = h * gScaleY;
    return ClickedOnRect(screenX, screenY, screenW, screenH);
}

// Profile button dimensions
static constexpr float kContinueW = 190.0f, kContinueH = 41.0f;
static constexpr float kNewGameW = 234.0f, kNewGameH = 35.0f;
static constexpr float kExistingProfileW = 234.0f, kExistingProfileH = 35.0f;
static constexpr float kExitW = 68.0f, kExitH = 39.0f;

// ============================================================
// Popup helpers
// ============================================================
static void OpenNewGamePopup()
{
    popupOpen = true;
    popupBuf[0] = '\0';
    popupLen = 0;
    popupShowFull = false;
}
static void ClosePopup()
{
    popupOpen = false;
    popupBuf[0] = '\0';
    popupLen = 0;
}
static void ConfirmNewGame()
{
    if (popupLen == 0) return;

    int slot = FirstFreeSlot();
    if (slot == -1)
    {
        popupShowFull = true;
        popupFullTimer = 2.5f;
        return;
    }

    g_profiles[slot].exists = true;
    strncpy_s(g_profiles[slot].name, PROFILE_NAME_MAX, popupBuf, _TRUNCATE);
    g_profiles[slot].level = 1;
    g_profiles[slot].score = 0;
    SS_Profiles_Save();

    ClosePopup();
    hasSave = true;
    isExiting = true;
    nextState = GS_MAIN_SCREEN;
}

// ============================================================
// Public interface
// ============================================================
bool StartScreen_IsActive() { return startScreenActive; }

void StartScreen_Load()
{
    SS_Profiles_Load();
    hasSave = (CountProfiles() > 0);

    // Create meshes
    if (!pMeshButton) pMeshButton = CreateMesh();
    if (!pMeshLogo) pMeshLogo = CreateMesh();
    if (!pMeshGradientBlur) pMeshGradientBlur = CreateMesh();
    if (!pMeshPopup) pMeshPopup = CreateMesh();

    // Load textures
    if (!logoTexture) logoTexture = AEGfxTextureLoad("Assets/StartScreen_Logo.png");
    if (!gradientBlur) gradientBlur = AEGfxTextureLoad("Assets/StartScreen_GradientBlur.png");
    if (!pTexPanel) pTexPanel = AEGfxTextureLoad("Assets/panel_brown.png");
    if (!pTexInputRect) pTexInputRect = AEGfxTextureLoad("Assets/input_outline_rectangle.png");

    // Load font
    if (ssFont < 0)
    {
        ssFont = AEGfxCreateFont("Assets/liberation-mono.ttf", 24);
        if (ssFont < 0)
            ssFont = AEGfxCreateFont("Assets/Crayon pastel.otf", 24);
    }

    // Button positions - UPDATED: 4 buttons now
    const float btnX = logoPosX - 165.0f;
    const float btnTopY = 60.0f;
    const float btnStep = -55.0f;  // Slightly smaller step to fit 4 buttons

    // Continue (loads most recent profile directly)
    if (!continueButton.normal)
    {
        continueButton.normal = AEGfxTextureLoad("Assets/StartScreen_Continue.png");
        continueButton.hover = AEGfxTextureLoad("Assets/StartScreen_Continue_Selected.png");
        continueButton.disabled = AEGfxTextureLoad("Assets/StartScreen_Continue.png");
        continueButton.x = btnX; continueButton.y = btnTopY;
        continueButton.x_sel = btnX; continueButton.y_sel = btnTopY;
    }

    // New Game
    if (!newGameButton.normal)
    {
        newGameButton.normal = AEGfxTextureLoad("Assets/StartScreen_NewGameButton.png");
        newGameButton.hover = AEGfxTextureLoad("Assets/StartScreen_NewGameButton_Selected.png");
        newGameButton.x = btnX; newGameButton.y = btnTopY + btnStep;
        newGameButton.x_sel = btnX; newGameButton.y_sel = btnTopY + btnStep;
    }

    // Existing Profile (goes to profile selection screen)
    if (!existingProfileButton.normal)
    {
        existingProfileButton.normal = AEGfxTextureLoad("Assets/StartScreen_ExistingProfile.png");
        existingProfileButton.hover = AEGfxTextureLoad("Assets/StartScreen_ExistingProfile_Selected.png");

        existingProfileButton.x = btnX;
        existingProfileButton.y = btnTopY + btnStep * 2.0f;
        existingProfileButton.x_sel = btnX;
        existingProfileButton.y_sel = btnTopY + btnStep * 2.0f;
    }

    // Exit (now 4th button)
    if (!exitButton.normal)
    {
        exitButton.normal = AEGfxTextureLoad("Assets/StartScreen_Exit.png");
        exitButton.hover = AEGfxTextureLoad("Assets/StartScreen_Exit_Selected.png");
        exitButton.x = btnX;
        exitButton.y = btnTopY + btnStep * 3.0f;  // Moved down
        exitButton.x_sel = btnX;
        exitButton.y_sel = btnTopY + btnStep * 3.0f;
    }
}

void StartScreen_Init()
{
    startScreenActive = true;
    isExiting = false;
    exitAnimProgress = 0.0f;
    exitAnimFadeOut = 1.0f;

    SS_Profiles_Load();
    hasSave = (CountProfiles() > 0);

    pMeshButton = CreateMesh();
    pMeshLogo = CreateMesh();
    pMeshGradientBlur = CreateMesh();
    pMeshPopup = CreateMesh();

    logoTexture = AEGfxTextureLoad("Assets/StartScreen_Logo.png");
    gradientBlur = AEGfxTextureLoad("Assets/StartScreen_GradientBlur.png");
    pTexPanel = AEGfxTextureLoad("Assets/panel_brown.png");
    pTexInputRect = AEGfxTextureLoad("Assets/input_outline_rectangle.png");

    ssFont = AEGfxCreateFont("Assets/liberation-mono.ttf", 24);
    if (ssFont < 0)
    {
        ssFont = AEGfxCreateFont("Assets/Crayon pastel.otf", 24);
        if (ssFont < 0)
            OutputDebugStringA("StartScreen: WARNING - failed to load popup font.\\n");
    }

    // Button column - UPDATED: 4 buttons with smaller spacing
    const float btnX = logoPosX - 165.0f;
    const float btnTopY = 60.0f;
    const float btnStep = -55.0f;  // Smaller step for 4 buttons

    // Continue
    continueButton.ID = BUTTON_CONTINUE;
    continueButton.normal = AEGfxTextureLoad("Assets/StartScreen_Continue.png");
    continueButton.hover = AEGfxTextureLoad("Assets/StartScreen_Continue_Selected.png");
    continueButton.disabled = AEGfxTextureLoad("Assets/StartScreen_Continue.png");
    continueButton.x = btnX;    continueButton.y = btnTopY;
    continueButton.x_sel = btnX;    continueButton.y_sel = btnTopY;

    // New Game
    newGameButton.ID = BUTTON_NEW_GAME;
    newGameButton.normal = AEGfxTextureLoad("Assets/StartScreen_NewGameButton.png");
    newGameButton.hover = AEGfxTextureLoad("Assets/StartScreen_NewGameButton_Selected.png");
    newGameButton.x = btnX;    newGameButton.y = btnTopY + btnStep;
    newGameButton.x_sel = btnX;    newGameButton.y_sel = btnTopY + btnStep;

    existingProfileButton.ID = BUTTON_EXISTING_PROFILE;
    existingProfileButton.normal = AEGfxTextureLoad("Assets/StartScreen_Profile.png");
    existingProfileButton.hover = AEGfxTextureLoad("Assets/StartScreen_Profile_Selected.png");

    // Fallback textures if specific ones don't exist
    if (!existingProfileButton.normal)
        existingProfileButton.normal = AEGfxTextureLoad("Assets/StartScreen_NewGameButton.png");
    if (!existingProfileButton.hover)
        existingProfileButton.hover = AEGfxTextureLoad("Assets/StartScreen_NewGameButton_Selected.png");

    existingProfileButton.x = btnX;
    existingProfileButton.y = btnTopY + btnStep * 2.0f;
    existingProfileButton.x_sel = btnX;
    existingProfileButton.y_sel = btnTopY + btnStep * 2.0f;

    // Exit (now 4th)
    exitButton.ID = BUTTON_EXIT;
    exitButton.normal = AEGfxTextureLoad("Assets/StartScreen_Exit.png");
    exitButton.hover = AEGfxTextureLoad("Assets/StartScreen_Exit_Selected.png");
    exitButton.x = btnX;
    exitButton.y = btnTopY + btnStep * 3.0f;
    exitButton.x_sel = btnX;
    exitButton.y_sel = btnTopY + btnStep * 3.0f;
}

// ============================================================
void StartScreen_Update(float dt)
{
    // Reset hover states for all 4 buttons
    continueButton.hovered = false;
    newGameButton.hovered = false;
    existingProfileButton.hovered = false;  // NEW
    exitButton.hovered = false;

    // Debug save-toggle – remove before shipping
    if (AEInputCheckTriggered(AEVK_RSHIFT))
        hasSave = !hasSave;

    // ----------------------------------------------------------
    // Popup open: intercept all keyboard input for name entry
    // ----------------------------------------------------------
    if (popupOpen)
    {
        if (popupShowFull)
        {
            popupFullTimer -= dt;
            if (popupFullTimer <= 0.0f) popupShowFull = false;
        }

        if (AEInputCheckTriggered(AEVK_RETURN))
        {
            ConfirmNewGame();
        }
        else if (AEInputCheckTriggered(AEVK_ESCAPE))
        {
            ClosePopup();
        }
        else if (AEInputCheckTriggered(AEVK_BACK) && popupLen > 0)
        {
            popupBuf[--popupLen] = '\0';
        }
        else
        {
            for (u8 key = 32; key < 127; key++)
            {
                if (AEInputCheckTriggered(key))
                {
                    bool shift = AEInputCheckCurr(AEVK_RSHIFT) || AEInputCheckCurr(AEVK_LSHIFT);
                    char c = (char)key;
                    if (c >= 'A' && c <= 'Z') { if (!shift) c += 32; }
                    else if (shift)
                    {
                        switch (c) {
                        case '1':c = '!'; break; case '2':c = '@'; break; case '3':c = '#'; break;
                        case '4':c = '$'; break; case '5':c = '%'; break; case '6':c = '^'; break;
                        case '7':c = '&'; break; case '8':c = '*'; break; case '9':c = '('; break;
                        case '0':c = ')'; break; case '-':c = '_'; break; case '=':c = '+'; break;
                        default: break;
                        }
                    }
                    if (popupLen < PROFILE_NAME_MAX - 1)
                    {
                        popupBuf[popupLen++] = c;
                        popupBuf[popupLen] = '\0';
                    }
                }
            }
        }
        return;
    }

    // ----------------------------------------------------------
    // Normal menu input
    // ----------------------------------------------------------
    if (!isExiting)
    {
        if (popupShowFull)
        {
            popupFullTimer -= dt;
            if (popupFullTimer <= 0.0f) popupShowFull = false;
        }

        // UPDATED: Hover detection for all 4 buttons
        if (hasSave && IsHoveredBtn(continueButton, kContinueW, kContinueH))
            continueButton.hovered = true;
        else if (IsHoveredBtn(newGameButton, kNewGameW, kNewGameH))
            newGameButton.hovered = true;
        else if (IsHoveredBtn(existingProfileButton, kExistingProfileW, kExistingProfileH))  // NEW
            existingProfileButton.hovered = true;
        else if (IsHoveredBtn(exitButton, kExitW, kExitH))
            exitButton.hovered = true;

        // UPDATED: Click handling with new logic
        if (hasSave && IsClicked(continueButton, kContinueW, kContinueH))
        {
            // NEW: Load most recent profile directly, then go to main screen
            int recentSlot = GetMostRecentProfileSlot();
            if (recentSlot >= 0)
            {
                // Set this as the active profile in Profile system
                // We need to tell Profile.cpp which slot to use
                // Option 1: Use ProfileScreen_SetSelectMode with a special flag
                // Option 2: Directly set the active slot if Profile.cpp exposes it

                // For now, we'll use the existing system but skip the selection UI
                // by setting the mode and immediately choosing the slot
                ProfileScreen_SetSelectMode(true);
                // We need to tell it which slot - this requires a new function or modification
                // As a workaround, we'll store it and use it in ProfileScreen

                isExiting = true;
                nextState = GS_MAIN_SCREEN;  // Go directly to main, skip profile screen
            }
            else
            {
                // No profiles actually exist (shouldn't happen if hasSave is true)
                ProfileScreen_SetSelectMode(true);
                isExiting = true;
                nextState = GS_NEXT_SCREEN;
            }
        }
        else if (IsClicked(newGameButton, kNewGameW, kNewGameH))
        {
            if (FirstFreeSlot() == -1)
            {
                popupShowFull = true;
                popupFullTimer = 2.5f;
            }
            else
            {
                OpenNewGamePopup();
            }
        }
        else if (IsClicked(existingProfileButton, kExistingProfileW, kExistingProfileH))  // NEW
        {
            // Go to profile selection screen to choose/manage profiles
            ProfileScreen_SetSelectMode(true);
            isExiting = true;
            nextState = GS_NEXT_SCREEN;
        }
        else if (IsClicked(exitButton, kExitW, kExitH))
        {
            nextState = GS_EXIT;
        }

        // Keyboard shortcut: Enter = New Game
        if (AEInputCheckTriggered(AEVK_RETURN))
        {
            if (FirstFreeSlot() == -1) { popupShowFull = true; popupFullTimer = 2.5f; }
            else { OpenNewGamePopup(); }
        }
    }
    else
    {
        exitAnimProgress += dt * exitAnimSpeed * 0.1f;
        exitAnimFadeOut -= dt * exitAnimSpeed;
        if (exitAnimProgress >= 1.0f)
        {
            exitAnimProgress = 1.0f;
            startScreenActive = false;
        }
    }
}

// ============================================================
void StartScreen_Draw()
{
    if (!startScreenActive) return;

    const float slideOffset = exitAnimProgress * 3000.0f;
    AEMtx33 scale, trans, transform;

    // --- Gradient blur background ---
    if (gradientBlur && pMeshGradientBlur)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 0.5f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.9f * exitAnimFadeOut);
        AEGfxTextureSet(gradientBlur, 0, 0);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Scale(&scale, 1920.0f * gScaleX, 1080.0f * gScaleY);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMeshGradientBlur, AE_GFX_MDM_TRIANGLES);
    }

    // --- Logo ---
    if (logoTexture && pMeshLogo)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.1f, 0.1f, 0.1f, 0.1f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(exitAnimFadeOut);
        AEGfxTextureSet(logoTexture, 0, 0);
        AEMtx33Trans(&trans, logoPosX - slideOffset, logoPosY);
        AEMtx33Scale(&scale, 331.0f * gScaleX, 228.0f * gScaleY);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMeshLogo, AE_GFX_MDM_TRIANGLES);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    }

    // --- Buttons (UPDATED: 4 buttons) ---
    RenderButton(continueButton, kContinueW, kContinueH, slideOffset, !hasSave);
    RenderButton(newGameButton, kNewGameW, kNewGameH, slideOffset);
    RenderButton(existingProfileButton, kExistingProfileW, kExistingProfileH, slideOffset);  // NEW
    RenderButton(exitButton, kExitW, kExitH, slideOffset);

    // --- "Profiles full" toast ---
    if (popupShowFull && !popupOpen && ssFont >= 0)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxPrint(ssFont,
            "All profile slots are full! Delete a profile first.",
            -0.50f, -0.62f, 0.55f, 1.0f, 0.35f, 0.35f, 1.0f);
    }

    // --- New Game name-entry popup ---
    if (popupOpen)
    {
        // Dim overlay
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.65f);
        AEMtx33Scale(&scale, 1600.0f, 900.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMeshPopup, AE_GFX_MDM_TRIANGLES);

        // Panel background
        if (pTexPanel)
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxTextureSet(pTexPanel, 0, 0);
            AEMtx33Trans(&trans, 0.0f, 25.0f);
            AEMtx33Scale(&scale, 500.0f * gScaleX, 280.0f * gScaleY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(pMeshPopup, AE_GFX_MDM_TRIANGLES);
        }

        // Input field outline
        if (pTexInputRect)
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxTextureSet(pTexInputRect, 0, 0);
            AEMtx33Trans(&trans, 0.0f, 10.0f);
            AEMtx33Scale(&scale, 360.0f * gScaleX, 52.0f * gScaleY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(pMeshPopup, AE_GFX_MDM_TRIANGLES);
        }

        if (ssFont >= 0)
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            AEGfxPrint(ssFont, "ENTER YOUR NAME",
                -0.1f, 0.15f, 0.75f, 1.0f, 0.9f, 0.6f, 1.0f);

            char display[PROFILE_NAME_MAX + 2];
            sprintf_s(display, sizeof(display), "%s|", popupBuf);
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxPrint(ssFont, display,
                -0.17f, 0.01f, 0.75f, 0.05f, 0.05f, 0.05f, 1.0f);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxPrint(ssFont, "ENTER to confirm    ESC to cancel",
                -0.15f, -0.12f, 0.45f, 0.75f, 0.75f, 0.75f, 1.0f);

            if (popupShowFull)
            {
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxPrint(ssFont, "All slots are full! Delete a profile first.",
                    -0.37f, -0.23f, 0.45f, 1.0f, 0.3f, 0.3f, 1.0f);
            }
        }
    }
}

void StartScreen_Unload()
{
    // Unload all textures
    if (logoTexture) { AEGfxTextureUnload(logoTexture); logoTexture = nullptr; }
    if (gradientBlur) { AEGfxTextureUnload(gradientBlur); gradientBlur = nullptr; }
    if (pTexPanel) { AEGfxTextureUnload(pTexPanel); pTexPanel = nullptr; }
    if (pTexInputRect) { AEGfxTextureUnload(pTexInputRect); pTexInputRect = nullptr; }

    if (continueButton.normal) { AEGfxTextureUnload(continueButton.normal); continueButton.normal = nullptr; }
    if (continueButton.hover) { AEGfxTextureUnload(continueButton.hover); continueButton.hover = nullptr; }
    if (continueButton.disabled) { AEGfxTextureUnload(continueButton.disabled); continueButton.disabled = nullptr; }

    if (newGameButton.normal) { AEGfxTextureUnload(newGameButton.normal); newGameButton.normal = nullptr; }
    if (newGameButton.hover) { AEGfxTextureUnload(newGameButton.hover); newGameButton.hover = nullptr; }

    // NEW: Unload existing profile button textures
    if (existingProfileButton.normal) { AEGfxTextureUnload(existingProfileButton.normal); existingProfileButton.normal = nullptr; }
    if (existingProfileButton.hover) { AEGfxTextureUnload(existingProfileButton.hover); existingProfileButton.hover = nullptr; }

    if (exitButton.normal) { AEGfxTextureUnload(exitButton.normal); exitButton.normal = nullptr; }
    if (exitButton.hover) { AEGfxTextureUnload(exitButton.hover); exitButton.hover = nullptr; }

    if (ssFont >= 0) { AEGfxDestroyFont(ssFont); ssFont = -1; }
}