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
	AESysInit(hInstance, nCmdShow, 800, 600, 1, 60, false, NULL);

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
	// Using AEGfxCreateFont as per your API definition
	s8 fontId = AEGfxCreateFont("Assets/liberation-mono.ttf", 20);

	// Error Checking for Font
	bool fontLoaded = (fontId >= 0);
	if (!fontLoaded)
	{
		// If we can't print to screen, at least print to debug console
		OutputDebugStringA("ERROR: Failed to load 'Assets/liberation-mono.ttf'. Text will not be displayed.\n");
	}

	// Mesh variables
	AEGfxVertexList* pMeshStall = 0;
	AEGfxVertexList* pMeshApple = 0;

	// 1. Create Stall Mesh (Brown Square)
	AEGfxMeshStart();
	AEGfxTriAdd(
		-50.0f, -50.0f, 0xFF8B4513, 0.0f, 1.0f,
		50.0f, -50.0f, 0xFF8B4513, 1.0f, 1.0f,
		-50.0f, 50.0f, 0xFF8B4513, 0.0f, 0.0f);
	AEGfxTriAdd(
		50.0f, -50.0f, 0xFF8B4513, 1.0f, 1.0f,
		50.0f, 50.0f, 0xFF8B4513, 1.0f, 0.0f,
		-50.0f, 50.0f, 0xFF8B4513, 0.0f, 0.0f);
	pMeshStall = AEGfxMeshEnd();

	// 2. Create Apple Mesh (Red Diamond/Circle approximation)
	AEGfxMeshStart();
	AEGfxTriAdd(
		-15.0f, -15.0f, 0xFFFF0000, 0.0f, 1.0f,
		15.0f, -15.0f, 0xFFFF0000, 1.0f, 1.0f,
		-15.0f, 15.0f, 0xFFFF0000, 0.0f, 0.0f);
	AEGfxTriAdd(
		15.0f, -15.0f, 0xFFFF0000, 1.0f, 1.0f,
		15.0f, 15.0f, 0xFFFF0000, 1.0f, 0.0f,
		-15.0f, 15.0f, 0xFFFF0000, 0.0f, 0.0f);
	pMeshApple = AEGfxMeshEnd();

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
		AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);

		// Transformation Matrices
		AEMtx33 scale, trans, transform;

		// --- Draw Stall (Center) ---
		AEMtx33Identity(&scale);
		AEMtx33Trans(&trans, 0.0f, 0.0f);
		AEMtx33Concat(&transform, &trans, &scale);

		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(pMeshStall, AE_GFX_MDM_TRIANGLES);

		// --- Draw Apple (On Stall) ---
		AEMtx33Identity(&scale);
		AEMtx33Trans(&trans, 0.0f, 20.0f);
		AEMtx33Concat(&transform, &trans, &scale);

		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(pMeshApple, AE_GFX_MDM_TRIANGLES);

		// --- Draw Resources (Top Left) ---
		// Screen coordinates: Center (0,0). Top Left approx (-400, 300).
		// We use AEGfxPrint for text.

		if (fontLoaded)
		{
			// 1. Gold Display
			sprintf_s(strBuffer, "Gold: %d", gold);
			// x: -0.95, y: 0.90, scale: 1.0, r: 1, g: 1, b: 0 (Yellow), a: 1
			AEGfxPrint(fontId, strBuffer, -0.95f, 0.90f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f);

			// 2. Energy Display
			sprintf_s(strBuffer, "Energy: %d/%d", energy, MAX_ENERGY);
			// x: -0.95, y: 0.80, scale: 1.0, r: 0, g: 1, b: 0 (Green), a: 1
			AEGfxPrint(fontId, strBuffer, -0.95f, 0.80f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
		}

		// Informing the system about the loop's end
		AESysFrameEnd();
	}

	// ---------------------------------------------------------------------------
	// Cleanup
	// ---------------------------------------------------------------------------

	AEGfxMeshFree(pMeshStall);
	AEGfxMeshFree(pMeshApple);

	// FIX: Do NOT manually destroy the font or shutdown the font system here.
	// AESysExit() likely handles the entire graphics subsystem teardown.
	// Manually calling these might be causing a double-free crash.

	// AEGfxDestroyFont(fontId);  // <-- Commented out to prevent crash
	// AEGfxFontSystemEnd();      // <-- Commented out to prevent crash

	// free the system
	AESysExit();
}



