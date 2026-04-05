#include <crtdbg.h>
#include "AEEngine.h"
#include "Main.h"
#include "GameStateManager.h"
#include "Transition.h"
#include <stdio.h>
#include <vector>
#include <fstream>
#include <random>
#include "Economy.h"
#include "SpawnFruits.h"
#include "UI.h"
#include "Rhythm.h"
#include "Farm.h"
#include <iostream>
#include "Profile.h"
#include "StartScreen.h"
#include "Utilities.h"
#include "Crate.h"
#include "Inventory.h"
#include "HelperCreatures.h"
#include "Upgrades.h"
#include "AEAudio.h"
#include "UIAudio.h"

// Access farm plots for ready state tracking
extern std::vector<FarmPlot> farmPlots;

// ============================================================
// Main screen BGM
// ============================================================
static AEAudio      g_mainBGM;
static AEAudioGroup g_mainBGMGroup;
// Track whether we've started playback to avoid restarting
static bool         g_mainBGMPlaying = false;

static void MainBGM_Start()
{
    // Require valid audio + group
    if (!AEAudioIsValidAudio(g_mainBGM) || !AEAudioIsValidGroup(g_mainBGMGroup))
        return;

    // If already playing, do nothing (prevents restart)
    if (g_mainBGMPlaying)
        return;

    // Start playback (looping)
    AEAudioPlay(g_mainBGM, g_mainBGMGroup, 0.3f, 1.0f, -1); // -1 = infinite loop
    g_mainBGMPlaying = true;
}

static void MainBGM_Stop()
{
    if (AEAudioIsValidGroup(g_mainBGMGroup))
    {
        AEAudioStopGroup(g_mainBGMGroup);
    }
    // mark as stopped so Start can start fresh later
    g_mainBGMPlaying = false;
}

void MainBGM_SetEnabled(bool enabled)
{
    if (enabled)
        MainBGM_Start();
    else
        MainBGM_Stop();
}

// ---------------------------------------------------------------------------
// Game State Variables

std::vector<FruitBasket> gFruitBaskets;
const std::vector<FruitBasket>& GetFruitBaskets()
{
    return gFruitBaskets;
}

//Image Scale
float rescale = 70.0f;

float gScaleX = 1600.0f / 1920.0f;
float gScaleY = 900.0f / 1080.0f;

// Start Screen
bool gStartScreenActive = true;

// Graphics Resources
s8 fontId = -1;
AEGfxTexture* pBackground = NULL;
AEGfxTexture* pGrass = NULL;
AEGfxTexture* pBaseStall = NULL;

AEGfxTexture* pTexApple = NULL;
AEGfxTexture* pTexPear = NULL;
AEGfxTexture* pTexBanana = NULL;
AEGfxTexture* pTexFruitApple = NULL;   // Fruit_Apple.png — used for crate display

AEGfxVertexList* pMeshBackground = NULL;
AEGfxVertexList* pMeshGrass = NULL;
AEGfxVertexList* pMeshStall = NULL;
AEGfxVertexList* pMeshFruit = NULL;
AEGfxVertexList* g_pMeshFullScreen = NULL;
AEGfxTexture* pTexPlus = nullptr;

// ---------------------------------------------------------------------------
// Pause Popup
// ---------------------------------------------------------------------------
static AEGfxTexture* pTexPausePanel = nullptr;
static AEGfxTexture* pTexPauseBtn = nullptr;
static AEGfxVertexList* pMeshPauseQuad = nullptr;

static bool g_pauseOpen = false;
static bool g_returnedFromPause = false;
static int  g_pauseHovered = -1;

// Key legend toggle (H key)
static bool g_legendOpen = true;   // visible by default; H hides/shows it
static s8   g_legendFontKey = -1;  // Nunito-SemiBold  — key labels
static s8   g_legendFontDesc = -1;  // Nunito-Regular   — descriptions

static const float PAUSE_PANEL_W = 400.0f;
static const float PAUSE_PANEL_H = 260.0f;
static const float PAUSE_BTN_W = 300.0f;
static const float PAUSE_BTN_H = 55.0f;
static const float PAUSE_BTN_Y0 = 20.0f;
static const float PAUSE_BTN_Y1 = -60.0f;

// ---------------------------------------------------------------------------
// Status Toast System
// ---------------------------------------------------------------------------
static const int   TOAST_MAX = 4;
static const float TOAST_LIFETIME = 3.0f;   // seconds before fade starts
static const float TOAST_FADETIME = 0.6f;   // seconds to fade to zero

struct Toast
{
    char  text[96] = {};
    float r = 1, g = 1, b = 1;  // text colour
    float timer = 0.0f;          // counts UP from 0
    bool  active = false;
};
static Toast g_toasts[TOAST_MAX];
static s8 g_toastFont = -1;
s8 g_uiFont = -1;

// Push a new toast (overwrites oldest if queue is full)
void Toast_Push(const char* msg, float r, float g , float b )
{
    // Find a free slot; if none, evict the one closest to expiry
    int slot = -1;
    float oldest = -1.0f;
    for (int i = 0; i < TOAST_MAX; i++)
    {
        if (!g_toasts[i].active) { slot = i; break; }
        if (g_toasts[i].timer > oldest) { oldest = g_toasts[i].timer; slot = i; }
    }
    if (slot < 0) slot = 0;
    strncpy_s(g_toasts[slot].text, sizeof(g_toasts[slot].text), msg, _TRUNCATE);
    g_toasts[slot].r = r;
    g_toasts[slot].g = g;
    g_toasts[slot].b = b;
    g_toasts[slot].timer = 0.0f;
    g_toasts[slot].active = true;
}

