// ---------------------------------------------------------------------------
// includes

#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include <stdio.h>
#include <vector>
#include <fstream>  // For file I/O

// ---------------------------------------------------------------------------
// main

struct Fruit
{
	int type; // 0: Apple, 1: Pear, 2: Banana
	float x, y;
	bool active;
};

// Function to Save Game Data
void SaveGame(int gold, int energy, int inventory[3])
{
	std::ofstream outFile("savegame.txt");
	if (outFile.is_open())
	{
		outFile << gold << "\n";
		outFile << energy << "\n";
		outFile << inventory[0] << "\n";
		outFile << inventory[1] << "\n";
		outFile << inventory[2] << "\n";
		outFile.close();
		OutputDebugStringA("Game Saved Successfully.\n");
	}
	else
	{
		OutputDebugStringA("ERROR: Could not save game.\n");
	}
}

// Function to Load Game Data
bool LoadGame(int& gold, int& energy, int inventory[3])
{
	std::ifstream inFile("savegame.txt");
	if (inFile.is_open())
	{
		inFile >> gold;
		inFile >> energy;
		inFile >> inventory[0];
		inFile >> inventory[1];
		inFile >> inventory[2];
		inFile.close();
		OutputDebugStringA("Game Loaded Successfully.\n");
		return true;
	}
	return false;
}

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
	const int FRUIT_PRICE = 10;// Gives 10 gold per fruit

	// Inventory System
	int inventory[3] = { 0, 0, 0 }; // 0: Apple, 1: Pear, 2: Banana
	const int MAX_INVENTORY = 30;   // Capacity for each fruit
	int selectedFruit = 0;          // 0: Apple, 1: Pear, 2: Banana

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

	// --- LOAD GAME ---
	LoadGame(gold, energy, inventory);

	// Load the font. MAKE SURE "Assets/liberation-mono.ttf" IS IN YOUR EXE FOLDER!
	s8 fontId = AEGfxCreateFont("Assets/liberation-mono.ttf", 26); // Increased font size slightly

	// Error Checking for Font
	bool fontLoaded = (fontId >= 0);
	if (!fontLoaded)
	{
		OutputDebugStringA("ERROR: Failed to load 'Assets/liberation-mono.ttf'. Text will not be displayed.\n");
	}

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

	// 2. Create Fruit Mesh (Standard Unit Square)
	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	AEGfxTriAdd(
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
		0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
	pMeshFruit = AEGfxMeshEnd();

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

		// Fruit Growth Logic
		if (fruits.size() < maxFruits)
		{
			fruitGrowthTimer += dt;
			if (fruitGrowthTimer >= FRUIT_GROWTH_TIME)
			{
				fruitGrowthTimer = 0.0f;

				// Find an empty spot
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

		// Input Logic: Mouse Click to Pluck Fruits
		if (AEInputCheckTriggered(AEVK_LBUTTON))
		{
			s32 mouseX, mouseY;
			AEInputGetCursorPosition(&mouseX, &mouseY);

			// Convert screen coordinates to world coordinates
			// Screen: (0,0) top-left to (1600,900) bottom-right
			// World: (-800, 450) top-left to (800, -450) bottom-right
			float worldX = (float)mouseX - 800.0f;
			float worldY = 450.0f - (float)mouseY;

			// Check collision with fruits
			for (auto it = fruits.begin(); it != fruits.end(); )
			{
				if (it->active)
				{
					// Simple AABB collision (Fruit size is approx 64x64)
					float halfSize = 32.0f;
					if (worldX >= it->x - halfSize && worldX <= it->x + halfSize &&
						worldY >= it->y - halfSize && worldY <= it->y + halfSize)
					{
						// Check inventory capacity
						if (inventory[it->type] < MAX_INVENTORY)
						{
							// Add to inventory
							inventory[it->type]++;

							// Remove fruit from tree
							it = fruits.erase(it);
							continue; // Skip incrementing iterator since we erased
						}
					}
				}
				++it;
			}
		}

		// Input Logic: Select Fruit to Sell
		if (AEInputCheckTriggered(AEVK_1)) selectedFruit = 0; // Apple
		if (AEInputCheckTriggered(AEVK_2)) selectedFruit = 1; // Pear
		if (AEInputCheckTriggered(AEVK_3)) selectedFruit = 2; // Banana

		// Check for "Activity" (Selling Selected Fruit)
		if (AEInputCheckTriggered(AEVK_SPACE))
		{
			// Check if we have the selected fruit to sell
			if (inventory[selectedFruit] > 0)
			{
				if (energy >= ENERGY_COST)
				{
					energy -= ENERGY_COST;
					inventory[selectedFruit]--; // Remove 1 fruit
					gold += FRUIT_PRICE;
				}
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

		// --- Draw Fruits ---
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetTransparency(1.0f);

		for (const auto& fruit : fruits)
		{
			if (!fruit.active) continue;

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

		// 3. Inventory UI (Right Side)
		float invX = 650.0f; // Right side
		float invY = 400.0f; // Top aligned with Gold
		float invHeight = 160.0f; // Taller box for 3 items

		// Inventory Border (Blue)
		AEGfxSetColorToMultiply(0.0f, 0.5f, 1.0f, 1.0f); // Blue Tint
		AEMtx33Scale(&scale, uiWidth, invHeight);
		AEMtx33Trans(&trans, invX, invY - 50.0f); // Shift down slightly to center
		AEMtx33Concat(&transform, &trans, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(pMeshUIBorder, AE_GFX_MDM_TRIANGLES);

		// Inventory Background (Black)
		AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f); // Black Tint
		AEMtx33Scale(&scale, uiWidth - (borderSize * 2), invHeight - (borderSize * 2));
		AEMtx33Trans(&trans, invX, invY - 50.0f);
		AEMtx33Concat(&transform, &trans, &scale);
		AEGfxSetTransform(transform.m);
		AEGfxMeshDraw(pMeshUIBorder, AE_GFX_MDM_TRIANGLES);

		// Reset Color Multiplier for other things
		AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

		// --- Draw Text (Top Left) ---
		if (fontLoaded)
		{
			// 1. Gold Display
			sprintf_s(strBuffer, "Gold: %d", gold);
			// Offset X slightly left to center text in box (-0.95 start)
			AEGfxPrint(fontId, strBuffer, -0.99f, 0.87f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f);

			// 2. Energy Display
			sprintf_s(strBuffer, "Energy: %d/%d", energy, MAX_ENERGY);
			AEGfxPrint(fontId, strBuffer, -0.99f, 0.74f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

			// 3. Inventory Display (Right Side)
			float textX = 0.67f;
			float textY = 0.87f;
			float lineSpacing = 0.12f;

			// Helper to draw selection marker
			const char* marker0 = (selectedFruit == 0) ? "> " : "  ";
			const char* marker1 = (selectedFruit == 1) ? "> " : "  ";
			const char* marker2 = (selectedFruit == 2) ? "> " : "  ";

			sprintf_s(strBuffer, "%sApples: %d/%d", marker0, inventory[0], MAX_INVENTORY);
			// Highlight selected text with Yellow, others White
			if (selectedFruit == 0) AEGfxPrint(fontId, strBuffer, textX, textY, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
			else AEGfxPrint(fontId, strBuffer, textX, textY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

			sprintf_s(strBuffer, "%sPears:  %d/%d", marker1, inventory[1], MAX_INVENTORY);
			if (selectedFruit == 1) AEGfxPrint(fontId, strBuffer, textX, textY - lineSpacing, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
			else AEGfxPrint(fontId, strBuffer, textX, textY - lineSpacing, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

			sprintf_s(strBuffer, "%sBananas:%d/%d", marker2, inventory[2], MAX_INVENTORY);
			if (selectedFruit == 2) AEGfxPrint(fontId, strBuffer, textX, textY - (lineSpacing * 2), 1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
			else AEGfxPrint(fontId, strBuffer, textX, textY - (lineSpacing * 2), 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}

		// Informing the system about the loop's end
		AESysFrameEnd();
	}

	// ---------------------------------------------------------------------------
	// Cleanup
	// ---------------------------------------------------------------------------

	// --- SAVE GAME ---
	SaveGame(gold, energy, inventory);

	AEGfxMeshFree(pMeshStall);
	AEGfxMeshFree(pMeshFruit);
	AEGfxMeshFree(pMeshUIBorder);

	if (pTexStall) AEGfxTextureUnload(pTexStall);
	if (pTexApple) AEGfxTextureUnload(pTexApple);
	if (pTexPear) AEGfxTextureUnload(pTexPear);
	if (pTexBanana) AEGfxTextureUnload(pTexBanana);

	// free the system
	AESysExit();
}

