#include "StartScreen.h"
#include "Profile.h"
#include "AEEngine.h"
#include "Utilities.h"
#include "GameStateManager.h"
#include <fstream>
#include <cstdio>
#include <cstring>

// ============================================================
// Profile persistence  (mirrored from Profile.cpp so the
// start screen can create a new slot without entering the
// full ProfileScreen first)
// ============================================================
static constexpr int   MAX_PROFILES = 3;
static constexpr int   PROFILE_NAME_MAX = 32;
static const char* PROFILES_FILE = "profiles.txt";

struct SSProfile   // "SS" prefix avoids clash with Profile.cpp's own Profile struct
{
    bool exists = false;
    char name[PROFILE_NAME_MAX] = "";
    int  level = 0;
    int  score = 0;
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
enum ButtonID { BUTTON_CONTINUE = 0, BUTTON_NEW_GAME = 1, BUTTON_EXIT = 2 };

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
static Button exitButton;

// ============================================================
// State
// ============================================================
static bool  hasSave = false;
static f32   logoPosX = -520.0f;
static f32   logoPosY = 260.0f;

float gScaleX;
float gScaleY;

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
static bool  popupShowFull = false;  // display "profiles full" message
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

static s8 ssFont = -1;  // start-screen's own font

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
    AEMtx33Trans(&trans, x - slideOffset, y);
    AEMtx33Scale(&scale, w * gScaleX, h * gScaleY);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMeshButton, AE_GFX_MDM_TRIANGLES);
}

static bool IsClicked(Button& btn, float w, float h)
{
    return ClickedOnRect(btn.x, btn.y, w * gScaleX, h * gScaleY);
}
static bool IsHoveredBtn(Button& btn, float w, float h)
{
    return IsMouseOverRect(btn.x, btn.y, w * gScaleX, h * gScaleY);
}

static constexpr float kContinueW = 190.0f, kContinueH = 41.0f;
static constexpr float kNewGameW = 234.0f, kNewGameH = 35.0f;
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
        // All 3 slots full
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

void StartScreen_Init()
{
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

    // Load font for popup text (independent of Main.cpp's fontId)
    ssFont = AEGfxCreateFont("Assets/liberation-mono.ttf", 24);
    if (ssFont < 0)
    {
        ssFont = AEGfxCreateFont("Assets/Crayon pastel.otf", 24);
        if (ssFont < 0)
            OutputDebugStringA("StartScreen: WARNING - failed to load popup font.\\n");
    }

    // Button column: left-aligned with logo left edge, 70 px apart vertically
    const float btnX = logoPosX - 165.0f;
    const float btnTopY = 60.0f;
    const float btnStep = -70.0f;

    continueButton.ID = BUTTON_CONTINUE;
    continueButton.normal = AEGfxTextureLoad("Assets/StartScreen_Continue.png");
    continueButton.hover = AEGfxTextureLoad("Assets/StartScreen_Continue_Selected.png");
    continueButton.disabled = AEGfxTextureLoad("Assets/StartScreen_Continue.png");
    continueButton.x = btnX;    continueButton.y = btnTopY;
    continueButton.x_sel = btnX;    continueButton.y_sel = btnTopY;

    newGameButton.ID = BUTTON_NEW_GAME;
    newGameButton.normal = AEGfxTextureLoad("Assets/StartScreen_NewGameButton.png");
    newGameButton.hover = AEGfxTextureLoad("Assets/StartScreen_NewGameButton_Selected.png");
    newGameButton.x = btnX;    newGameButton.y = btnTopY + btnStep;
    newGameButton.x_sel = btnX;    newGameButton.y_sel = btnTopY + btnStep;

    exitButton.ID = BUTTON_EXIT;
    exitButton.normal = AEGfxTextureLoad("Assets/StartScreen_Exit.png");
    exitButton.hover = AEGfxTextureLoad("Assets/StartScreen_Exit_Selected.png");
    exitButton.x = btnX;    exitButton.y = btnTopY + btnStep * 2.0f;
    exitButton.x_sel = btnX;    exitButton.y_sel = btnTopY + btnStep * 2.0f;
}

// ============================================================
void StartScreen_Update(float dt)
{
    continueButton.hovered = false;
    newGameButton.hovered = false;
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
        return; // block all other input while popup is open
    }

    // ----------------------------------------------------------
    // Normal menu input
    // ----------------------------------------------------------
    if (!isExiting)
    {
        // Tick "full" toast if showing outside popup
        if (popupShowFull)
        {
            popupFullTimer -= dt;
            if (popupFullTimer <= 0.0f) popupShowFull = false;
        }

        // Hover
        if (hasSave && IsHoveredBtn(continueButton, kContinueW, kContinueH))
            continueButton.hovered = true;
        else if (IsHoveredBtn(newGameButton, kNewGameW, kNewGameH))
            newGameButton.hovered = true;
        else if (IsHoveredBtn(exitButton, kExitW, kExitH))
            exitButton.hovered = true;

        // Continue → ProfileScreen to select which existing profile to load
        if (hasSave && IsClicked(continueButton, kContinueW, kContinueH))
        {
            isExiting = true;
            nextState = GS_NEXT_SCREEN;
        }
        // New Game → open inline name-entry popup (or show "full" toast)
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

    // --- Buttons (Continue greyed out when no save) ---
    RenderButton(continueButton, kContinueW, kContinueH, slideOffset, !hasSave);
    RenderButton(newGameButton, kNewGameW, kNewGameH, slideOffset);
    RenderButton(exitButton, kExitW, kExitH, slideOffset);

    // --- "Profiles full" toast (shown outside popup) ---
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
        // Dim overlay (full-screen colour quad)
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
            // Always reset render state before AEGfxPrint calls
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            // Popup title
            AEGfxPrint(ssFont, "ENTER YOUR NAME",
                -0.1f, 0.15f, 0.75f, 1.0f, 0.9f, 0.6f, 1.0f);

            // Typed text with blinking cursor bar
            char display[PROFILE_NAME_MAX + 2];
            sprintf_s(display, sizeof(display), "%s|", popupBuf);
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxPrint(ssFont, display,
                -0.17f, 0.01f, 0.75f, 0.05f, 0.05f, 0.05f, 1.0f);

            // Hint line
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxPrint(ssFont, "ENTER to confirm    ESC to cancel",
                -0.14f, -0.12f, 0.45f, 0.75f, 0.75f, 0.75f, 1.0f);

            // "Profiles full" warning inside popup (edge case)
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