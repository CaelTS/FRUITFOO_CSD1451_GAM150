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
static constexpr int   PROFILE_NAME_MAX = 32; //name limit
static const char* PROFILES_FILE = "profiles.txt";

static AEGfxVertexList* pMeshPopup = nullptr;
static AEGfxTexture* pTexInputRect = nullptr;

static s8 ssFont = -1;

struct SSProfile
{
    bool exists = false;
    char name[PROFILE_NAME_MAX] = "";
    int  coins = 0;
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

// Find the most recent profile 
static int GetMostRecentProfileSlot()
{
    for (int i = 0; i < MAX_PROFILES; i++)
        if (g_profiles[i].exists) return i;
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
        else if (strcmp(key, "coins") == 0) g_profiles[slot].coins = atoi(val);
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
        fprintf(f, "coins=%d\n\n", g_profiles[i].coins);
    }
    fclose(f);
}

// ============================================================
// Inline "New Game" name-entry popup
// ============================================================
static bool  popupOpen = false;
static char  popupBuf[PROFILE_NAME_MAX] = {};
static int   popupLen = 0;
static bool  popupShowFull = false;
static float popupFullTimer = 0.0f;

static AEGfxTexture* pTexPanel = nullptr;


enum ButtonID
{
    BUTTON_NEW_GAME = 0,
    BUTTON_CONTINUE,
    BUTTON_SETTINGS,
    BUTTON_EXIT
};

struct Button
{
    AEGfxTexture* normal = nullptr;
    AEGfxTexture* hover = nullptr;

    int ID = 0; // 0: New Game, 1: Continue, 2: Settings, 3: Exit

    float x = 0.0f;
    float y = 0.0f;

    float x_selected = 0.0f;
    float y_selected = 0.0f;

    float x_save = 0.0f;
    float y_save = 0.0f;

    float x_selected_save = 0.0f;
    float y_selected_save = 0.0f;

    bool hovered = false;
};


//Buttons
Button newGameButton;
Button continueButton;
Button profileButton;
Button exitButton;

static bool hasSave = false; //placeholder until we implement profile system
static f32 logoPosX = -520.0;
static f32 logoPosY = 260.0;

extern float gScaleX;
extern float gScaleY;

//animation variable
extern bool startScreenActive = true;   // Is the start screen still active?
static bool isExiting = false;          // Has the exit animation started?
static float exitAnimProgress = 0.0f;   // 0.0 -> 1.0 animation progress
static float exitAnimFadeOut = 1.0f;   // 0.0 -> 1.0 fade out progress
static float exitAnimSpeed = 1.3f;      // Speed of slide animation

//Local helper function button click detection
bool IsButtonClicked(Button& btn, float width, float height) {
    float x = hasSave ? btn.x_save : btn.x;
    float y = hasSave ? btn.y_save : btn.y;

    if (ClickedOnRect(x, y, width * gScaleX, height * gScaleY))
        return true;
    else return false;
}

bool IsButtonHovered(Button& btn, float width, float height) {
    float x = btn.x;
    float y = btn.y;

    if (hasSave) {
        x = btn.x_save;
        y = btn.y_save;
    }

    return IsMouseOverRect(x, y, width * gScaleX, height * gScaleY);
}

//ButtonID ID (float x, float y) {
//    if (!newGameButton.hovered && IsButtonHovered(newGameButton, 234.0f, 35.0f)) return BUTTON_NEW_GAME;
//    if (!continueButton.hovered && IsButtonHovered(continueButton, 300.0f, 80.0f)) return BUTTON_CONTINUE;
//    if (!settingsButton.hovered && IsButtonHovered(settingsButton, 300.0f, 80.0f)) return BUTTON_SETTINGS;
//    if (!exitButton.hovered && IsButtonHovered(exitButton, 68.0f, 39.0f)) return BUTTON_EXIT;
//}

//Textures
AEGfxTexture* logoTexture = nullptr;
AEGfxTexture* gradientBlur = nullptr;


