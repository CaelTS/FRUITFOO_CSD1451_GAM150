// ---------------------------------------------------------------------------
// includes

#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include <stdio.h>

// ---------------------------------------------------------------------------
// main

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	int gGameRunning = 1;

	// ---------------------------------------------------------------------------
	// Initialization
	// ---------------------------------------------------------------------------

	// Using custom window procedure
	// Window Size: 1600x900
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL);

	// Changing the window title
	AESysSetWindowTitle("Fruit Stall Game");

	// reset the system modules
	AESysReset();

	// Initialize Font System
	AEGfxFontSystemStart();

	// Game Variables
	int gold = 0;
	int energy = 50;           // Changed to int for display, max 50
	const int MAX_ENERGY = 50; // Capacity is 50
	const int ENERGY_COST = 5; // Consumes 5 energy
	const int APPLE_PRICE = 10;// Gives 10 gold

	// Energy Regeneration
	float energyTimer = 0.0f;
	const float REGEN_TIME = 10.0f; // 10 seconds

	// Load the font. MAKE SURE "Assets/liberation-mono.ttf" IS IN YOUR EXE FOLDER!
	s8 fontId = AEGfxCreateFont("Assets/liberation-mono.ttf", 26); // Increased font size slightly

	// Error Checking for Font
	bool fontLoaded = (fontId >= 0);
	if (!fontLoaded)
	{
		OutputDebugStringA("ERROR: Failed to load 'Assets/liberation-mono.ttf'. Text will not be displayed.\n");
	}

	// Load Textures
	// MAKE SURE "Assets/Stall_POT.png" IS IN YOUR EXE FOLDER!
	AEGfxTexture* pTexStall = AEGfxTextureLoad("Assets/Stall_POT.png");
	if (!pTexStall)
	{
		OutputDebugStringA("ERROR: Failed to load 'Assets/Stall_POT.png'. Stall will be untextured.\n");
	}

	// Mesh variables
	AEGfxVertexList* pMeshStall = 0;
	AEGfxVertexList* pMeshApple = 0;
	AEGfxVertexList* pMeshUIBorder = 0; // Shared mesh for UI borders

	// 1. Create Stall Mesh (Standard Unit Square for Sprites)
	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	AEGfxTriAdd(
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
		0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	pMeshStall = AEGfxMeshEnd();

	// 2. Create Apple Mesh (Red Diamond/Circle approximation)
	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xFFFF0000, 0.0f, 1.0f,
		0.5f, -0.5f, 0xFFFF0000, 1.0f, 1.0f,
		-0.5f, 0.5f, 0xFFFF0000, 0.0f, 0.0f);
	AEGfxTriAdd(
		0.5f, -0.5f, 0xFFFF0000, 1.0f, 1.0f,
		0.5f, 0.5f, 0xFFFF0000, 1.0f, 0.0f,
		-0.5f, 0.5f, 0xFFFF0000, 0.0f, 0.0f);
	pMeshApple = AEGfxMeshEnd();

	// 3. Create UI Border Mesh (White Unit Square, we will color it per instance)
	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	AEGfxTriAdd(
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
		0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	pMeshUIBorder = AEGfxMeshEnd();

	// String buffer for text
	char strBuffer[100];

	// ---------------------------------------------------------------------------
	// Game Loop
	// ---------------------------------------------------------------------------
	while (gGameRunning)
	{
		// Informing the system about the loop's start
		AESysFrameStart();

		// Basic way to trigger exiting the application
		if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
			gGameRunning = 0;

		// ---------------------------------------------------------------------------
		// Update Logic
		// ---------------------------------------------------------------------------

		// Get Delta Time
		float dt = (float)AEFrameRateControllerGetFrameTime();

		// Energy Regeneration Logic
		if (energy < MAX_ENERGY)
		{
			energyTimer += dt;
			if (energyTimer >= REGEN_TIME)
			{
				energy++;
				energyTimer -= REGEN_TIME; // Keep remainder
				if (energy > MAX_ENERGY) energy = MAX_ENERGY;
			}
		}

		// Check for "Activity" (Selling Apple)
		if (AEInputCheckTriggered(AEVK_SPACE))
		{
			if (energy >= ENERGY_COST)
			{
				energy -= ENERGY_COST;
				gold += APPLE_PRICE;
			}
		}

		// ---------------------------------------------------------------------------
		// Rendering Logic
		// ---------------------------------------------------------------------------

		// Clear screen
		AEGfxSetBackgroundColor(0.2f, 0.2f, 0.2f); // Dark Gray

		// Transformation Matrices
		AEMtx33 scale, trans, transform;

		// --- Draw Stall (Center) ---
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

		// Scale: 1600x900 pixels (Full Screen)
		AEMtx33Scale(&scale, 1600.0f, 900.0f);
		AEMtx33Trans(&trans, 0.0f, 0.0f);
		AEMtx33Concat(&transform, &trans, &scale);

		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(pMeshStall, AE_GFX_MDM_TRIANGLES);

		// --- Draw Apple (On Stall) ---
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxSetBlendMode(AE_GFX_BM_NONE);

		// Scale: 30x30 pixels
		AEMtx33Scale(&scale, 30.0f, 30.0f);
		AEMtx33Trans(&trans, 0.0f, 20.0f);
		AEMtx33Concat(&transform, &trans, &scale);

		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(pMeshApple, AE_GFX_MDM_TRIANGLES);

		// --- Draw UI Borders ---
		// Screen Dimensions: 1600x900
		// Top Left is roughly (-800, 450) in world coordinates

		float uiX = -700.0f; // 100px padding from left edge
		float uiY_Gold = 400.0f; // 50px padding from top edge
		float uiY_Energy = 340.0f; // 60px below Gold
		float uiWidth = 250.0f;
		float uiHeight = 50.0f;
		float borderSize = 4.0f;

		// 1. Gold Border (Yellow)
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f); // Yellow Tint

		AEMtx33Scale(&scale, uiWidth, uiHeight);
		AEMtx33Trans(&trans, uiX, uiY_Gold);
		AEMtx33Concat(&transform, &trans, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(pMeshUIBorder, AE_GFX_MDM_TRIANGLES);

		// 1b. Gold Background (Black)
		AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f); // Black Tint
		AEMtx33Scale(&scale, uiWidth - (borderSize * 2), uiHeight - (borderSize * 2));
		AEMtx33Trans(&trans, uiX, uiY_Gold);
		AEMtx33Concat(&transform, &trans, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(pMeshUIBorder, AE_GFX_MDM_TRIANGLES);


		// 2. Energy Border (Green)
		AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f); // Green Tint

		AEMtx33Scale(&scale, uiWidth, uiHeight);
		AEMtx33Trans(&trans, uiX, uiY_Energy);
		AEMtx33Concat(&transform, &trans, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(pMeshUIBorder, AE_GFX_MDM_TRIANGLES);

		// 2b. Energy Background (Black)
		AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f); // Black Tint
		AEMtx33Scale(&scale, uiWidth - (borderSize * 2), uiHeight - (borderSize * 2));
		AEMtx33Trans(&trans, uiX, uiY_Energy);
		AEMtx33Concat(&transform, &trans, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(pMeshUIBorder, AE_GFX_MDM_TRIANGLES);

		// Reset Color Multiplier for other things
		AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

		// --- Draw Text (Top Left) ---
		if (fontLoaded)
		{
			// Normalized Coordinates for Text (-1.0 to 1.0)
			// X: -700 / 800 = -0.875
			// Y Gold: 400 / 450 = 0.88
			// Y Energy: 340 / 450 = 0.75

			// 1. Gold Display
			sprintf_s(strBuffer, "Gold: %d", gold);
			// Offset X slightly left to center text in box (-0.95 start)
			AEGfxPrint(fontId, strBuffer, -1.0f, 0.87f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f);

			// 2. Energy Display
			sprintf_s(strBuffer, "Energy: %d/%d", energy, MAX_ENERGY);
			AEGfxPrint(fontId, strBuffer, -1.0f, 0.74f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
		}

		// Informing the system about the loop's end
		AESysFrameEnd();
	}

	// ---------------------------------------------------------------------------
	// Cleanup
	// ---------------------------------------------------------------------------

	AEGfxMeshFree(pMeshStall);
	AEGfxMeshFree(pMeshApple);
	AEGfxMeshFree(pMeshUIBorder);

	if (pTexStall)
	{
		AEGfxTextureUnload(pTexStall);
	}

	// FIX: Do NOT manually destroy the font or shutdown the font system here.
	// AESysExit() likely handles the entire graphics subsystem teardown.

	// free the system
	AESysExit();
}


