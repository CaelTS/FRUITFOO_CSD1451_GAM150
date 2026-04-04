#include "StartScreen.h"
#include "Profile.h"
#include "AEEngine.h"
#include "Utilities.h"
#include "Tutorial.h"
#include "GameStateManager.h"
#include <fstream>
#include <cstdio>
#include <cstring>
#include "Main.h"
#include "AEAudio.h"

// ============================================================
// Audio  (mirrors the pattern used in Rhythm.cpp)
// ============================================================
static AEAudio      g_startMusic;
static AEAudioGroup g_startMusicGroup;

//static void ResetAudio(AEAudio& audio) { memset(&audio, 0, sizeof(AEAudio)); }
//static void ResetAudioGroup(AEAudioGroup& g) { memset(&g, 0, sizeof(AEAudioGroup)); }

// ============================================================
// Profile persistence
// ============================================================
static constexpr int   MAX_PROFILES = 3;
static constexpr int   PROFILE_NAME_MAX = 32; //name limit
static constexpr int   MAX_FARM_PLOTS = 4;
static constexpr int   MAX_CRATES = 3;
static const char* PROFILES_FILE = "profiles.txt";

static AEGfxVertexList* pMeshPopup = nullptr;
static AEGfxTexture* pTexInputRect = nullptr;

static s8 ssFont = -1;

