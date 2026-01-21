// ---------------------------------------------------------------------------
// includes

#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include <stdio.h>
#include <vector>

// ---------------------------------------------------------------------------
// main

struct Fruit
{
	int type; // 0: Apple, 1: Pear, 2: Banana
	float x, y;
	bool active;
};

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

	// Fruit Growth Logic
	float fruitGrowthTimer = 0.0f;
	const float FRUIT_GROWTH_TIME = 10.0f; // 1 fruit per 10 seconds
	std::vector<Fruit> fruits;

	// Pre-defined positions for fruits on trees (approximate coordinates based on 1600x900)
	// Left Tree area: x < -400, Right Tree area: x > 400
	struct Point { float x, y; };
	Point treePositions[] = {
		{-600.0f, 200.0f}, {-500.0f, 300.0f}, {-700.0f, 100.0f}, // Left Tree
		{600.0f, 200.0f}, {500.0f, 300.0f}, {700.0f, 100.0f},    // Right Tree
		{-550.0f, 150.0f}, {650.0f, 250.0f}
	};
	int maxFruits = sizeof(treePositions) / sizeof(Point);

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

<<<<<<< Updated upstream
	// Mesh variables
	AEGfxVertexList* pMeshStall = 0;
	AEGfxVertexList* pMeshApple = 0;
=======
	// Load Textures
	// MAKE SURE THESE FILES ARE IN YOUR EXE FOLDER!
	AEGfxTexture* pTexStall = AEGfxTextureLoad("Assets/Stall_Empty_POT.png");
	AEGfxTexture* pTexApple = AEGfxTextureLoad("Assets/Apple.png");
	AEGfxTexture* pTexPear = AEGfxTextureLoad("Assets/Pear.png");
	AEGfxTexture* pTexBanana = AEGfxTextureLoad("Assets/Banana.png");

	if (!pTexStall) OutputDebugStringA("ERROR: Failed to load 'Assets/Stall_Empty_POT.png'.\n");
	if (!pTexApple) OutputDebugStringA("ERROR: Failed to load 'Assets/Apple.png'.\n");
	if (!pTexPear) OutputDebugStringA("ERROR: Failed to load 'Assets/Pear.png'.\n");
	if (!pTexBanana) OutputDebugStringA("ERROR: Failed to load 'Assets/Banana.png'.\n");

	// Mesh variables
	AEGfxVertexList* pMeshStall = 0;
	AEGfxVertexList* pMeshFruit = 0;
	AEGfxVertexList* pMeshUIBorder = 0; // Shared mesh for UI borders
>>>>>>> Stashed changes

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

	// 2. Create Fruit Mesh (Standard Unit Square)
	AEGfxMeshStart();
	AEGfxTriAdd(
<<<<<<< Updated upstream
		-15.0f, -15.0f, 0xFFFF0000, 0.0f, 1.0f,
		15.0f, -15.0f, 0xFFFF0000, 1.0f, 1.0f,
		-15.0f, 15.0f, 0xFFFF0000, 0.0f, 0.0f);
	AEGfxTriAdd(
		15.0f, -15.0f, 0xFFFF0000, 1.0f, 1.0f,
		15.0f, 15.0f, 0xFFFF0000, 1.0f, 0.0f,
		-15.0f, 15.0f, 0xFFFF0000, 0.0f, 0.0f);
	pMeshApple = AEGfxMeshEnd();
