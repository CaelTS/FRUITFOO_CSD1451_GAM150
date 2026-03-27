// ---------------------------------------------------------------------------
// includes

#include <crtdbg.h> // To check for memory leaks
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
#include "Crate.h"   // <-- ADDED: needed for Crate_GetFruitCount / Crate_IsUnlocked
#include "Inventory.h"
#include "HelperCreatures.h"
#include "Upgrades.h"
#include "AEAudio.h"
#include "UIAudio.h"

// ============================================================
// Main screen BGM
// ============================================================
static AEAudio      g_mainBGM;
static AEAudioGroup g_mainBGMGroup;

static void MainBGM_Start()
{
	if (AEAudioIsValidAudio(g_mainBGM) && AEAudioIsValidGroup(g_mainBGMGroup))
	{
		AEAudioStopGroup(g_mainBGMGroup);
		AEAudioPlay(g_mainBGM, g_mainBGMGroup, 0.3f, 1.0f, -1); // -1 = infinite loop
	}
}

static void MainBGM_Stop()
{
	if (AEAudioIsValidGroup(g_mainBGMGroup))
		AEAudioStopGroup(g_mainBGMGroup);
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
	// Start screen is now its own GSM state (GS_START_SCREEN).
	// By the time MainScreen_Initialize runs, the start screen is done.
	gStartScreenActive = false;
	startScreenActive = false;

	// Reset pause popup state
	g_pauseOpen = false;
	g_pauseHovered = -1;
	g_returnedFromPause = false;

	Economy_Init();
	SpawnFruit_Init();
	Upgrades_Init();

	// UIAudio_Init loads audio_settings.txt and caches sSFXEnabled / sMusicEnabled.
	// UI_Init then reads those back via UIAudio_SFXEnabled() / UIAudio_MusicEnabled()
	// to sync the visual toggle state (gSoundEnabled / gMusicEnabled).
	UIAudio_Init();
	UI_Init();
	Helper_Init();

	// Apply persisted audio settings to all subsystems.
	// UIAudio_SetMusicEnabled controls both Rhythm AND MainBGM in one call,
	// so we do NOT call MainBGM_SetEnabled separately.
	gSoundEnabled = UIAudio_SFXEnabled();
	gMusicEnabled = UIAudio_MusicEnabled();
	UIAudio_EnableSFX(gSoundEnabled);
	UIAudio_SetMusicEnabled(gMusicEnabled); // starts MainBGM if gMusicEnabled=true


	if (previousState != GS_RHYTHM_SCREEN)
	{
		Farm_Initialize();
		Crate_Initialize();
	}

	// Build crate rectangles to match the stall's current transform
	{
		float stallW = 702.0f * gScaleX;
		float stallH = 716.0f * gScaleY;
		float stallX = 330.0f * gScaleX;
		float stallY = -15.0f * gScaleY;
		UI_RebuildCrateHitboxesFromStall(stallX, stallY, stallW, stallH);
	}

	fontId = AEGfxCreateFont("Assets/Crayon pastel.otf", 26);
	if (fontId < 0)
		OutputDebugStringA("ERROR: Failed to load 'Assets/Crayon pastel.otf'.\n");

	// Legend fonts — Nunito gives a clean, modern game-UI feel.
	// Drop Nunito-SemiBold.ttf and Nunito-Regular.ttf into Assets/.
	// Falls back to the main font if the files aren't present yet.
	g_legendFontKey = AEGfxCreateFont("Assets/Nunito-SemiBold.ttf", 28);
	if (g_legendFontKey < 0)
		g_legendFontKey = fontId;

	g_legendFontDesc = AEGfxCreateFont("Assets/Nunito-Regular.ttf", 28);
	if (g_legendFontDesc < 0)
		g_legendFontDesc = fontId;

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
				MainBGM_Start(); // start screen dismissed — begin BGM now
		}
		return; // block all game input while start screen is active
	}

	s32 mouseX, mouseY;
	AEInputGetCursorPosition(&mouseX, &mouseY);

	// ---------------------------------------------------------------
	// Pause popup (only while in-game, not on start screen)
	// ---------------------------------------------------------------
	if (!gStartScreenActive)
	{
		// Debug: check what's blocking pause
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
			// Update hover using IsMouseOverRect
			g_pauseHovered = -1;
			if (IsMouseOverRect(0.0f, PAUSE_BTN_Y0, PAUSE_BTN_W, PAUSE_BTN_H)) g_pauseHovered = 0;
			else if (IsMouseOverRect(0.0f, PAUSE_BTN_Y1, PAUSE_BTN_W, PAUSE_BTN_H)) g_pauseHovered = 1;

			// Handle clicks
			if (ClickedOnRect(0.0f, PAUSE_BTN_Y0, PAUSE_BTN_W, PAUSE_BTN_H))
			{
				Profile_EndSession();
				g_pauseOpen = false;
				g_returnedFromPause = true;
				MainBGM_Stop(); // going back to start screen — force stop BGM
				StartScreen_Init();         // reset animation + buttons in place
				gStartScreenActive = true; // show overlay without leaving GS_MAIN_SCREEN
			}
			else if (ClickedOnRect(0.0f, PAUSE_BTN_Y1, PAUSE_BTN_W, PAUSE_BTN_H))
			{
				Profile_EndSession();
				g_pauseOpen = false;
				nextState = GS_EXIT;
			}

			// Swallow all other input while paused
			return;
		}
	}

	// Farm gets first pick of all clicks so its prompts aren't stolen by UI
	Farm_Update();

	// Toggle key legend (H)
	if (AEInputCheckTriggered(AEVK_H))
		g_legendOpen = !g_legendOpen;

	UI_Input();  // Farm_Update() already handles its own clicks before this runs

	Economy_Update(dt);
	UpdateSpawnFruits(dt);
	UpdateFruitSpawner(dt);
	Helper_Update(dt);
	CheckForFruitClicks(mouseX, mouseY);

	// Check if farm triggered rhythm
	if (Farm_ShouldStartRhythm())
	{
		OutputDebugStringA("Farm requested rhythm game\n");
		Farm_ClearRhythmFlag();
		MainBGM_Stop(); // leaving for rhythm — force stop BGM
		nextState = GS_RHYTHM_SCREEN;
	}
}