struct SSProfile
{
    bool exists = false;
    char name[PROFILE_NAME_MAX] = "";
    int  coins = 0;
    int  total_fruits = 0;  // apples + pears + bananas (mirrors Profile.cpp)
    int  total_seeds = 0;  // seed sums (mirrors Profile.cpp)
    // Farm plot data (mirrors Profile.cpp's farm fields)
    bool plot_unlocked[MAX_FARM_PLOTS] = { true, false, false, false };
    bool plot_planted[MAX_FARM_PLOTS] = { false, false, false, false };
    bool plot_ready[MAX_FARM_PLOTS] = { false, false, false, false };
    float plot_timer[MAX_FARM_PLOTS] = { 0.0f, 0.0f, 0.0f, 0.0f };
    int  plot_seed_type[MAX_FARM_PLOTS] = { -1, -1, -1, -1 };
    // Crate data (mirrors Profile.cpp's crate fields)
    // Crate 0 always unlocked; crates 1 & 2 purchasable
    bool crate_unlocked[MAX_CRATES] = { true, false, false };
    int  crate_fruit_count[MAX_CRATES] = { 0, 0, 0 };
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
    bool inEconomy = false;
    bool inInventory = false;
    bool inFarm = false;
    bool inCrate = false;
    char line[256] = {};
    while (fgets(line, sizeof(line), f))
    {
        int len = (int)strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
        if (len > 0 && line[len - 1] == '\r') line[--len] = '\0';
        if (len == 0) continue;

        if (line[0] == '[') {
            if (strcmp(line, "[economy]") == 0) { inEconomy = true;  inInventory = false; inFarm = false;  inCrate = false; }
            else if (strcmp(line, "[inventory]") == 0) { inInventory = true; inEconomy = false;  inFarm = false;  inCrate = false; }
            else if (strcmp(line, "[farm]") == 0) { inFarm = true;     inEconomy = false;   inInventory = false; inCrate = false; }
            else if (strcmp(line, "[crate]") == 0) { inCrate = true;    inFarm = false;      inEconomy = false;   inInventory = false; }
            else {
                inEconomy = false; inInventory = false; inFarm = false; inCrate = false;
                sscanf_s(line, "[PROFILE_%d]", &slot);
            }
            continue;
        }
        if (slot < 0 || slot >= MAX_PROFILES) continue;

        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        const char* key = line;
        const char* val = eq + 1;

        // Only parse top-level profile fields (not in [economy], [inventory], or [farm])
        if (!inEconomy && !inInventory && !inFarm && !inCrate) {
            if (strcmp(key, "EXISTS") == 0) g_profiles[slot].exists = (atoi(val) != 0);
            else if (strcmp(key, "NAME") == 0) strncpy_s(g_profiles[slot].name, PROFILE_NAME_MAX, val, _TRUNCATE);
            else if (strcmp(key, "coins") == 0) g_profiles[slot].coins = atoi(val); // backward compat
        }
        // Read from [economy] section for display purposes
        else if (inEconomy) {
            if (strcmp(key, "total_money") == 0) g_profiles[slot].coins = atoi(val);
        }
        // Parse [inventory] section for total counts displayed on profile slots
        else if (inInventory) {
            if (strcmp(key, "total_fruits") == 0) g_profiles[slot].total_fruits = atoi(val);
            else if (strcmp(key, "total_seeds") == 0)  g_profiles[slot].total_seeds = atoi(val);
        }
        else if (inFarm) {
            int v0, v1, v2, v3;
            float f0, f1, f2, f3;
            if (strcmp(key, "plot_unlocked") == 0) {
                if (sscanf_s(val, "%d,%d,%d,%d", &v0, &v1, &v2, &v3) == 4) {
                    g_profiles[slot].plot_unlocked[0] = (v0 != 0);
                    g_profiles[slot].plot_unlocked[1] = (v1 != 0);
                    g_profiles[slot].plot_unlocked[2] = (v2 != 0);
                    g_profiles[slot].plot_unlocked[3] = (v3 != 0);
                }
            }
            else if (strcmp(key, "plot_planted") == 0) {
                if (sscanf_s(val, "%d,%d,%d,%d", &v0, &v1, &v2, &v3) == 4) {
                    g_profiles[slot].plot_planted[0] = (v0 != 0);
                    g_profiles[slot].plot_planted[1] = (v1 != 0);
                    g_profiles[slot].plot_planted[2] = (v2 != 0);
                    g_profiles[slot].plot_planted[3] = (v3 != 0);
                }
            }
            else if (strcmp(key, "plot_ready") == 0) {
                if (sscanf_s(val, "%d,%d,%d,%d", &v0, &v1, &v2, &v3) == 4) {
                    g_profiles[slot].plot_ready[0] = (v0 != 0);
                    g_profiles[slot].plot_ready[1] = (v1 != 0);
                    g_profiles[slot].plot_ready[2] = (v2 != 0);
                    g_profiles[slot].plot_ready[3] = (v3 != 0);
                }
            }
            else if (strcmp(key, "plot_timer") == 0) {
                if (sscanf_s(val, "%f,%f,%f,%f", &f0, &f1, &f2, &f3) == 4) {
                    g_profiles[slot].plot_timer[0] = f0;
                    g_profiles[slot].plot_timer[1] = f1;
                    g_profiles[slot].plot_timer[2] = f2;
                    g_profiles[slot].plot_timer[3] = f3;
                }
            }
            else if (strcmp(key, "plot_seed_type") == 0) {
                if (sscanf_s(val, "%d,%d,%d,%d", &v0, &v1, &v2, &v3) == 4) {
                    g_profiles[slot].plot_seed_type[0] = v0;
                    g_profiles[slot].plot_seed_type[1] = v1;
                    g_profiles[slot].plot_seed_type[2] = v2;
                    g_profiles[slot].plot_seed_type[3] = v3;
                }
            }
        }
        else if (inCrate) {
            int v0, v1, v2;
            if (strcmp(key, "crate_unlocked") == 0) {
                if (sscanf_s(val, "%d,%d,%d", &v0, &v1, &v2) == 3) {
                    g_profiles[slot].crate_unlocked[0] = (v0 != 0);
                    g_profiles[slot].crate_unlocked[1] = (v1 != 0);
                    g_profiles[slot].crate_unlocked[2] = (v2 != 0);
                }
            }
            else if (strcmp(key, "crate_fruit_count") == 0) {
                if (sscanf_s(val, "%d,%d,%d", &v0, &v1, &v2) == 3) {
                    g_profiles[slot].crate_fruit_count[0] = v0;
                    g_profiles[slot].crate_fruit_count[1] = v1;
                    g_profiles[slot].crate_fruit_count[2] = v2;
                }
            }
        }
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
    BUTTON_EXIT,
    BUTTON_CREDITS
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
Button tutorialButton;
Button creditsButton;

static bool hasSave = false; //placeholder until we implement profile system
static f32 logoPosX = -520.0;
static f32 logoPosY = 260.0;

extern float gScaleX;
extern float gScaleY;

//animation variable
bool startScreenActive = true;   // Is the start screen still active?

// ============================================================
// Local helper function button click detection
// ============================================================
static bool IsButtonClicked(Button& btn, float width, float height) {
    float x = hasSave ? btn.x_save : btn.x;
    float y = hasSave ? btn.y_save : btn.y;

    if (ClickedOnRect(x, y, width * gScaleX, height * gScaleY))
        return true;
    else return false;
}

static bool IsButtonHovered(Button& btn, float width, float height) {
    float x = btn.x;
    float y = btn.y;

    if (hasSave) {
        x = btn.x_save;
        y = btn.y_save;
    }

    return IsMouseOverRect(x, y, width * gScaleX, height * gScaleY);
}

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
AEGfxVertexList* pMeshTutorialButton = nullptr;
AEGfxVertexList* pMeshTutorialButton_Selected = nullptr;
AEGfxVertexList* pMeshCreditsButton = nullptr;
AEGfxVertexList* pMeshCreditsButton_Selected = nullptr;
AEGfxVertexList* pMeshGradientBlur = nullptr;

// Animation variables - now static inside the file scope
static bool isExiting = false;          // Has the exit animation started?
static float exitAnimProgress = 0.0f;   // 0.0 -> 1.0 animation progress
static float exitAnimFadeOut = 1.0f;    // 0.0 -> 1.0 fade out progress
static float exitAnimSpeed = 1.3f;      // Speed of slide animation

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

    // Use the new unified profile system instead of old local save
    Profile_CreateSlot(slot, popupBuf);

    // Ensure Profile.cpp's internal array is loaded from disk
    Profiles_Reload();

    // Set as active slot to initialize Economy and Inventory globals
    Profile_SetActiveSlot(slot);

    // Update local cache to match
    g_profiles[slot].exists = true;
    strncpy_s(g_profiles[slot].name, PROFILE_NAME_MAX, popupBuf, _TRUNCATE);
    g_profiles[slot].coins = 0;

    ClosePopup();
    hasSave = true;
    isExiting = true;
    nextState = GS_MAIN_SCREEN;

    // Force-stop start screen music the moment we leave this state
    if (AEAudioIsValidGroup(g_startMusicGroup))
        AEAudioStopGroup(g_startMusicGroup);
}

// ------------------------------------------------------------
// Helper
// ------------------------------------------------------------

static void DrawButton(Button& btn, AEGfxVertexList* mesh, f32 width, f32 height, float offset)
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

static AEGfxVertexList* createMesh()
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

// ============================================================
// Public interface implementation
// ============================================================

bool StartScreen_IsActive()
{
    return startScreenActive;
}

void StartScreen_Load()
{
    // Load profile data from disk to check if saves exist
    SS_Profiles_Load();
    hasSave = (CountProfiles() > 0);
    Tutorial_Load();

    // Debug: Confirm Load was called
    printf("[DEBUG] StartScreen_Load() called - loading audio\n");
}
void StartScreen_Init()
{
    // Reload profile data to ensure we have latest state
    SS_Profiles_Load();
    hasSave = (CountProfiles() > 0);

    // Only create if not already allocated
    if (!pMeshPopup) pMeshPopup = createMesh();

    // Only load font if not already loaded
    if (ssFont < 0) {
        ssFont = AEGfxCreateFont("Assets/liberation-mono.ttf", 24);
        if (ssFont < 0)
            ssFont = AEGfxCreateFont("Assets/Crayon pastel.otf", 24);
    }

    // Only load textures/meshes if not already loaded (should be in Load function)
    if (!pTexPanel) pTexPanel = AEGfxTextureLoad("Assets/panel_brown.png");
    if (!pTexInputRect) pTexInputRect = AEGfxTextureLoad("Assets/input_outline_rectangle.png");

    if (!logoTexture) logoTexture = AEGfxTextureLoad("Assets/StartScreen_Logo.png");
    if (!gradientBlur) gradientBlur = AEGfxTextureLoad("Assets/StartScreen_GradientBlur.png");

    if (!pMeshLogo) pMeshLogo = createMesh();
    if (!pMeshGradientBlur) pMeshGradientBlur = createMesh();
    if (!pMeshNewGameButton) pMeshNewGameButton = createMesh();
    if (!pMeshNewGameButton_Selected) pMeshNewGameButton_Selected = createMesh();
    if (!pMeshContinueButton) pMeshContinueButton = createMesh();
    if (!pMeshContinueButton_Selected) pMeshContinueButton_Selected = createMesh();
    if (!pMeshProfileButton) pMeshProfileButton = createMesh();
    if (!pMeshProfileButton_Selected) pMeshProfileButton_Selected = createMesh();
    if (!pMeshExitButton) pMeshExitButton = createMesh();
    if (!pMeshExitButton_Selected) pMeshExitButton_Selected = createMesh();

    // Initialize "Exit" button (always shown)
    if (!exitButton.normal) exitButton.normal = AEGfxTextureLoad("Assets/StartScreen_Exit.png");
    if (!exitButton.hover) exitButton.hover = AEGfxTextureLoad("Assets/StartScreen_Exit_Selected.png");
    exitButton.x = logoPosX - 102.0f;
    exitButton.y = -55.0f;
    exitButton.x_selected = exitButton.x; // slide left on hover
    exitButton.y_selected = exitButton.y;
    exitButton.x_save = exitButton.x; // no slide when save exists
    exitButton.y_save = exitButton.y - 110.0f;
    exitButton.x_selected_save = exitButton.x - 1; // no slide when save exists
    exitButton.y_selected_save = exitButton.y_save;

    // Initialize "Continue" button 
    if (!continueButton.normal) continueButton.normal = AEGfxTextureLoad("Assets/StartScreen_Continue.png");
    if (!continueButton.hover) continueButton.hover = AEGfxTextureLoad("Assets/StartScreen_Continue_Selected.png");
    continueButton.x = logoPosX - 50.0f;
    continueButton.y = 0.0f;
    continueButton.x_selected = continueButton.x; // slide left on hover
    continueButton.y_selected = continueButton.y;
    continueButton.x_save = continueButton.x; // no slide when save exists
    continueButton.y_save = continueButton.y;
    continueButton.x_selected_save = continueButton.x; // no slide when save exists
    continueButton.y_selected_save = continueButton.y;

    // Initialize "Profile" button
    if (!profileButton.normal) profileButton.normal = AEGfxTextureLoad("Assets/StartScreen_Profile.png");
    if (!profileButton.hover) profileButton.hover = AEGfxTextureLoad("Assets/StartScreen_Profile_Selected.png");
    profileButton.x = logoPosX - 72.0f;
    profileButton.y = -55.0f;
    profileButton.x_selected = profileButton.x; // slide left on hover
    profileButton.y_selected = profileButton.y;
    profileButton.x_save = profileButton.x; // no slide when save exists    
    profileButton.y_save = profileButton.y;
    profileButton.x_selected_save = profileButton.x; // no slide when save exists   
    profileButton.y_selected_save = profileButton.y;

    // Initialize "New Game" button
    if (!newGameButton.normal) newGameButton.normal = AEGfxTextureLoad("Assets/StartScreen_NewGameButton.png");
    if (!newGameButton.hover) newGameButton.hover = AEGfxTextureLoad("Assets/StartScreen_NewGameButton_Selected.png");
    newGameButton.x = logoPosX - 32.0f;
    newGameButton.y = 0.0f;
    newGameButton.x_selected = newGameButton.x; // slide left on hover
    newGameButton.y_selected = newGameButton.y;

    // Ensure Tutorial resources are ready even if StartScreen_Load was
    // skipped (Tutorial_Load is idempotent -- safe to call more than once).
    Tutorial_Load();

    // Initialize "Tutorial" button
    if (!tutorialButton.normal) tutorialButton.normal = AEGfxTextureLoad("Assets/StartScreen_Tutorial.png");
    if (!tutorialButton.hover)  tutorialButton.hover = AEGfxTextureLoad("Assets/StartScreen_Tutorial_selected.png");
    if (!pMeshTutorialButton)          pMeshTutorialButton = createMesh();
    if (!pMeshTutorialButton_Selected) pMeshTutorialButton_Selected = createMesh();
    tutorialButton.x = logoPosX - 50.0f;  // aligned with continueButton
    tutorialButton.y = 55.0f;
    tutorialButton.x_selected = logoPosX - 50.0f;
    tutorialButton.y_selected = 55.0f;
    tutorialButton.x_save = logoPosX - 50.0f;
    tutorialButton.y_save = 55.0f;
    tutorialButton.x_selected_save = logoPosX - 50.0f;
    tutorialButton.y_selected_save = 55.0f;

    // Initialize "Credits" button — same texture style as Exit, positioned below it
    if (!creditsButton.normal) creditsButton.normal = AEGfxTextureLoad("Assets/StartScreen_Credits.png");
    if (!creditsButton.hover)  creditsButton.hover = AEGfxTextureLoad("Assets/StartScreen_Credits_Selected.png");
    if (!pMeshCreditsButton)          pMeshCreditsButton = createMesh();
    if (!pMeshCreditsButton_Selected) pMeshCreditsButton_Selected = createMesh();
    // Positioned directly below the Exit button (exit is at y=-55, credits sit 55px lower)
    creditsButton.x = continueButton.x;
    creditsButton.y = exitButton.y + 55.0f;
    creditsButton.x_selected = continueButton.x_selected;
    creditsButton.y_selected = exitButton.y_selected + 55.0f;
    creditsButton.x_save = continueButton.x_save;
    creditsButton.y_save = profileButton.y_save - 55.0f;
    creditsButton.x_selected_save = continueButton.x_selected_save;
    creditsButton.y_selected_save = profileButton.y_selected_save - 55.0f;

    // Reset animation state
    isExiting = false;
    exitAnimProgress = 0.0f;
    exitAnimFadeOut = 1.0f;
    startScreenActive = true;
}

void StartScreen_Update(float dt)
{
    // Refresh profile check every frame in case profiles were deleted/added
    int currentProfileCount = CountProfiles();
    hasSave = (currentProfileCount > 0);

    int mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    // Mouse is outside the window, reset hover states
    newGameButton.hovered = false;
    continueButton.hovered = false;
    profileButton.hovered = false;
    exitButton.hovered = false;
    tutorialButton.hovered = false;
    creditsButton.hovered = false;

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

    // Tutorial -- must run before other buttons; suppresses input while open
    Tutorial_Update();
    if (Tutorial_IsOpen()) return;

    // --- Input handling ---
    if (!isExiting)
    {
        if (hasSave)
        {
            if (IsButtonClicked(profileButton, 137.0f, 40.0f))
            {
                // Go to profile screen
                nextState = GS_NEXT_SCREEN;

                //// Force-stop start screen music the moment we leave this state
                //if (AEAudioIsValidGroup(g_startMusicGroup))
                //    AEAudioStopGroup(g_startMusicGroup);
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
                    Profiles_Reload();
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
            else if (IsButtonHovered(creditsButton, 190.0f, 41.0f))
                creditsButton.hovered = true;

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
            // No save: check NewGame + Exit + Credits
            if (IsButtonHovered(newGameButton, 234.0f, 35.0f))
                newGameButton.hovered = true;
            else if (IsButtonHovered(exitButton, 68.0f, 39.0f))
                exitButton.hovered = true;
            else if (IsButtonHovered(creditsButton, 190.0f, 41.0f))
                creditsButton.hovered = true;

        }

        if (IsButtonClicked(exitButton, 68.0f, 39.0f))
        {
            nextState = GS_EXIT;
        }

        if (IsButtonClicked(creditsButton, 190.0f, 41.0f))
        {
            nextState = GS_CREDITS;
            startScreenActive = false;
        }

        // Tutorial button (always visible, both layouts)
        if (IsButtonClicked(tutorialButton, 190.0f, 41.0f))
            Tutorial_Open();
        if (IsButtonHovered(tutorialButton, 190.0f, 41.0f))
            tutorialButton.hovered = true;

        // For now, just simulate button click with keyboard for testing
        if (AEInputCheckTriggered(AEVK_RETURN)) // press Enter to start
        {
            isExiting = true;
        }

    }
    else
    {
        // Animate exit
        exitAnimProgress += dt * exitAnimSpeed * 0.8f;
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
    if (!startScreenActive) return; // don't draw after animation finished

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

    //Credits Button (same dimensions as Continue button)
    if (pMeshCreditsButton) {
        if (!creditsButton.hovered)
            DrawButton(creditsButton, pMeshCreditsButton, 190.0f, 41.0f, slideOffset);
        else
            DrawButton(creditsButton, pMeshCreditsButton_Selected, 211.0f, 61.0f, slideOffset);
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

    // Tutorial button + panel (drawn on top of everything)
    if (!tutorialButton.hovered)
        DrawButton(tutorialButton, pMeshTutorialButton, 190.0f, 41.0f, slideOffset);
    else
        DrawButton(tutorialButton, pMeshTutorialButton_Selected, 211.0f, 61.0f, slideOffset);

    Tutorial_Draw();
}

void StartScreen_Unload()
{
    Tutorial_Unload();

    // Stop and release music (mirrors Rhythm.cpp teardown)
    if (AEAudioIsValidGroup(g_startMusicGroup))
        AEAudioStopGroup(g_startMusicGroup);
    if (AEAudioIsValidAudio(g_startMusic))
        AEAudioUnloadAudio(g_startMusic);
    if (AEAudioIsValidGroup(g_startMusicGroup))
        AEAudioUnloadAudioGroup(g_startMusicGroup);
  
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
    if (pMeshTutorialButton) { AEGfxMeshFree(pMeshTutorialButton); pMeshTutorialButton = nullptr; }
    if (pMeshTutorialButton_Selected) { AEGfxMeshFree(pMeshTutorialButton_Selected); pMeshTutorialButton_Selected = nullptr; }
    if (pMeshCreditsButton) { AEGfxMeshFree(pMeshCreditsButton); pMeshCreditsButton = nullptr; }
    if (pMeshCreditsButton_Selected) { AEGfxMeshFree(pMeshCreditsButton_Selected); pMeshCreditsButton_Selected = nullptr; }

    // Unload button textures (guard against null)
    if (newGameButton.normal) { AEGfxTextureUnload(newGameButton.normal); newGameButton.normal = nullptr; }
    if (newGameButton.hover && newGameButton.hover != newGameButton.normal) { AEGfxTextureUnload(newGameButton.hover); newGameButton.hover = nullptr; }

    if (continueButton.normal) { AEGfxTextureUnload(continueButton.normal); continueButton.normal = nullptr; }
    if (continueButton.hover && continueButton.hover != continueButton.normal) { AEGfxTextureUnload(continueButton.hover); continueButton.hover = nullptr; }

    if (profileButton.normal) { AEGfxTextureUnload(profileButton.normal); profileButton.normal = nullptr; }
    if (profileButton.hover && profileButton.hover != profileButton.normal) { AEGfxTextureUnload(profileButton.hover); profileButton.hover = nullptr; }

    if (exitButton.normal) { AEGfxTextureUnload(exitButton.normal); exitButton.normal = nullptr; }
    if (exitButton.hover && exitButton.hover != exitButton.normal) { AEGfxTextureUnload(exitButton.hover); exitButton.hover = nullptr; }

    if (tutorialButton.normal) { AEGfxTextureUnload(tutorialButton.normal); tutorialButton.normal = nullptr; }
    if (tutorialButton.hover && tutorialButton.hover != tutorialButton.normal) { AEGfxTextureUnload(tutorialButton.hover); tutorialButton.hover = nullptr; }

    if (creditsButton.normal) { AEGfxTextureUnload(creditsButton.normal); creditsButton.normal = nullptr; }
    if (creditsButton.hover && creditsButton.hover != creditsButton.normal) { AEGfxTextureUnload(creditsButton.hover); creditsButton.hover = nullptr; }

    // Clear any remaining state
    popupOpen = false;
    popupBuf[0] = '\0';
    popupLen = 0;
    popupShowFull = false;
    popupFullTimer = 0.0f;
    hasSave = false;

    if (ssFont >= 0) {
        AEGfxDestroyFont(ssFont);
        ssFont = -1;
    }
}