// Tick timers; call once per frame in Update
static void Toast_Update(float dt)
{
    for (int i = 0; i < TOAST_MAX; i++)
    {
        if (!g_toasts[i].active) continue;
        g_toasts[i].timer += dt;
        if (g_toasts[i].timer >= TOAST_LIFETIME + TOAST_FADETIME)
            g_toasts[i].active = false;
    }
}

// ---------------------------------------------------------------------------
// Status tracking — previous-frame state to detect changes
// ---------------------------------------------------------------------------
static bool  g_prevPlotReady[4] = { false, false, false, false };
static bool  g_prevRhythmAvail = false;
static int   g_prevCrateCount[3] = { 0, 0, 0 };
static int   g_prevGold = -1;
static float g_prevPlotTimer[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
static int   g_prevFruitCount = -1;

// ---------------------------------------------------------------------------
// Rhythm Reward Popup
// ---------------------------------------------------------------------------
static bool g_rewardPopupOpen = false;
static char g_rewardPopupText[256] = "";

static void GrantRhythmReward()
{
    if (!Rhythm_IsSongFinished()) return;

    int seedType = Farm_GetRhythmSeedType();
    RhythmRewardTier tier = Rhythm_GetRewardTier();
    RhythmDifficulty diff = Rhythm_GetDifficulty();

    g_rewardPopupOpen = true;

    switch (diff)
    {
    case DIFFICULTY_EASY:
        if (tier == REWARD_POOR) {
            sprintf_s(g_rewardPopupText, "Rhythm Complete!\nNo rewards this time.\nKeep practicing!");
        }
        else if (tier == REWARD_AVERAGE) {
            Inventory_AddFruit(1, static_cast<u8>(seedType));
            sprintf_s(g_rewardPopupText, "Rhythm Complete!\nReward: 1 Fruit\nadded to inventory!");
        }
        else {
            Inventory_AddFruit(2, static_cast<u8>(seedType));
            sprintf_s(g_rewardPopupText, "Rhythm Complete!\nReward: 2 Fruits\nadded to inventory!");
        }
        break;

    case DIFFICULTY_MEDIUM:
        if (tier == REWARD_POOR) {
            Inventory_AddFruit(1, static_cast<u8>(seedType));
            sprintf_s(g_rewardPopupText, "Rhythm Complete!\nReward: 1 Fruit\nadded to inventory!");
        }
        else if (tier == REWARD_AVERAGE) {
            Inventory_AddFruit(2, static_cast<u8>(seedType));
            sprintf_s(g_rewardPopupText, "Rhythm Complete!\nReward: 2 Fruits\nadded to inventory!");
        }
        else {
            Inventory_AddFruit(2, static_cast<u8>(seedType));
            Economy_AddMoney(50);
            sprintf_s(g_rewardPopupText, "Rhythm Complete!\nReward: 2 Fruits\n+ 50 Gold added!");
        }
        break;

    case DIFFICULTY_HARD:
        if (tier == REWARD_POOR) {
            Inventory_AddFruit(2, static_cast<u8>(seedType));
            sprintf_s(g_rewardPopupText, "Rhythm Complete!\nReward: 2 Fruits\nadded to inventory!");
        }
        else if (tier == REWARD_AVERAGE) {
            Inventory_AddFruit(1, static_cast<u8>(seedType));
            Inventory_AddSeed(1, static_cast<u8>(seedType));
            sprintf_s(g_rewardPopupText, "Rhythm Complete!\nReward: 1 Fruit\n+ 1 Seed added!");
        }
        else {
            Inventory_AddSeed(1, static_cast<u8>(seedType));
            Economy_AddMoney(100);
            sprintf_s(g_rewardPopupText, "Rhythm Complete!\nReward: 1 Seed\n+ 100 Gold added!");
        }
        break;
    }
}

void MainScreen_OnHelperCollect(int amount , FruitType fruit)
{
    char msg[96];

    if (fruit == APPLE) {
        sprintf_s(msg, "Helper collected %d apple%s!", amount, (amount > 1) ? "s" : "");
    }
    if (fruit == PEAR) {
        sprintf_s(msg, "Helper collected %d pear%s!", amount, (amount > 1) ? "s" : "");
    }
    if (fruit == BANANA) {
        sprintf_s(msg, "Helper collected %d banana%s!", amount, (amount > 1) ? "s" : "");
    }
    Toast_Push(msg, 0.4f, 0.9f, 0.4f);
}

void MainScreen_Load()
{
    pBackground = AEGfxTextureLoad("Assets/MainMenu_Background.png");
    pGrass = AEGfxTextureLoad("Assets/MainMenu_Background_Grass.png");
    pBaseStall = AEGfxTextureLoad("Assets/base level 1 with apple.png");
    pTexApple = AEGfxTextureLoad("Assets/Apple.png");
    pTexPear = AEGfxTextureLoad("Assets/Pear.png");
    pTexBanana = AEGfxTextureLoad("Assets/Banana.png");
    pTexFruitApple = AEGfxTextureLoad("Assets/Fruit_Apple.png");  // for crate display
    pTexPlus = AEGfxTextureLoad("Assets/Plus.png");
    Farm_Load();
    Crate_Load();

    // Load main screen BGM
    g_mainBGM = AEAudioLoadMusic("Assets/bgm.wav");
    g_mainBGMGroup = AEAudioCreateGroup();

    // Pause popup assets
    pTexPausePanel = AEGfxTextureLoad("Assets/panel_brown.png");
    pTexPauseBtn = AEGfxTextureLoad("Assets/input_outline_rectangle.png");
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    pMeshPauseQuad = AEGfxMeshEnd();

    if (!pBackground) OutputDebugStringA("ERROR: Failed to load 'Assets/MainMenu_Background.png'.\n");
    if (!pBaseStall)  OutputDebugStringA("ERROR: Failed to load 'Assets/base level 1 with apple.png'.\n");
    if (!pTexApple)   OutputDebugStringA("ERROR: Failed to load 'Assets/Apple.png'.\n");
    if (!pTexPear)    OutputDebugStringA("ERROR: Failed to load 'Assets/Pear.png'.\n");
    if (!pTexBanana)  OutputDebugStringA("ERROR: Failed to load 'Assets/Banana.png'.\n");
}

void MainScreen_Initialize()
{
    // If StartScreen requested a profile activation, do it now (deferred work)
    if (g_pendingProfileSlot >= 0) {
        Profiles_Reload();
        Profile_SetActiveSlot(g_pendingProfileSlot);
        g_pendingProfileSlot = -1;
    }

    // Start Screen Init
    if (g_returnedFromPause)
    {
        // Pause->Main Menu: overlay already active, nothing to change
    }
    else if (previousState == GS_NEXT_SCREEN && !Profile_WentBack())
    {
        // Profile selected -- go straight into game
        gStartScreenActive = false;
        startScreenActive = false;
    }
    else if (previousState == GS_RHYTHM_SCREEN)
    {
        // Returning from rhythm -- skip start screen
        gStartScreenActive = false;
        startScreenActive = false;
    }
    else if (previousState == GS_START_SCREEN)
    {
        // From standalone start screen state -- go straight to game
        gStartScreenActive = false;
        startScreenActive = false;
    }
    else
    {
        // First launch -- show start screen overlay
        gStartScreenActive = true;
        StartScreen_Init();
    }

    // Reset pause popup state
    g_pauseOpen = false;
    g_pauseHovered = -1;
    g_returnedFromPause = false;

    Economy_Init();
    SpawnFruit_Init();
    Upgrades_Init();

    UI_Init();
    Helper_Init();

    UIAudio_EnableSFX(gSoundEnabled);
    UIAudio_SetMusicEnabled(gMusicEnabled);
    MainBGM_SetEnabled(gMusicEnabled);

    if (previousState != GS_RHYTHM_SCREEN)
    {
        Farm_Initialize();
        Crate_Initialize();
    }

    // Load per-type fruit/seed counts from the active profile
    Inventory_LoadFromProfile(Profile_GetActiveSlot());

    // Initialize fruit count tracking after inventory is loaded
    g_prevFruitCount = GetFruitCount();

    // Build crate rectangles to match the stall's current transform
    {
        float stallW = 702.0f * gScaleX;
        float stallH = 716.0f * gScaleY;
        float stallX = 330.0f * gScaleX;
        float stallY = -15.0f * gScaleY;
        UI_RebuildCrateHitboxesFromStall(stallX, stallY, stallW, stallH);
    }

    fontId = AEGfxCreateFont("Assets/Crayon pastel.otf", 26);

	g_uiFont = AEGfxCreateFont("Assets/Crayon pastel.otf", 54); //for big text like the start screen title and the rhythm reward popup

    if (fontId < 0)
        OutputDebugStringA("ERROR: Failed to load 'Assets/Crayon pastel.otf'.\n");
    
    g_toastFont = fontId;

    // Legend fonts — Nunito gives a clean, modern game-UI feel.
    g_legendFontKey = AEGfxCreateFont("Assets/Nunito-SemiBold.ttf", 28);
    if (g_legendFontKey < 0)
        g_legendFontKey = fontId;

    g_legendFontDesc = AEGfxCreateFont("Assets/Nunito-Regular.ttf", 28);
    if (g_legendFontDesc < 0)
        g_legendFontDesc = fontId;

    // Start BGM only if we're going straight into the game
    if (!gStartScreenActive && gMusicEnabled)
        MainBGM_Start();

    if (!pMeshBackground)
    {
        AEGfxMeshStart();
        AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
        AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
        pMeshBackground = AEGfxMeshEnd();
    }

    if (!pMeshGrass)
    {
        AEGfxMeshStart();
        AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
        AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
        pMeshGrass = AEGfxMeshEnd();
    }

    if (!pMeshStall)
    {
        AEGfxMeshStart();
        AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
        AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
        pMeshStall = AEGfxMeshEnd();
    }

    if (!pMeshFruit)
    {
        AEGfxMeshStart();
        AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
        AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
        pMeshFruit = AEGfxMeshEnd();
    }

    // Debug toast to verify system works
    Toast_Push("Game Started! Press SPACE to harvest ready crops.", 1.0f, 1.0f, 0.5f);
}

void MainScreen_Update()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();

    // Dismiss reward popup — blocks all other input until clicked away
    if (g_rewardPopupOpen)
    {
        if (AEInputCheckTriggered(AEVK_LBUTTON) ||
            AEInputCheckTriggered(AEVK_RETURN) ||
            AEInputCheckTriggered(AEVK_SPACE))
        {
            g_rewardPopupOpen = false;
        }
        return;
    }

    if (gStartScreenActive)
    {
        StartScreen_Update(dt);
        if (!StartScreen_IsActive())
        {
            gStartScreenActive = false;
            if (gMusicEnabled)
                MainBGM_Start();
        }
        return;
    }

    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    // ---------------------------------------------------------------
    // Pause popup
    // ---------------------------------------------------------------
    if (!gStartScreenActive)
    {
        UI_Input();

        if (AEInputCheckTriggered(AEVK_ESCAPE))
        {
            if (UI_IsMenuOpen())
            {
                OutputDebugStringA("PAUSE BLOCKED: UI menu is open\n");
            }
            else
            {
                g_pauseOpen = !g_pauseOpen;
                OutputDebugStringA(g_pauseOpen ? "PAUSE OPENED\n" : "PAUSE CLOSED\n");
            }
        }
        if (g_pauseOpen)
        {
            g_pauseHovered = -1;
            if (IsMouseOverRect(0.0f, PAUSE_BTN_Y0, PAUSE_BTN_W, PAUSE_BTN_H)) g_pauseHovered = 0;
            else if (IsMouseOverRect(0.0f, PAUSE_BTN_Y1, PAUSE_BTN_W, PAUSE_BTN_H)) g_pauseHovered = 1;

            if (ClickedOnRect(0.0f, PAUSE_BTN_Y0, PAUSE_BTN_W, PAUSE_BTN_H))
            {
                Profile_EndSession();
                g_pauseOpen = false;
                g_returnedFromPause = true;
                MainBGM_Stop();
                StartScreen_Init();
                gStartScreenActive = true;
            }
            else if (ClickedOnRect(0.0f, PAUSE_BTN_Y1, PAUSE_BTN_W, PAUSE_BTN_H))
            {
                Profile_EndSession();
                g_pauseOpen = false;
                nextState = GS_EXIT;
            }
            return;
        }
    }

    // Farm gets first pick of all clicks
    Farm_Update();

    // Toggle key legend (H)
    if (AEInputCheckTriggered(AEVK_H))
        g_legendOpen = !g_legendOpen;

    Economy_Update(dt);
    Crate_Update();
    UpdateSpawnFruits(dt);
    UpdateFruitSpawner(dt);
    Helper_Update(dt);
    CheckForFruitClicks(mouseX, mouseY);

    // ---------------------------------------------------------------
    // Status toasts — update timers and detect changes
    // ---------------------------------------------------------------
    Toast_Update(dt);

    // Farm plot growth tracking and harvest detection
    for (int i = 0; i < 4; i++)
    {
        bool isReady = farmPlots[i].isReady;
        bool isPlanted = farmPlots[i].isPlanted;
        float currentTimer = farmPlots[i].growTimer;
        float growTime = Farm_GetGrowTime();
        float remainingTime = (currentTimer < growTime) ? (growTime - currentTimer) : 0.0f;

        // Detect when a plot becomes ready
        if (isReady && !g_prevPlotReady[i] && isPlanted)
        {
            char msg[96];
            sprintf_s(msg, "Plot %d is ready to harvest!", i + 1);
            Toast_Push(msg, 0.3f, 1.0f, 0.3f);   // Bright green
        }

        // Track growth progress at milestones
        if (!isReady && isPlanted)
        {
            float ratio = currentTimer / growTime;
            int percentComplete = (int)(ratio * 100);

            if (percentComplete >= 75 && percentComplete < 80 && g_prevPlotTimer[i] / growTime < 0.75f)
            {
                char timeMsg[32];
                FormatTime(remainingTime, timeMsg, sizeof(timeMsg));
                char toastMsg[128];
                sprintf_s(toastMsg, "Plot %d: 75%% complete - %s remaining", i + 1, timeMsg);
                Toast_Push(toastMsg, 1.0f, 0.7f, 0.2f);   // Orange
            }
            else if (percentComplete >= 50 && percentComplete < 55 && g_prevPlotTimer[i] / growTime < 0.50f)
            {
                char timeMsg[32];
                FormatTime(remainingTime, timeMsg, sizeof(timeMsg));
                char toastMsg[128];
                sprintf_s(toastMsg, "Plot %d: 50%% complete - %s remaining", i + 1, timeMsg);
                Toast_Push(toastMsg, 1.0f, 0.7f, 0.2f);
            }
            else if (percentComplete >= 25 && percentComplete < 30 && g_prevPlotTimer[i] / growTime < 0.25f)
            {
                char timeMsg[32];
                FormatTime(remainingTime, timeMsg, sizeof(timeMsg));
                char toastMsg[128];
                sprintf_s(toastMsg, "Plot %d: 25%% complete - %s remaining", i + 1, timeMsg);
                Toast_Push(toastMsg, 1.0f, 0.7f, 0.2f);
            }
        }

        g_prevPlotReady[i] = isReady;
        g_prevPlotTimer[i] = currentTimer;
    }

    // Detect when a plot is harvested
    static int lastHarvestedPlot = -1;
    for (int i = 0; i < 4; i++)
    {
        bool isPlanted = farmPlots[i].isPlanted;
        if (g_prevPlotReady[i] && !isPlanted)
        {
            if (lastHarvestedPlot != i)
            {
                lastHarvestedPlot = i;

                int seedType = farmPlots[i].seedType;
                const char* fruitName = (seedType == 0) ? "Apple" :
                    (seedType == 1) ? "Pear" : "Banana";

                char msg[96];
                sprintf_s(msg, "Harvested %s from Plot %d!", fruitName, i + 1);
                Toast_Push(msg, 1.0f, 0.85f, 0.3f);  // Gold color
            }
        }
    }

    // Rhythm watering-can available
    bool rhythmAvail = Farm_ShouldStartRhythm();
    if (rhythmAvail && !g_prevRhythmAvail)
        Toast_Push("Watering can ready - play Rhythm game!", 0.3f, 0.8f, 1.0f);  // Cyan
    g_prevRhythmAvail = rhythmAvail;

    // Crate stocked notifications
    static const char* fruitNames[] = { "Apple", "Pear", "Banana" };
    for (int i = 0; i < 3; i++)
    {
        if (!Crate_IsUnlocked(i)) { g_prevCrateCount[i] = 0; continue; }
        int count = Crate_GetFruitCount(i);
        if (count > g_prevCrateCount[i])
        {
            char msg[64];
            int added = count - g_prevCrateCount[i];
            sprintf_s(msg, "+%d %s added to stall.", added, fruitNames[i]);
            Toast_Push(msg, 1.0f, 0.9f, 0.2f);   // Bright yellow
        }
        g_prevCrateCount[i] = count;
    }

    // Gold earned (fruit sold)
    int gold = Economy_GetTotalMoney();
    if (g_prevGold >= 0 && gold > g_prevGold)
    {
        char msg[64];
        sprintf_s(msg, "+%d gold earned!", gold - g_prevGold);
        Toast_Push(msg, 1.0f, 0.95f, 0.2f);   // Gold
    }
    g_prevGold = gold;

    // Check if farm triggered rhythm
    if (Farm_ShouldStartRhythm())
    {
        OutputDebugStringA("Farm requested rhythm game\n");
        Farm_ClearRhythmFlag();
        Rhythm_SetSeedType(Farm_GetRhythmSeedType());
        MainBGM_Stop();
        nextState = GS_RHYTHM_SCREEN;
    }
}