// Meshes
AEGfxVertexList* pMeshLogo = nullptr;
AEGfxVertexList* pMeshNewGameButton = nullptr;
AEGfxVertexList* pMeshNewGameButton_Selected = nullptr;
AEGfxVertexList* pMeshContinueButton = nullptr;
AEGfxVertexList* pMeshContinueButton_Selected = nullptr;
AEGfxVertexList* pMeshProfileButton = nullptr;
AEGfxVertexList* pMeshProfileButton_Selected = nullptr;
AEGfxVertexList* pMeshExitButton = nullptr;
AEGfxVertexList* pMeshExitButton_Selected = nullptr;

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
    g_profiles[slot].coins = 0;
    SS_Profiles_Save();

    ClosePopup();
    hasSave = true;
    isExiting = true;
    nextState = GS_MAIN_SCREEN;
}
AEGfxVertexList* pMeshGradientBlur = nullptr;


// ------------------------------------------------------------
// Helper
// ------------------------------------------------------------

static bool CheckSaveExists()
{
    std::ifstream file("save.dat");

    if (file.good())
        return true;

    return false;
}

void DrawButton(Button& btn, AEGfxVertexList* mesh, f32 width, f32 height, float offset)
{
    AEMtx33 scale, trans, transform;

    AEGfxTexture* tex = btn.hovered ? btn.hover : btn.normal;

    float x = btn.hovered ? btn.x_selected : btn.x;
    float y = btn.hovered ? btn.y_selected : btn.y;



    if (hasSave)
    {

        x = btn.hovered ? btn.x_selected_save : btn.x_save;
        y = btn.hovered ? btn.y_selected_save : btn.y_save;

    }



    if (!tex) return;

    AEGfxTextureSet(tex, 0, 0);
    AEGfxSetTransparency(1.0f * exitAnimFadeOut);

    AEMtx33Trans(&trans, x - offset, y);
    AEMtx33Scale(&scale, width * gScaleX, height * gScaleY);
    AEMtx33Concat(&transform, &trans, &scale);

    AEGfxSetTransform(transform.m);

    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

AEGfxVertexList* createMesh()
{
    AEGfxMeshStart();
    // bottom-left, bottom-right, top-right
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxVertexAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f);
    AEGfxVertexAdd(0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f);

    // bottom-left, top-right, top-left
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxVertexAdd(0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f);
    AEGfxVertexAdd(-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    return AEGfxMeshEnd();
}

bool StartScreen_IsActive()
{
    return startScreenActive; // from your start screen cpp
}

void StartScreen_Init()
{
    SS_Profiles_Load();
    hasSave = (CountProfiles() > 0);
    pMeshPopup = createMesh();

    ssFont = AEGfxCreateFont("Assets/liberation-mono.ttf", 24);
    if (ssFont < 0)
    {
        ssFont = AEGfxCreateFont("Assets/Crayon pastel.otf", 24);
        if (ssFont < 0)
            OutputDebugStringA("StartScreen: WARNING - failed to load popup font.\\n");
    }
    if (!pTexPanel) pTexPanel = AEGfxTextureLoad("Assets/panel_brown.png");
    if (!pTexInputRect) pTexInputRect = AEGfxTextureLoad("Assets/input_outline_rectangle.png");
    // Check if save file exists to determine if "Continue" button should be shown
    // Should be implemented in Profile.cpp, but for now we can just check if the file exists here
    //hasSave = SaveSystem_HasSaveFile();
    hasSave = false;

    logoTexture = AEGfxTextureLoad("Assets/StartScreen_Logo.png");
    gradientBlur = AEGfxTextureLoad("Assets/StartScreen_GradientBlur.png");
    pMeshLogo = createMesh();
    pMeshGradientBlur = createMesh();
    pMeshNewGameButton = createMesh();
    pMeshNewGameButton_Selected = createMesh();
    pMeshContinueButton = createMesh();
    pMeshContinueButton_Selected = createMesh();
    pMeshProfileButton = createMesh();
    pMeshProfileButton_Selected = createMesh();
    pMeshExitButton = createMesh();
    pMeshExitButton_Selected = createMesh();

    // Initialize "Exit" button (always shown)
    exitButton.normal = AEGfxTextureLoad("Assets/StartScreen_Exit.png");
    exitButton.hover = AEGfxTextureLoad("Assets/StartScreen_Exit_Selected.png");
    exitButton.x = logoPosX - 102.0f;
    exitButton.y = 45.0f;
    exitButton.x_selected = exitButton.x; // slide left on hover
    exitButton.y_selected = exitButton.y;
    exitButton.x_save = exitButton.x; // no slide when save exists
    exitButton.y_save = exitButton.y - (100.0f - 45.0f);
    exitButton.x_selected_save = exitButton.x - 1; // no slide when save exists
    exitButton.y_selected_save = exitButton.y_save;

    // Initialize "Continue" button 
    continueButton.normal = AEGfxTextureLoad("Assets/StartScreen_Continue.png");
    continueButton.hover = AEGfxTextureLoad("Assets/StartScreen_Continue_Selected.png");
    continueButton.x = logoPosX - 50.0f;
    continueButton.y = 100.0f;
    continueButton.x_selected = continueButton.x; // slide left on hover
    continueButton.y_selected = continueButton.y;
    continueButton.x_save = continueButton.x; // no slide when save exists
    continueButton.y_save = continueButton.y;
    continueButton.x_selected_save = continueButton.x; // no slide when save exists
    continueButton.y_selected_save = continueButton.y;

    // Initialize "Profile" button
    profileButton.normal = AEGfxTextureLoad("Assets/StartScreen_Profile.png");
    profileButton.hover = AEGfxTextureLoad("Assets/StartScreen_Profile_Selected.png");
    profileButton.x = logoPosX - 72.0f;
    profileButton.y = 45.0f;
    profileButton.x_selected = profileButton.x; // slide left on hover
    profileButton.y_selected = profileButton.y;
    profileButton.x_save = profileButton.x; // no slide when save exists    
    profileButton.y_save = profileButton.y;
    profileButton.x_selected_save = profileButton.x; // no slide when save exists   
    profileButton.y_selected_save = profileButton.y;

    // Initialize "New Game" button
    newGameButton.normal = AEGfxTextureLoad("Assets/StartScreen_NewGameButton.png");
    newGameButton.hover = AEGfxTextureLoad("Assets/StartScreen_NewGameButton_Selected.png");
    newGameButton.x = logoPosX - 32.0f;
    newGameButton.y = 100.0f;
    newGameButton.x_selected = newGameButton.x; // slide left on hover
    newGameButton.y_selected = newGameButton.y;


}

void StartScreen_Update(float dt)
{

    int mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    // Mouse is outside the window, reset hover states
    newGameButton.hovered = false;
    continueButton.hovered = false;
    profileButton.hovered = false;
    exitButton.hovered = false;

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

    // If mouse is outside window, keep all false
    if (mouseX >= 0 && mouseX <= 1600 && mouseY >= 0 && mouseY <= 900)
    {
    }

    if (AEInputCheckTriggered(AEVK_RSHIFT)) {
        hasSave = !hasSave; // toggle save file existence for testing
    }

    // --- Input handling ---
    if (!isExiting)
    {
        if (hasSave)
        {
            if (IsButtonClicked(profileButton, 137.0f, 40.0f))
            {
                // Go to profile screen
                nextState = GS_NEXT_SCREEN;
            }
            else if (IsButtonClicked(continueButton, 190.0f, 41.0f))
            {
                int recentSlot = GetMostRecentProfileSlot();
                // Continue game (for now just go to farm screen)
                /*isExiting = true;*/
                 // Ensure Profile.cpp's internal array is loaded from disk before
                // calling Profile_SetActiveSlot (which reads from that array).
                if (recentSlot >= 0)
                {
                    // Ensure Profile.cpp's internal array is loaded from disk before
                    // calling Profile_SetActiveSlot (which reads from that array).
                    ProfileScreen_Load();
                    Profile_SetActiveSlot(recentSlot); // sets activeSlot + syncs economy
                    isExiting = true;
                    nextState = GS_MAIN_SCREEN;
                }
            }

            // When a save exists, only check the save-related buttons
            if (IsButtonHovered(continueButton, 190.0f, 41.0f))
                continueButton.hovered = true;
            else if (IsButtonHovered(profileButton, 137.0f, 40.0f))
                profileButton.hovered = true;
            else if (IsButtonHovered(exitButton, 68.0f, 39.0f))
                exitButton.hovered = true;

        }
        else
        {
            if (IsButtonClicked(newGameButton, 234.0f, 35.0f)) {
                /*isExiting = true;*/
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
            // No save: check NewGame + Exit
            if (IsButtonHovered(newGameButton, 234.0f, 35.0f))
                newGameButton.hovered = true;
            else if (IsButtonHovered(exitButton, 68.0f, 39.0f))
                exitButton.hovered = true;

        }

        if (IsButtonClicked(exitButton, 68.0f, 39.0f))
        {
            nextState = GS_EXIT;
        }

        // For now, just simulate button click with keyboard for testing
        if (AEInputCheckTriggered(AEVK_RETURN)) // press Enter to start
        {
            isExiting = true;
        }

    }
    else
    {
        // Animate exit
        exitAnimProgress += (dt * exitAnimSpeed) * 0.1f;
        exitAnimFadeOut -= (dt * exitAnimSpeed);
        if (exitAnimProgress >= 1.0f)
        {
            exitAnimProgress = 1.0f;
            startScreenActive = false; // animation complete
        }
    }
}

void StartScreen_Draw()
{

    if (!startScreenActive) return; // don’t draw after animation finished

    float slideOffset = exitAnimProgress * 3000.0f; // pixels to move left
    float fadeOut = exitAnimFadeOut; // fade out from 1 to 0

    // Transformation Matrices
    AEMtx33 scale, trans, transform;

    //Draw background gradient blur
    if (pMeshGradientBlur)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 0.5f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.9f * fadeOut);
        AEGfxTextureSet(gradientBlur, 0, 0);

        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Scale(&scale, 1920 * gScaleX, 1080.0f * gScaleY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMeshGradientBlur, AE_GFX_MDM_TRIANGLES);


    }

    // Draw logo
    if (pMeshLogo)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.1f, 0.1f, 0.1f, 0.1f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f * fadeOut);
        AEGfxTextureSet(logoTexture, 0, 0);

        AEMtx33Trans(&trans, logoPosX - slideOffset, logoPosY);
        AEMtx33Scale(&scale, 331.0f * gScaleX, 228.0f * gScaleY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMeshLogo, AE_GFX_MDM_TRIANGLES);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    }

    //Exit Button
    if (pMeshExitButton) {

        if (!exitButton.hovered)
        {
            DrawButton(exitButton, pMeshExitButton, 68.0f, 39.0f, slideOffset);
        }
        else if (exitButton.hovered)
        {
            DrawButton(exitButton, pMeshExitButton_Selected, 89.0f, 60.0f, slideOffset);
        }
    }



    // Not new user, has save file
    if (hasSave)
    {
        // Draw Continue button
        if (!continueButton.hovered)
            DrawButton(continueButton, pMeshContinueButton, 190.0f, 41.0f, slideOffset);
        else
            DrawButton(continueButton, pMeshContinueButton_Selected, 211.0f, 61.0f, slideOffset);

        // Draw Profile button
        if (!profileButton.hovered)
            DrawButton(profileButton, pMeshProfileButton, 137.0f, 40.0f, slideOffset);
        else
            DrawButton(profileButton, pMeshProfileButton_Selected, 159.0f, 63.0f, slideOffset);
    }

    else
    {

        if (!newGameButton.hovered)
            DrawButton(newGameButton, pMeshNewGameButton, 234.0f, 35.0f, slideOffset);
        else
            DrawButton(newGameButton, pMeshNewGameButton_Selected, 256.0f, 58.0f, slideOffset);
    }

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


    // Draw New Game button
    // Draw Settings/Profile button
    // Draw Exit button
}