// ---------------------------------------------------------------------------
// Draw fruit icons + count inside each crate bin based on live Crate data
// ---------------------------------------------------------------------------
static void MainScreen_DrawCrateFruits()
{
	const auto& baskets = GetFruitBaskets();
	AEMtx33 scale, trans, transform;

	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetColorToMultiply(1, 1, 1, 1);
	AEGfxSetTransparency(1.0f);

	for (int i = 0; i < (int)baskets.size(); i++)
	{
		if (!Crate_IsUnlocked(i)) continue;

		int count = Crate_GetFruitCount(i);
		if (count <= 0) continue;

		// Pick texture by fruit type (matches crate index: 0=apple, 1=pear, 2=banana)
		AEGfxTexture* tex = nullptr;
		switch (i)
		{
		case 0: tex = pTexFruitApple ? pTexFruitApple : pTexApple; break;
		case 1: tex = pTexPear;   break;
		case 2: tex = pTexBanana; break;
		}
		if (!tex) continue;

		const auto& b = baskets[i];

		// Icon size fits inside the bin
		float iconSize = b.height * 0.65f;

		// Show up to 5 icons spread horizontally across the bin
		int displayCount = (count > 5) ? 5 : count;
		float spread = b.width * 0.55f;
		float spacing = (displayCount > 1) ? spread / (displayCount - 1) : 0.0f;
		float startX = b.x - spread * 0.5f;

		AEGfxTextureSet(tex, 0, 0);

		for (int n = 0; n < displayCount; n++)
		{
			float fx = startX + n * spacing;
			float fy = b.y;

			AEMtx33Scale(&scale, iconSize, iconSize);
			AEMtx33Trans(&trans, fx, fy);
			AEMtx33Concat(&transform, &trans, &scale);
			AEGfxSetTransform(transform.m);
			AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
		}

		// Count label (e.g. "x3") above the bin
		char buf[8];
		sprintf_s(buf, "x%d", count);
		float textX = (b.x - b.width * 0.05f) / 800.0f;
		float textY = (b.y + b.height * 0.65f) / 450.0f;
		AEGfxSetColorToMultiply(0, 0, 0, 1);
		AEGfxPrint(fontId, buf, textX, textY, 0.7f, 0, 0, 0, 1);
		AEGfxSetColorToMultiply(1, 1, 1, 1);
	}
}

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

	UI_DrawCrateHoverTint_Yellow();

	// Draw fruit icons inside crate bins based on live stock counts
	MainScreen_DrawCrateFruits();

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

	if (gStartScreenActive)
		StartScreen_Draw();

	// ---------------------------------------------------------------
	// Key Legend — glass style, flush right border
	// Toggle with [H]. Always hidden during pause / reward popup.
	// ---------------------------------------------------------------
	if (g_legendFontKey >= 0 && !gStartScreenActive && !g_pauseOpen && !g_rewardPopupOpen)
	{
		const float HW = 800.0f;
		const float HH = 450.0f;

		const float PNL_W = 260.0f;
		const float PNL_H = 380.0f;
		const float PNL_X = HW - PNL_W * 0.5f;
		const float PNL_Y = 0.0f;

		// Always draw the [H] hint
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxPrint(g_legendFontDesc, g_legendOpen ? "[H] Hide" : "[H] Controls",
			(PNL_X - PNL_W * 0.46f) / HW,
			(PNL_Y - PNL_H * 0.46f) / HH,
			0.65f, 0.0f, 0.0f, 0.0f, 0.80f);

		if (g_legendOpen)
		{
			// Frosted glass backdrop
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

			// Title — SemiBold
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
	// Pause popup rendering (drawn last so it sits on top)
	// ---------------------------------------------------------------
	if (g_pauseOpen)
	{
		AEMtx33 pScale, pTrans, pTransform;

		// 1. Dim overlay
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.6f);
		AEMtx33Scale(&pScale, 1600.0f, 900.0f);
		AEMtx33Trans(&pTrans, 0.0f, 0.0f);
		AEMtx33Concat(&pTransform, &pTrans, &pScale);
		AEGfxSetTransform(pTransform.m);
		AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

		// 2. Panel background
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

		// 3. "Main Menu" button
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

		// 4. "Exit" button
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

		// 5. Text labels
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
	// Rhythm Reward Popup (drawn on top of everything)
	// ---------------------------------------------------------------
	if (g_rewardPopupOpen && fontId >= 0)
	{
		AEMtx33 rScale, rTrans, rTransform;

		// 1. Dim overlay
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.55f);
		AEMtx33Scale(&rScale, 1600.0f, 900.0f);
		AEMtx33Trans(&rTrans, 0.0f, 0.0f);
		AEMtx33Concat(&rTransform, &rTrans, &rScale);
		AEGfxSetTransform(rTransform.m);
		AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

		// 2. Panel background (reuse pause panel texture)
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

		// 3. Text — split on \n since AEGfxPrint is single-line
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

		// 4. Dismiss hint
		AEGfxPrint(fontId, "Click to continue", -0.16f, lineY - 0.02f,
			0.7f, 0.5f, 0.5f, 0.5f, 1.0f);
	}
}