//// ---------------------------------------------------------------------------
//// Draw fruit icons + count inside each crate bin
//// ---------------------------------------------------------------------------
//static void MainScreen_DrawCrateFruits()
//{
//    const auto& baskets = GetFruitBaskets();
//    AEMtx33 scale, trans, transform;
//
//    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
//    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
//    AEGfxSetColorToMultiply(1, 1, 1, 1);
//    AEGfxSetTransparency(1.0f);
//
//    for (int i = 0; i < (int)baskets.size(); i++)
//    {
//        if (!Crate_IsUnlocked(i)) continue;
//
//        int count = Crate_GetFruitCount(i);
//        if (count <= 0) continue;
//
//        AEGfxTexture* tex = nullptr;
//        switch (i)
//        {
//        case 0: tex = pTexFruitApple ? pTexFruitApple : pTexApple; break;
//        case 1: tex = pTexPear;   break;
//        case 2: tex = pTexBanana; break;
//        }
//        if (!tex) continue;
//
//        const auto& b = baskets[i];
//
//        float iconSize = b.height * 0.65f;
//        int displayCount = (count > 5) ? 5 : count;
//        float spread = b.width * 0.55f;
//        float spacing = (displayCount > 1) ? spread / (displayCount - 1) : 0.0f;
//        float startX = b.x - spread * 0.5f;
//
//        AEGfxTextureSet(tex, 0, 0);
//
//        for (int n = 0; n < displayCount; n++)
//        {
//            float fx = startX + n * spacing;
//            float fy = b.y;
//
//            AEMtx33Scale(&scale, iconSize, iconSize);
//            AEMtx33Trans(&trans, fx, fy);
//            AEMtx33Concat(&transform, &trans, &scale);
//            AEGfxSetTransform(transform.m);
//            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
//        }
//
//        char buf[8];
//        sprintf_s(buf, "x%d", count);
//        float textX = (b.x - b.width * 0.05f) / 800.0f;
//        float textY = (b.y + b.height * 0.65f) / 450.0f;
//        AEGfxSetColorToMultiply(0, 0, 0, 1);
//        AEGfxPrint(fontId, buf, textX, textY, 0.7f, 0, 0, 0, 1);
//        AEGfxSetColorToMultiply(1, 1, 1, 1);
//    }
//}

