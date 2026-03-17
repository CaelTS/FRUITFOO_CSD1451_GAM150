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
static int  g_pauseHovered = -1;

static const float PAUSE_PANEL_W = 400.0f;
static const float PAUSE_PANEL_H = 260.0f;
static const float PAUSE_BTN_W = 300.0f;
static const float PAUSE_BTN_H = 55.0f;
static const float PAUSE_BTN_Y0 = 20.0f;
static const float PAUSE_BTN_Y1 = -60.0f;

void MainScreen_Load()
{
	pBackground = AEGfxTextureLoad("Assets/MainMenu_Background.png");
	pGrass = AEGfxTextureLoad("Assets/MainMenu_Background_Grass.png");
	pBaseStall = AEGfxTextureLoad("Assets/base level 1 with apple.png");
	pTexApple = AEGfxTextureLoad("Assets/Apple.png");
	pTexPear = AEGfxTextureLoad("Assets/Pear.png");
	pTexBanana = AEGfxTextureLoad("Assets/Banana.png");
	pTexPlus = AEGfxTextureLoad("Assets/Plus.png");
	Farm_Load();

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
	// Start Screen Init
	if (previousState == GS_NEXT_SCREEN && !Profile_WentBack())
	{
		gStartScreenActive = false;
		startScreenActive = false;
	}
	else if (previousState == GS_RHYTHM_SCREEN)
	{
		// Returning from rhythm game — skip start screen entirely
		gStartScreenActive = false;
		startScreenActive = false;
	}
	else
	{
		gStartScreenActive = true;
		StartScreen_Init();
	}

	// Reset pause popup state
	g_pauseOpen = false;
	g_pauseHovered = -1;

	Economy_Init();
	SpawnFruit_Init();
	UI_Init();

	if (previousState != GS_RHYTHM_SCREEN)
	{
		Farm_Initialize();
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

	AEGfxMeshStart();
	AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	pMeshBackground = AEGfxMeshEnd();

	AEGfxMeshStart();
	AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	pMeshGrass = AEGfxMeshEnd();

	AEGfxMeshStart();
	AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	pMeshStall = AEGfxMeshEnd();

	AEGfxMeshStart();
	AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	pMeshFruit = AEGfxMeshEnd();
}

void MainScreen_Update()
{
	float dt = (float)AEFrameRateControllerGetFrameTime();

	if (gStartScreenActive)
	{
		StartScreen_Update(dt);
		if (!StartScreen_IsActive())
			gStartScreenActive = false;
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
				nextState = GS_START_SCREEN;
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

	// UI only runs when no farm prompt is visible
	if (!Farm_IsRhythmPaused() && !Farm_IsWaitingForRhythm())
	{
		UI_Input();
	}

	Economy_Update(dt);
	UpdateSpawnFruits(dt);
	UpdateFruitSpawner(dt);
	CheckForFruitClicks(mouseX, mouseY);

	// Check if farm triggered rhythm
	if (Farm_ShouldStartRhythm())
	{
		OutputDebugStringA("Farm requested rhythm game\n");
		Farm_ClearRhythmFlag();
		nextState = GS_RHYTHM_SCREEN;
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
	RenderSpawnFruits();

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
}

void MainScreen_Free()
{
	if (pMeshBackground) AEGfxMeshFree(pMeshBackground);
	if (pMeshGrass)      AEGfxMeshFree(pMeshGrass);
	if (pMeshStall)      AEGfxMeshFree(pMeshStall);
	if (pMeshFruit)      AEGfxMeshFree(pMeshFruit);

	if (pBackground) AEGfxTextureUnload(pBackground);
	if (pTexApple)   AEGfxTextureUnload(pTexApple);
	if (pTexPear)    AEGfxTextureUnload(pTexPear);
	if (pTexBanana)  AEGfxTextureUnload(pTexBanana);

	if (fontId >= 0)
	{
		AEGfxDestroyFont(fontId);
		fontId = -1;
	}

	// Free pause popup assets
	if (pTexPausePanel) { AEGfxTextureUnload(pTexPausePanel); pTexPausePanel = nullptr; }
	if (pTexPauseBtn) { AEGfxTextureUnload(pTexPauseBtn);   pTexPauseBtn = nullptr; }
	if (pMeshPauseQuad) { AEGfxMeshFree(pMeshPauseQuad);     pMeshPauseQuad = nullptr; }

	pMeshBackground = pMeshStall = pMeshFruit = nullptr;
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

	GSM_Initialize(GS_MAIN_SCREEN);

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
				Farm_OnRhythmResult(true);
				Farm_ClearRhythmRequest();
				nextState = GS_MAIN_SCREEN;
			}

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