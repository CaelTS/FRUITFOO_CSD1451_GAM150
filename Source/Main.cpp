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

// ---------------------------------------------------------------------------
// Graphics Resources

s8 fontId = -1;
AEGfxTexture* pTexStall = NULL;
AEGfxVertexList* pMeshStall = NULL;
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
	pTexStall = AEGfxTextureLoad("Assets/Stall_Empty_POT.png");
	if (!pTexStall) OutputDebugStringA("ERROR: Failed to load 'Assets/Stall_Empty_POT.png'.\n");

	Farm_Load();
}

void MainScreen_Initialize()
{
	UI_Init();

	if (previousState != GS_RHYTHM_SCREEN)
	{
		Farm_Initialize();
	}

	// Load font
	fontId = AEGfxCreateFont("Assets/liberation-mono.ttf", 26);
	if (fontId < 0)
		OutputDebugStringA("ERROR: Failed to load 'Assets/liberation-mono.ttf'.\n");

	// Create stall mesh
	AEGfxMeshStart();
	AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	pMeshStall = AEGfxMeshEnd();
}

void MainScreen_Update()
{
	UI_Input();

	// If the menu is open, skip all gameplay input so nothing fires underneath
	if (UI_IsMenuOpen())
	{
		Farm_Update();
		return;
	}

	Farm_Update();

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

	// --- Draw Stall (Background) ---
	if (pTexStall)
	{
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
		AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetTransparency(1.0f);
		AEGfxTextureSet(pTexStall, 0, 0);
	}
	else
	{
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	}

	AEMtx33Scale(&scale, 1600.0f, 900.0f);
	AEMtx33Trans(&trans, 0.0f, 0.0f);
	AEMtx33Concat(&transform, &trans, &scale);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(pMeshStall, AE_GFX_MDM_TRIANGLES);

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
	if (pMeshStall) AEGfxMeshFree(pMeshStall);
	if (pTexStall)  AEGfxTextureUnload(pTexStall);

	if (fontId >= 0)
	{
		AEGfxDestroyFont(fontId);
		fontId = -1;
	}

	pMeshStall = nullptr;
	pTexStall = nullptr;
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

	GSM_Initialize(GS_MAIN_SCREEN);

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