void MainScreen_Render()
{
    AEGfxSetBackgroundColor(0.2f, 0.2f, 0.2f);

    AEMtx33 scale, trans, transform;

    // Draw Background
    if (pBackground)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxTextureSet(pBackground, 0, 0);
    }
    else
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    }

    AEMtx33Scale(&scale, 1920 * gScaleX, 1080.0f * gScaleY);
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMeshBackground, AE_GFX_MDM_TRIANGLES);

    // Draw Base Stall
    if (pBaseStall)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxTextureSet(pBaseStall, 0, 0);
        AEMtx33Scale(&scale, 702.0f * gScaleX, 716.0f * gScaleY);
        AEMtx33Trans(&trans, 330.0f * gScaleX, -15.0f * gScaleY);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMeshStall, AE_GFX_MDM_TRIANGLES);
    }

    // Draw Grass
    if (pGrass)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxTextureSet(pGrass, 0, 0);
        AEMtx33Scale(&scale, 1920.0f * gScaleX, 1080.0f * gScaleY);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMeshGrass, AE_GFX_MDM_TRIANGLES);
    }

    Crate_Draw();
    UI_DrawCrateHoverTint_Yellow();
    /*MainScreen_DrawCrateFruits();*/
    RenderSpawnFruits();
    Helper_Draw();

    if (TR_IsActive())
    {
        float alpha = TR_GetAlpha();
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(0, 0, 0, alpha);
        AEMtx33Scale(&scale, 1600, 900);
        AEMtx33Trans(&trans, 0, 0);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
    }

    UI_Draw();
    Farm_Render();
    UI_DrawFruitBasketTooltips();
    UI_DrawPlotTooltips();

    if (gStartScreenActive)
        StartScreen_Draw();

    // ---------------------------------------------------------------
    // Key Legend — glass style, flush right border
    // ---------------------------------------------------------------
    if (g_legendFontKey >= 0 && !gStartScreenActive && !g_pauseOpen && !g_rewardPopupOpen)
    {
        const float HW = 800.0f;
        const float HH = 450.0f;

        const float PNL_W = 260.0f;
        const float PNL_H = 380.0f;
        const float PNL_X = HW - PNL_W * 0.5f;
        const float PNL_Y = 0.0f;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxPrint(g_legendFontDesc, g_legendOpen ? "[H] Hide" : "[H] Controls",
            (PNL_X - PNL_W * 0.46f) / HW,
            (PNL_Y - PNL_H * 0.46f) / HH,
            0.65f, 0.0f, 0.0f, 0.0f, 0.80f);

        if (g_legendOpen)
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 0.25f);
            AEMtx33Scale(&scale, PNL_W, PNL_H);
            AEMtx33Trans(&trans, PNL_X, PNL_Y);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);

            const float txtScale = 0.85f;
            const float keyX = (PNL_X - PNL_W * 0.46f) / HW;
            const float descX = (PNL_X - PNL_W * 0.02f) / HW;

            AEGfxPrint(g_legendFontKey, "CONTROLS",
                (PNL_X - PNL_W * 0.40f) / HW,
                (PNL_Y + PNL_H * 0.40f) / HH,
                txtScale + 0.10f, 0.0f, 0.0f, 0.0f, 1.0f);

            struct { const char* key; const char* desc; }
            static const E[] = {
                { "[M]",     "Menu"        },
                { "[ESC]",   "Pause"       },
                { "[CLICK]", "Interact"    },
                { "[SPACE]", "Collect"     },
                { "[E]",     "Exit Rhythm" },
            };
            const int N = 5;
            const float rowTop = PNL_Y + PNL_H * 0.26f;
            const float rowSpacing = PNL_H / (N + 1.8f);

            for (int i = 0; i < N; i++)
            {
                const float ry = (rowTop - i * rowSpacing) / HH;
                AEGfxPrint(g_legendFontKey, E[i].key, keyX, ry, txtScale, 0.0f, 0.0f, 0.0f, 1.0f);
                AEGfxPrint(g_legendFontDesc, E[i].desc, descX, ry, txtScale, 0.0f, 0.0f, 0.0f, 0.80f);
            }
        }
    }


    // ---------------------------------------------------------------
    // Status Toasts — TOP CENTER with background, all game messages
    // ---------------------------------------------------------------
    if (fontId >= 0 && !gStartScreenActive && !g_pauseOpen && !g_rewardPopupOpen)
    {
        const float HH = 450.0f;
        const float TOAST_TOP_Y = 380.0f;
        const float TOAST_ROW_SPACING = 58.0f;
        const float TOAST_TEXT_SCALE = 1.0f;

        s8 toastFont = (g_toastFont >= 0) ? g_toastFont : ((g_legendFontDesc >= 0) ? g_legendFontDesc : fontId);

        int row = 0;
        for (int i = 0; i < TOAST_MAX; i++)
        {
            if (!g_toasts[i].active) continue;

            float age = g_toasts[i].timer;
            float alpha = 1.0f;
            if (age > TOAST_LIFETIME)
                alpha = 1.0f - (age - TOAST_LIFETIME) / TOAST_FADETIME;
            alpha = (alpha < 0.0f) ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);

            float worldY = TOAST_TOP_Y - row * TOAST_ROW_SPACING;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);

            float nx = 0.0f;  // Center horizontally
            float ny = worldY / HH;

            // Shadow for better readability
            AEGfxPrint(toastFont, g_toasts[i].text, nx - 0.002f, ny - 0.002f,
                TOAST_TEXT_SCALE,
                0.0f, 0.0f, 0.0f,
                alpha * 0.8f);

            // Main text - brighter colors
            AEGfxPrint(toastFont, g_toasts[i].text, nx, ny,
                TOAST_TEXT_SCALE,
                g_toasts[i].r * 1.2f, g_toasts[i].g * 1.2f, g_toasts[i].b * 1.2f,
                alpha);
            row++;
        }
    }

    // ---------------------------------------------------------------
    // Pause popup rendering
    // ---------------------------------------------------------------
    if (g_pauseOpen)
    {
        AEMtx33 pScale, pTrans, pTransform;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.6f);
        AEMtx33Scale(&pScale, 1600.0f, 900.0f);
        AEMtx33Trans(&pTrans, 0.0f, 0.0f);
        AEMtx33Concat(&pTransform, &pTrans, &pScale);
        AEGfxSetTransform(pTransform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        if (pTexPausePanel && pMeshPauseQuad)
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxSetTransparency(1.0f);
            AEGfxTextureSet(pTexPausePanel, 0, 0);
            AEMtx33Scale(&pScale, PAUSE_PANEL_W * gScaleX, PAUSE_PANEL_H * gScaleY);
            AEMtx33Trans(&pTrans, 0.0f, 0.0f);
            AEMtx33Concat(&pTransform, &pTrans, &pScale);
            AEGfxSetTransform(pTransform.m);
            AEGfxMeshDraw(pMeshPauseQuad, AE_GFX_MDM_TRIANGLES);
        }

        if (pTexPauseBtn && pMeshPauseQuad)
        {
            float tint0 = (g_pauseHovered == 0) ? 1.3f : 1.0f;
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(tint0, tint0, tint0, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxSetTransparency(1.0f);
            AEGfxTextureSet(pTexPauseBtn, 0, 0);
            AEMtx33Scale(&pScale, PAUSE_BTN_W * gScaleX, PAUSE_BTN_H * gScaleY);
            AEMtx33Trans(&pTrans, 0.0f, PAUSE_BTN_Y0 * gScaleY);
            AEMtx33Concat(&pTransform, &pTrans, &pScale);
            AEGfxSetTransform(pTransform.m);
            AEGfxMeshDraw(pMeshPauseQuad, AE_GFX_MDM_TRIANGLES);
        }

        if (pTexPauseBtn && pMeshPauseQuad)
        {
            float tint1 = (g_pauseHovered == 1) ? 1.3f : 1.0f;
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(tint1, tint1, tint1, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxSetTransparency(1.0f);
            AEGfxTextureSet(pTexPauseBtn, 0, 0);
            AEMtx33Scale(&pScale, PAUSE_BTN_W * gScaleX, PAUSE_BTN_H * gScaleY);
            AEMtx33Trans(&pTrans, 0.0f, PAUSE_BTN_Y1 * gScaleY);
            AEMtx33Concat(&pTransform, &pTrans, &pScale);
            AEGfxSetTransform(pTransform.m);
            AEGfxMeshDraw(pMeshPauseQuad, AE_GFX_MDM_TRIANGLES);
        }

        if (fontId >= 0)
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxPrint(fontId, "PAUSED", -0.056f, 0.13f, 0.9f, 1.0f, 0.95f, 0.75f, 1.0f);
            AEGfxPrint(fontId, "Main Menu", -0.067f, 0.015f, 0.8f, 0.2f, 0.12f, 0.05f, 1.0f);
            AEGfxPrint(fontId, "Exit", -0.032f, -0.13f, 0.8f, 0.2f, 0.12f, 0.05f, 1.0f);
        }
    }

    // ---------------------------------------------------------------
    // Rhythm Reward Popup
    // ---------------------------------------------------------------
    if (g_rewardPopupOpen && fontId >= 0)
    {
        AEMtx33 rScale, rTrans, rTransform;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.55f);
        AEMtx33Scale(&rScale, 1600.0f, 900.0f);
        AEMtx33Trans(&rTrans, 0.0f, 0.0f);
        AEMtx33Concat(&rTransform, &rTrans, &rScale);
        AEGfxSetTransform(rTransform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        if (pTexPausePanel && pMeshPauseQuad)
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxSetTransparency(1.0f);
            AEGfxTextureSet(pTexPausePanel, 0, 0);
            AEMtx33Scale(&rScale, 420.0f * gScaleX, 230.0f * gScaleY);
            AEMtx33Trans(&rTrans, 0.0f, 0.0f);
            AEMtx33Concat(&rTransform, &rTrans, &rScale);
            AEGfxSetTransform(rTransform.m);
            AEGfxMeshDraw(pMeshPauseQuad, AE_GFX_MDM_TRIANGLES);
        }

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        char buf[256];
        strncpy_s(buf, g_rewardPopupText, sizeof(buf));
        char* ctx = nullptr;
        char* line = strtok_s(buf, "\n", &ctx);
        float lineY = 0.13f;
        while (line)
        {
            AEGfxPrint(fontId, line, -0.19f, lineY, 0.85f, 0.15f, 0.08f, 0.02f, 1.0f);
            lineY -= 0.10f;
            line = strtok_s(nullptr, "\n", &ctx);
        }

        AEGfxPrint(fontId, "Click to continue", -0.16f, lineY - 0.02f,
            0.7f, 0.5f, 0.5f, 0.5f, 1.0f);
    }
}

