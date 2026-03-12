// ---------------------------------------------------------------------------
// includes

#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include "Main.h"
#include "GameStateManager.h"
#include "Transition.h"
#include <stdio.h>
#include <vector>
#include "UI.h"
#include "Rhythm.h"
#include "Farm.h"
#include <iostream>
#include "Profile.h"
#include "StartScreen.h"
#include "Economy.h"
#include "SpawnFruits.h"

// ---------------------------------------------------------------------------
// Graphics Resources

// Scale factors (1920x1080 -> 1600x900) - referenced by StartScreen.cpp and others
float gScaleX = 1600.0f / 1920.0f;
float gScaleY = 900.0f / 1080.0f;

s8 fontId = -1;

// Background textures from first Main.cpp
AEGfxTexture* pBackground = NULL;
AEGfxTexture* pGrass = NULL;
AEGfxTexture* pBaseStall = NULL;

// Fruit textures (kept for potential use)
AEGfxTexture* pTexApple = NULL;
AEGfxTexture* pTexPear = NULL;
AEGfxTexture* pTexBanana = NULL;
AEGfxTexture* pTexPlus = NULL;

// Meshes
AEGfxVertexList* pMeshBackground = NULL;
AEGfxVertexList* pMeshGrass = NULL;
AEGfxVertexList* pMeshStall = NULL;
AEGfxVertexList* pMeshFruit = NULL;
AEGfxVertexList* g_pMeshFullScreen = NULL;

// Fruit basket (hover) - kept for UI tooltip system
std::vector<FruitBasket> gFruitBaskets;
const std::vector<FruitBasket>& GetFruitBaskets()
{
	return gFruitBaskets;
}

// ---------------------------------------------------------------------------
// MainScreen Lifecycle

void MainScreen_Load()
{
	// Load background textures from first Main.cpp
	pBackground = AEGfxTextureLoad("Assets/MainMenu_Background.png");
	pGrass = AEGfxTextureLoad("Assets/MainMenu_Background_Grass.png");
	pBaseStall = AEGfxTextureLoad("Assets/base level 1 with apple.png");

	pTexApple = AEGfxTextureLoad("Assets/Apple.png");
	pTexPear = AEGfxTextureLoad("Assets/Pear.png");
	pTexBanana = AEGfxTextureLoad("Assets/Banana.png");
	pTexPlus = AEGfxTextureLoad("Assets/Plus.png");

	if (!pBackground) OutputDebugStringA("ERROR: Failed to load 'Assets/MainMenu_Background.png'.\n");
	if (!pGrass) OutputDebugStringA("ERROR: Failed to load 'Assets/MainMenu_Background_Grass.png'.\n");
	if (!pBaseStall) OutputDebugStringA("ERROR: Failed to load 'Assets/base level 1 with apple.png'.\n");
	if (!pTexApple) OutputDebugStringA("ERROR: Failed to load 'Assets/Apple.png'.\n");
	if (!pTexPear) OutputDebugStringA("ERROR: Failed to load 'Assets/Pear.png'.\n");
	if (!pTexBanana) OutputDebugStringA("ERROR: Failed to load 'Assets/Banana.png'.\n");

	Farm_Load();
}

void MainScreen_Initialize()
{
	UI_Init();

	// Economy Init
	Economy_Init();

	// Spawn Fruits Init
	SpawnFruit_Init();

	if (previousState != GS_RHYTHM_SCREEN)
	{
		Farm_Initialize();
	}

	// Build crate rectangles to match the stall's current transform:
	{
		float stallW = 702.0f * gScaleX;
		float stallH = 716.0f * gScaleY;
		float stallX = 330.0f * gScaleX;
		float stallY = -15.0f * gScaleY;
		UI_RebuildCrateHitboxesFromStall(stallX, stallY, stallW, stallH);
	}

	// Load font
	fontId = AEGfxCreateFont("Assets/Crayon pastel.otf", 26);
	if (fontId < 0)
		OutputDebugStringA("ERROR: Failed to load 'Assets/Crayon pastel.otf'.\n");

	// Create meshes (from first Main.cpp)
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
	// Get Delta Time
	float dt = (float)AEFrameRateControllerGetFrameTime();

	UI_Input();

	// If the menu is open, skip all gameplay input so nothing fires underneath
	if (UI_IsMenuOpen())
	{
		Farm_Update();
		Economy_Update(dt);
		return;
	}

	Farm_Update();
	Economy_Update(dt);

	// Update spawned fruits
	UpdateSpawnFruits(dt);
	UpdateFruitSpawner(dt);

	// Get mouse position for fruit clicks
	s32 mouseX, mouseY;
	AEInputGetCursorPosition(&mouseX, &mouseY);
	CheckForFruitClicks(mouseX, mouseY);

	// ---- Check if farm triggered rhythm ----
	if (Farm_ShouldStartRhythm())
	{
		OutputDebugStringA("Farm requested rhythm game\n");
		Farm_ClearRhythmRequest();
		nextState = GS_RHYTHM_SCREEN;
	}

	if (AEInputCheckTriggered(AEVK_N))
	{
		nextState = GS_NEXT_SCREEN;
	}

	// Switch to Rhythm game when pressing R (debug shortcut, main screen only)
	if (AEInputCheckTriggered(AEVK_R) && currentState == GS_MAIN_SCREEN)
	{
		OutputDebugStringA("Switching to RHYTHM state\n");
		nextState = GS_RHYTHM_SCREEN;
	}
}