=======
		-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	AEGfxTriAdd(
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
		0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	pMeshFruit = AEGfxMeshEnd();
>>>>>>> Stashed changes

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

		// Fruit Growth Logic
		if (fruits.size() < maxFruits)
		{
			fruitGrowthTimer += dt;
			if (fruitGrowthTimer >= FRUIT_GROWTH_TIME)
			{
				fruitGrowthTimer = 0.0f;

				// Find an empty spot
				// Simple logic: just pick the next available spot in the array
				// In a real game, you might want to randomize this or check for overlap
				if (fruits.size() < maxFruits)
				{
					Fruit newFruit;
					newFruit.type = rand() % 3; // Random fruit type: 0, 1, or 2
					newFruit.x = treePositions[fruits.size()].x;
					newFruit.y = treePositions[fruits.size()].y;
					newFruit.active = true;
					fruits.push_back(newFruit);
				}
			}
		}

		// Check for "Activity" (Selling Apple)
		// For now, SPACE sells a "virtual" apple from stock if energy permits
		// Later you might want to click on fruits to harvest them
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

<<<<<<< Updated upstream
		// --- Draw Apple (On Stall) ---
		AEMtx33Identity(&scale);
		AEMtx33Trans(&trans, 0.0f, 20.0f);
		AEMtx33Concat(&transform, &trans, &scale);
=======
		// --- Draw Fruits ---
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetTransparency(1.0f);

		for (const auto& fruit : fruits)
		{
			if (!fruit.active) continue;
>>>>>>> Stashed changes

			AEGfxTexture* pCurrentTex = 0;
			if (fruit.type == 0) pCurrentTex = pTexApple;
			else if (fruit.type == 1) pCurrentTex = pTexPear;
			else if (fruit.type == 2) pCurrentTex = pTexBanana;

			if (pCurrentTex)
			{
				AEGfxTextureSet(pCurrentTex, 0, 0);

				// Scale: 64x64 pixels for fruits
				AEMtx33Scale(&scale, 64.0f, 64.0f);
				AEMtx33Trans(&trans, fruit.x, fruit.y);
				AEMtx33Concat(&transform, &trans, &scale);

				AEGfxSetTransform(transform.m);
				AEGfxMeshDraw(pMeshFruit, AE_GFX_MDM_TRIANGLES);
			}
		}

		// --- Draw Resources (Top Left) ---
		// Screen coordinates: Center (0,0). Top Left approx (-400, 300).
		// We use AEGfxPrint for text.

		if (fontLoaded)
		{
			// 1. Gold Display
			sprintf_s(strBuffer, "Gold: %d", gold);
<<<<<<< Updated upstream
			// x: -0.95, y: 0.90, scale: 1.0, r: 1, g: 1, b: 0 (Yellow), a: 1
			AEGfxPrint(fontId, strBuffer, -0.95f, 0.90f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f);

			// 2. Energy Display
			sprintf_s(strBuffer, "Energy: %d/%d", energy, MAX_ENERGY);
			// x: -0.95, y: 0.80, scale: 1.0, r: 0, g: 1, b: 0 (Green), a: 1
			AEGfxPrint(fontId, strBuffer, -0.95f, 0.80f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
=======
			// Offset X slightly left to center text in box (-0.95 start)
			AEGfxPrint(fontId, strBuffer, -0.99f, 0.87f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f);

			// 2. Energy Display
			sprintf_s(strBuffer, "Energy: %d/%d", energy, MAX_ENERGY);
			AEGfxPrint(fontId, strBuffer, -0.99f, 0.74f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
>>>>>>> Stashed changes
		}

		// Informing the system about the loop's end
		AESysFrameEnd();
	}

	// ---------------------------------------------------------------------------
	// Cleanup
	// ---------------------------------------------------------------------------

	AEGfxMeshFree(pMeshStall);
<<<<<<< Updated upstream
	AEGfxMeshFree(pMeshApple);

	// FIX: Do NOT manually destroy the font or shutdown the font system here.
	// AESysExit() likely handles the entire graphics subsystem teardown.
	// Manually calling these might be causing a double-free crash.

	// AEGfxDestroyFont(fontId);  // <-- Commented out to prevent crash
	// AEGfxFontSystemEnd();      // <-- Commented out to prevent crash
=======
	AEGfxMeshFree(pMeshFruit);
	AEGfxMeshFree(pMeshUIBorder);

	if (pTexStall) AEGfxTextureUnload(pTexStall);
	if (pTexApple) AEGfxTextureUnload(pTexApple);
	if (pTexPear) AEGfxTextureUnload(pTexPear);
	if (pTexBanana) AEGfxTextureUnload(pTexBanana);
>>>>>>> Stashed changes

	// free the system
	AESysExit();
}
<<<<<<< Updated upstream



=======
>>>>>>> Stashed changes