void MainScreen_Free()
{
    MainBGM_Stop();
    if (AEAudioIsValidAudio(g_mainBGM))
        AEAudioUnloadAudio(g_mainBGM);
    if (AEAudioIsValidGroup(g_mainBGMGroup))
        AEAudioUnloadAudioGroup(g_mainBGMGroup);
    memset(&g_mainBGM, 0, sizeof(AEAudio));
    memset(&g_mainBGMGroup, 0, sizeof(AEAudioGroup));

    if (pMeshBackground) AEGfxMeshFree(pMeshBackground);
    if (pMeshGrass)      AEGfxMeshFree(pMeshGrass);
    if (pMeshStall)      AEGfxMeshFree(pMeshStall);
    if (pMeshFruit)      AEGfxMeshFree(pMeshFruit);

    if (pBackground) AEGfxTextureUnload(pBackground);
    if (pTexApple)   AEGfxTextureUnload(pTexApple);
    if (pTexPear)    AEGfxTextureUnload(pTexPear);
    if (pTexBanana)  AEGfxTextureUnload(pTexBanana);
    if (pTexFruitApple) AEGfxTextureUnload(pTexFruitApple);

    if (fontId >= 0)
    {
        AEGfxDestroyFont(fontId);
        fontId = -1;
    }

    if (g_legendFontKey >= 0 && g_legendFontKey != fontId)
    {
        AEGfxDestroyFont(g_legendFontKey);
        g_legendFontKey = -1;
    }
    if (g_legendFontDesc >= 0 && g_legendFontDesc != g_legendFontKey && g_legendFontDesc != fontId)
    {
        AEGfxDestroyFont(g_legendFontDesc);
        g_legendFontDesc = -1;
    }

    if (pTexPausePanel) { AEGfxTextureUnload(pTexPausePanel); pTexPausePanel = nullptr; }
    if (pTexPauseBtn) { AEGfxTextureUnload(pTexPauseBtn);   pTexPauseBtn = nullptr; }
    if (pMeshPauseQuad) { AEGfxMeshFree(pMeshPauseQuad);      pMeshPauseQuad = nullptr; }
    if (pGrass) { AEGfxTextureUnload(pGrass);   pGrass = nullptr; }
    if (pTexPlus) { AEGfxTextureUnload(pTexPlus); pTexPlus = nullptr; }

    pMeshBackground = pMeshGrass = pMeshStall = pMeshFruit = nullptr;
    pBackground = pTexApple = pTexPear = pTexBanana = nullptr;
}