void MainScreen_Free()
{
	// Stop and unload main BGM
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

	// Free legend fonts (only if they're not sharing the fallback fontId)
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

	// Free pause popup assets
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

	GSM_Initialize(GS_START_SCREEN);

	while (gGameRunning)
	{
		AESysFrameStart();

		// Global ESC to exit
		if ((AEInputCheckTriggered(AEVK_ESCAPE)
			&& !ProfileScreen_IsPopupActive()
			&& currentState != GS_MAIN_SCREEN
			&& currentState != GS_NEXT_SCREEN
			&& currentState != GS_RHYTHM_SCREEN)
			|| 0 == AESysDoesWindowExist())
		{
			nextState = GS_EXIT;
		}

		// RHYTHM GAME INPUT
		if (currentState == GS_RHYTHM_SCREEN)
		{
			// Player completes rhythm normally
			if (AEInputCheckTriggered(AEVK_E))
			{
				GrantRhythmReward();          // grant rewards + build popup text
				Farm_OnRhythmResult(true);
				Farm_ClearRhythmRequest();
				nextState = GS_MAIN_SCREEN;
			}

#ifdef _DEBUG
			// DEBUG: press TAB to skip rhythm (counts as success)
			if (AEInputCheckTriggered(AEVK_TAB))
			{
				Farm_OnRhythmResult(true);
				Farm_ClearRhythmRequest();
				nextState = GS_MAIN_SCREEN;
				OutputDebugStringA("[DEBUG] Rhythm skipped via TAB\n");
			}
#endif

			// Player ESCs out — pause growth and return to farm
			if (AEInputCheckTriggered(AEVK_ESCAPE))
			{
				Farm_SetRhythmPaused(true);
				Farm_ClearRhythmRequest();
				nextState = GS_MAIN_SCREEN;
			}
		}

		// State transition
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