void StartScreen_Unload()
{
    // Free popup resources
    if (pMeshPopup) { AEGfxMeshFree(pMeshPopup); pMeshPopup = nullptr; }
    if (pTexInputRect) { AEGfxTextureUnload(pTexInputRect); pTexInputRect = nullptr; }

    // Free panel texture
    if (pTexPanel) { AEGfxTextureUnload(pTexPanel); pTexPanel = nullptr; }

    // Free logo / background
    if (pMeshLogo) { AEGfxMeshFree(pMeshLogo); pMeshLogo = nullptr; }
    if (pMeshGradientBlur) { AEGfxMeshFree(pMeshGradientBlur); pMeshGradientBlur = nullptr; }
    if (gradientBlur) { AEGfxTextureUnload(gradientBlur); gradientBlur = nullptr; }
    if (logoTexture) { AEGfxTextureUnload(logoTexture); logoTexture = nullptr; }

    // Free button meshes
    if (pMeshNewGameButton) { AEGfxMeshFree(pMeshNewGameButton); pMeshNewGameButton = nullptr; }
    if (pMeshNewGameButton_Selected) { AEGfxMeshFree(pMeshNewGameButton_Selected); pMeshNewGameButton_Selected = nullptr; }
    if (pMeshContinueButton) { AEGfxMeshFree(pMeshContinueButton); pMeshContinueButton = nullptr; }
    if (pMeshContinueButton_Selected) { AEGfxMeshFree(pMeshContinueButton_Selected); pMeshContinueButton_Selected = nullptr; }
    if (pMeshProfileButton) { AEGfxMeshFree(pMeshProfileButton); pMeshProfileButton = nullptr; }
    if (pMeshProfileButton_Selected) { AEGfxMeshFree(pMeshProfileButton_Selected); pMeshProfileButton_Selected = nullptr; }
    if (pMeshExitButton) { AEGfxMeshFree(pMeshExitButton); pMeshExitButton = nullptr; }
    if (pMeshExitButton_Selected) { AEGfxMeshFree(pMeshExitButton_Selected); pMeshExitButton_Selected = nullptr; }

    // Unload button textures (guard against null)
    if (newGameButton.normal) { AEGfxTextureUnload(newGameButton.normal); newGameButton.normal = nullptr; }
    if (newGameButton.hover && newGameButton.hover != newGameButton.normal) { AEGfxTextureUnload(newGameButton.hover); newGameButton.hover = nullptr; }

    if (continueButton.normal) { AEGfxTextureUnload(continueButton.normal); continueButton.normal = nullptr; }
    if (continueButton.hover && continueButton.hover != continueButton.normal) { AEGfxTextureUnload(continueButton.hover); continueButton.hover = nullptr; }

    if (profileButton.normal) { AEGfxTextureUnload(profileButton.normal); profileButton.normal = nullptr; }
    if (profileButton.hover && profileButton.hover != profileButton.normal) { AEGfxTextureUnload(profileButton.hover); profileButton.hover = nullptr; }

    if (exitButton.normal) { AEGfxTextureUnload(exitButton.normal); exitButton.normal = nullptr; }
    if (exitButton.hover && exitButton.hover != exitButton.normal) { AEGfxTextureUnload(exitButton.hover); exitButton.hover = nullptr; }

    // Clear any remaining state
    popupOpen = false;
    popupBuf[0] = '\0';
    popupLen = 0;
    popupShowFull = false;
    popupFullTimer = 0.0f;
    hasSave = false;

    // Note: font unloading isn't included because engine API varies;
    // add font cleanup here if your AE engine exposes a font-unload function.
}