void MainScreen_Unload()
{
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
#ifdef _DEBUG
    // _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    int gGameRunning = 1;

    AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL);
    AESysSetWindowTitle("Fruit Stall Game");
    AESysReset();
    AEGfxFontSystemStart();

    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 1, 0.5f, -0.5f, 0xFFFFFFFF, 1, 1, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1, 1, 0.5f, 0.5f, 0xFFFFFFFF, 1, 0, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    g_pMeshFullScreen = AEGfxMeshEnd();

    GSM_Initialize(GS_SPLASH);

    while (gGameRunning)
    {
        AESysFrameStart();

        if ((AEInputCheckTriggered(AEVK_ESCAPE)
            && !ProfileScreen_IsPopupActive()
            && currentState != GS_MAIN_SCREEN
            && currentState != GS_NEXT_SCREEN
            && currentState != GS_RHYTHM_SCREEN)
            || 0 == AESysDoesWindowExist())
        {
            nextState = GS_EXIT;
        }

        if (currentState == GS_RHYTHM_SCREEN)
        {
            if (AEInputCheckTriggered(AEVK_E))
            {
                GrantRhythmReward();
                Farm_OnRhythmResult(true);
                Farm_ClearRhythmRequest();
                nextState = GS_MAIN_SCREEN;
            }

#ifdef _DEBUG
            if (AEInputCheckTriggered(AEVK_TAB))
            {
                Farm_OnRhythmResult(true);
                Farm_ClearRhythmRequest();
                nextState = GS_MAIN_SCREEN;
                OutputDebugStringA("[DEBUG] Rhythm skipped via TAB\n");
            }
#endif

            if (AEInputCheckTriggered(AEVK_ESCAPE))
            {
                Farm_SetRhythmPaused(true);
                Farm_ClearRhythmRequest();
                nextState = GS_MAIN_SCREEN;
            }
        }

        if (nextState != currentState && !TR_IsActive())
        {
            TR_Start(currentState, nextState);
        }

        if (TR_Update())
        {
            if (fpFree)   fpFree();
            if (currentState != GS_EXIT && fpUnload) fpUnload();

            previousState = currentState;
            currentState = nextState;
            GSM_Update();

            if (currentState != GS_EXIT)
            {
                if (fpLoad)       fpLoad();
                if (fpInitialize) fpInitialize();
            }
        }

        if (currentState != GS_EXIT)
        {
            if (fpUpdate) fpUpdate();
            if (fpDraw)   fpDraw();
        }
        else
        {
            gGameRunning = 0;
        }

        AESysFrameEnd();
    }

    AESysExit();
    return 0;
}