void MainScreen_Render()
{
	AEGfxSetBackgroundColor(0.2f, 0.2f, 0.2f);

	AEMtx33 scale, trans, transform;

	// --- Draw Background (Center) ---
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

	// Scale: 1920x1080 pixels (Full Screen)
	AEMtx33Scale(&scale, 1920.0f * gScaleX, 1080.0f * gScaleY);
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

		// Set the texture
		AEGfxTextureSet(pBaseStall, 0, 0);

		// Scale (size of stall)
		AEMtx33Scale(&scale, 702.0f * gScaleX, 716.0f * gScaleY);

		// Position
		AEMtx33Trans(&trans, 330.0f * gScaleX, -15.0f * gScaleY);

		// Combine scale and translation
		AEMtx33Concat(&transform, &trans, &scale);

		// Apply transformation
		AEGfxSetTransform(transform.m);

		// Draw mesh
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

		// Set the texture
		AEGfxTextureSet(pGrass, 0, 0);

		// Scale (size of grass)
		AEMtx33Scale(&scale, 1920.0f * gScaleX, 1080.0f * gScaleY);

		// Position (center of screen)
		AEMtx33Trans(&trans, 0.0f, 0.0f);

		// Combine scale and translation
		AEMtx33Concat(&transform, &trans, &scale);

		// Apply transformation
		AEGfxSetTransform(transform.m);

		// Draw mesh
		AEGfxMeshDraw(pMeshGrass, AE_GFX_MDM_TRIANGLES);
	}

	// Crate hover tint
	UI_DrawCrateHoverTint_Yellow();

	// Draw spawned fruits
	RenderSpawnFruits();

	// --- Transition overlay ---
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
}

void MainScreen_Free()
{
	// Free meshes
	if (pMeshBackground) AEGfxMeshFree(pMeshBackground);
	if (pMeshGrass) AEGfxMeshFree(pMeshGrass);
	if (pMeshStall) AEGfxMeshFree(pMeshStall);
	if (pMeshFruit) AEGfxMeshFree(pMeshFruit);

	// Free textures
	if (pBackground)  AEGfxTextureUnload(pBackground);
	if (pGrass)       AEGfxTextureUnload(pGrass);
	if (pBaseStall)   AEGfxTextureUnload(pBaseStall);
	if (pTexApple)    AEGfxTextureUnload(pTexApple);
	if (pTexPear)     AEGfxTextureUnload(pTexPear);
	if (pTexBanana)   AEGfxTextureUnload(pTexBanana);
	if (pTexPlus)     AEGfxTextureUnload(pTexPlus);

	// Free font
	if (fontId >= 0)
	{
		AEGfxDestroyFont(fontId);
		fontId = -1;
	}

	// Reset pointers
	pMeshBackground = pMeshGrass = pMeshStall = pMeshFruit = nullptr;
	pBackground = pGrass = pBaseStall = pTexApple = pTexPear = pTexBanana = pTexPlus = nullptr;
}

void MainScreen_Unload()
{
	// Nothing to unload at this time
	// Save/load will be handled by the dedicated state when implemented
}

// ---------------------------------------------------------------------------
// WinMain / Game Loop

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

	// Window Size: 1600x900
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL);
	AESysSetWindowTitle("Fruit Stall Game");
	AESysReset();
	AEGfxFontSystemStart();

	// Create shared full-screen quad
	AEGfxMeshStart();
	AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 1, 0.5f, -0.5f, 0xFFFFFFFF, 1, 1, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
	AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1, 1, 0.5f, 0.5f, 0xFFFFFFFF, 1, 0, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
	g_pMeshFullScreen = AEGfxMeshEnd();

	GSM_Initialize(GS_START_SCREEN);

	while (gGameRunning)
	{
		AESysFrameStart();

		// Exit check
		if ((AEInputCheckTriggered(AEVK_ESCAPE) && !ProfileScreen_IsPopupActive()) || 0 == AESysDoesWindowExist())
		{
			nextState = GS_EXIT;
		}

		// Return from rhythm - only active while in the rhythm screen
		if (currentState == GS_RHYTHM_SCREEN && AEInputCheckTriggered(AEVK_E))
		{
			bool success = (Rhythm_GetScore().misses < 3);
			Farm_OnRhythmResult(success);
			nextState = GS_MAIN_SCREEN;
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