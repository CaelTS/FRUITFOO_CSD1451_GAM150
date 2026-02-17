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
#include "UI.h"
#include "Rhythm.h"

// ---------------------------------------------------------------------------
// Game State Variables

struct Fruit
{
	int type; // 0: Apple, 1: Pear, 2: Banana
	float x, y;
	bool active;
};

// Game Variables
int gold = 0;
int energy = 50;
const int MAX_ENERGY = 50;
const int ENERGY_COST = 5;
const int FRUIT_PRICE = 10;


// Inventory System
int inventory[3] = { 0, 0, 0 }; // 0: Apple, 1: Pear, 2: Banana
const int MAX_INVENTORY = 30;
int selectedFruit = 0;

// Energy Regeneration
float energyTimer = 0.0f;
const float REGEN_TIME = 10.0f;

// Fruit Growth Logic
float fruitGrowthTimer = 0.0f;
const float FRUIT_GROWTH_TIME = 10.0f;
std::vector<Fruit> fruits;

// Pre-defined positions for fruits on trees
struct Point { float x, y; } treePositions[] = {
	{-600.0f, 200.0f}, {-500.0f, 300.0f}, {-700.0f, 100.0f}, // Left Tree
	{600.0f, 200.0f}, {500.0f, 300.0f}, {700.0f, 100.0f},    // Right Tree
	{-550.0f, 150.0f}, {650.0f, 250.0f}
};
int maxFruits = sizeof(treePositions) / sizeof(Point);

// Graphics Resources
s8 fontId = -1;
AEGfxTexture* pTexStall = NULL;
AEGfxTexture* pTexApple = NULL;
AEGfxTexture* pTexPear = NULL;
AEGfxTexture* pTexBanana = NULL;
AEGfxVertexList* pMeshStall = NULL;
AEGfxVertexList* pMeshFruit = NULL;
AEGfxVertexList* g_pMeshFullScreen = NULL;

// Random number generator
std::random_device rd;
std::mt19937 gen(rd());
// Fruit basket (hover)
std::vector<FruitBasket> gFruitBaskets;
const std::vector<FruitBasket>& GetFruitBaskets()
{
	return gFruitBaskets;
}
void SaveGame(int goldParam, int energyParam, int inventoryparam[3])
{
	std::ofstream outFile("savegame.txt");
	if (outFile.is_open())
	{
		outFile << goldParam << "\n";
		outFile << energyParam << "\n";
		outFile << inventoryparam[0] << "\n";
		outFile << inventoryparam[1] << "\n";
		outFile << inventoryparam[2] << "\n";
		outFile.close();
		OutputDebugStringA("Game Saved Successfully.\n");
	}
	else
	{
		OutputDebugStringA("ERROR: Could not save game.\n");
	}
}

bool LoadGame(int goldParam, int energyParam, int inventoryparam[3])
{
	std::ifstream inFile("savegame.txt");
	if (inFile.is_open())
	{
		inFile >> goldParam;
		inFile >> energyParam;
		inFile >> inventoryparam[0];
		inFile >> inventoryparam[1];
		inFile >> inventoryparam[2];
		inFile.close();
		OutputDebugStringA("Game Loaded Successfully.\n");
		return true;
	}
	return false;
}

int GetGold() { return gold; }
int GetEnergy() { return energy; }
int GetSelectedFruit() { return selectedFruit; }
int GetInventory(int index) { return inventory[index]; }
float GetEnergyTimer() { return energyTimer; }
float GetFruitGrowthTimer() { return fruitGrowthTimer; }
const std::vector<Fruit>& GetFruits() { return fruits; }

void MainScreen_Load()
{
	// Load Textures
	pTexStall = AEGfxTextureLoad("Assets/Stall_Empty_POT.png");
	pTexApple = AEGfxTextureLoad("Assets/Apple.png");
	pTexPear = AEGfxTextureLoad("Assets/Pear.png");
	pTexBanana = AEGfxTextureLoad("Assets/Banana.png");

	if (!pTexStall) OutputDebugStringA("ERROR: Failed to load 'Assets/Stall_Empty_POT.png'.\n");
	if (!pTexApple) OutputDebugStringA("ERROR: Failed to load 'Assets/Apple.png'.\n");
	if (!pTexPear) OutputDebugStringA("ERROR: Failed to load 'Assets/Pear.png'.\n");
	if (!pTexBanana) OutputDebugStringA("ERROR: Failed to load 'Assets/Banana.png'.\n");
}

void MainScreen_Initialize()
{
	// Reset game state
	gold = 0;
	energy = 50;
	energyTimer = 0.0f;
	fruitGrowthTimer = 0.0f;
	selectedFruit = 0;
	fruits.clear();
	inventory[0] = inventory[1] = inventory[2] = 0;

	//Economy Init
	Economy_Init();

	UI_Init();
	gFruitBaskets.clear();
	gFruitBaskets.push_back({ 0, -350.0f, -250.0f, 120.0f, 120.0f }); // Apple
	gFruitBaskets.push_back({ 1, -150.0f, -250.0f, 120.0f, 120.0f }); // Pear
	gFruitBaskets.push_back({ 2,   50.0f, -250.0f, 120.0f, 120.0f }); // Banana

	// Load saved game
	LoadGame(gold, energy, inventory);

	// Load font
	fontId = AEGfxCreateFont("Assets/liberation-mono.ttf", 26);
	if (fontId < 0)
		OutputDebugStringA("ERROR: Failed to load 'Assets/liberation-mono.ttf'.\n");

	// Create meshes
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

	// Economy Update
	Economy_Update(dt);
	UI_Input();

	// Energy Regeneration Logic
	if (energy < MAX_ENERGY)
	{
		energyTimer += dt;
		if (energyTimer >= REGEN_TIME)
		{
			energy++;
			energyTimer -= REGEN_TIME;
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
				Fruit newFruit{};
				newFruit.type = gen() % 3; // Random fruit type: 0, 1, or 2
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

	// Input Logic: Mouse Click to Pluck Fruits
	if (AEInputCheckTriggered(AEVK_LBUTTON))
	{
		s32 mouseX, mouseY;
		AEInputGetCursorPosition(&mouseX, &mouseY);

		// Convert screen coordinates to world coordinates
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

		if (UI_IsMenuOpen())
			return;
	}

	if (AEInputCheckTriggered(AEVK_F))
	{
		OutputDebugStringA("Switching to FARM state\n");
		next = GS_FARM_SCREEN;
	}


	if (AEInputCheckTriggered(AEVK_N)) {
		next = GS_NEXT_SCREEN;
	}

	// Switch to Rhythm game when pressing R
	if (AEInputCheckTriggered(AEVK_R))
	{
		next = GS_RHYTHM_SCREEN;
	}
}

void MainScreen_Render()
{
	// Clear screen
	AEGfxSetBackgroundColor(0.2f, 0.2f, 0.2f); // Dark Gray

	// Transformation Matrices
	AEMtx33 scale, trans, transform;
	char strBuffer[100];

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
	float uiX = -700.0f;
	float uiY_Gold = 400.0f;
	float uiY_Energy = 340.0f;
	float uiWidth = 250.0f;
	float uiHeight = 50.0f;
	float borderSize = 4.0f;

	// 1. Gold Border (Yellow)
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);

	AEMtx33Scale(&scale, uiWidth, uiHeight);
	AEMtx33Trans(&trans, uiX, uiY_Gold);
	AEMtx33Concat(&transform, &trans, &scale);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

	// 1b. Gold Background (Black)
	AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
	AEMtx33Scale(&scale, uiWidth - (borderSize * 2), uiHeight - (borderSize * 2));
	AEMtx33Trans(&trans, uiX, uiY_Gold);
	AEMtx33Concat(&transform, &trans, &scale);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

	// 2. Energy Border (Green)
	AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);

	AEMtx33Scale(&scale, uiWidth, uiHeight);
	AEMtx33Trans(&trans, uiX, uiY_Energy);
	AEMtx33Concat(&transform, &trans, &scale);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

	// 2b. Energy Background (Black)
	AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
	AEMtx33Scale(&scale, uiWidth - (borderSize * 2), uiHeight - (borderSize * 2));
	AEMtx33Trans(&trans, uiX, uiY_Energy);
	AEMtx33Concat(&transform, &trans, &scale);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

	// 3. Inventory UI (Right Side)
	float invX = 650.0f;
	float invY = 400.0f;
	float invHeight = 160.0f;

	// Inventory Border (Blue)
	AEGfxSetColorToMultiply(0.0f, 0.5f, 1.0f, 1.0f);
	AEMtx33Scale(&scale, uiWidth, invHeight);
	AEMtx33Trans(&trans, invX, invY - 50.0f);
	AEMtx33Concat(&transform, &trans, &scale);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

	// Inventory Background (Black)
	AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
	AEMtx33Scale(&scale, uiWidth - (borderSize * 2), invHeight - (borderSize * 2));
	AEMtx33Trans(&trans, invX, invY - 50.0f);
	AEMtx33Concat(&transform, &trans, &scale);
	AEGfxSetTransform(transform.m);
	AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

	// Reset Color Multiplier
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

	// --- Draw Text (Top Left) ---
	if (fontId >= 0)
	{
		// 1. Gold Display
		sprintf_s(strBuffer, "Gold: %d", gold);
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
		if (selectedFruit == 0) AEGfxPrint(fontId, strBuffer, textX, textY, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
		else AEGfxPrint(fontId, strBuffer, textX, textY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

		sprintf_s(strBuffer, "%sPears:  %d/%d", marker1, inventory[1], MAX_INVENTORY);
		if (selectedFruit == 1) AEGfxPrint(fontId, strBuffer, textX, textY - lineSpacing, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
		else AEGfxPrint(fontId, strBuffer, textX, textY - lineSpacing, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

		sprintf_s(strBuffer, "%sBananas:%d/%d", marker2, inventory[2], MAX_INVENTORY);
		if (selectedFruit == 2) AEGfxPrint(fontId, strBuffer, textX, textY - (lineSpacing * 2), 1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
		else AEGfxPrint(fontId, strBuffer, textX, textY - (lineSpacing * 2), 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}

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
	UI_DrawFruitBasketTooltips();
}

void MainScreen_Free()
{
	// Free meshes
	if (pMeshStall) AEGfxMeshFree(pMeshStall);
	if (pMeshFruit) AEGfxMeshFree(pMeshFruit);

	// Free textures
	if (pTexStall)  AEGfxTextureUnload(pTexStall);
	if (pTexApple)  AEGfxTextureUnload(pTexApple);
	if (pTexPear)   AEGfxTextureUnload(pTexPear);
	if (pTexBanana) AEGfxTextureUnload(pTexBanana);

	// Free font
	if (fontId >= 0)
	{
		AEGfxDestroyFont(fontId);
		fontId = -1;
	}

	// Clear STL containers
	fruits.clear();
	fruits.shrink_to_fit();

	// Reset pointers
	pMeshStall = pMeshFruit = nullptr;
	pTexStall = pTexApple = pTexPear = pTexBanana = nullptr;
}


void MainScreen_Unload()
{
	// Save game when leaving MainScreen
	SaveGame(gold, energy, inventory);
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

	// ---------------------------------------------------------------------------
	// Initialization
	// ---------------------------------------------------------------------------

	// Window Size: 1600x900
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL);

	// Changing the window title
	AESysSetWindowTitle("Fruit Stall Game");

	// reset the system modules
	AESysReset();

	// Initialize Font System
	AEGfxFontSystemStart();

	// create shared full-screen quad once
	AEGfxMeshStart();
	AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 1, 0.5f, -0.5f, 0xFFFFFFFF, 1, 1, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
	AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1, 1, 0.5f, 0.5f, 0xFFFFFFFF, 1, 0, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
	g_pMeshFullScreen = AEGfxMeshEnd();

	// Initialize Game State Manager
	GSM_Initialize(GS_MAIN_SCREEN);

	// Load and initialize first state
	//if (fpLoad) fpLoad();
	//if (fpInitialize) fpInitialize();

	// ---------------------------------------------------------------------------
	// Game Loop
	// ---------------------------------------------------------------------------
	while (gGameRunning)
	{
		// Informing the system about the loop's start
		AESysFrameStart();

		// Check for ESCAPE key to exit
		if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist()) {
			next = GS_EXIT;  // Set next state to exit
		}

		// RHYTHM GAME INPUT
		if (current == GS_RHYTHM_SCREEN)
		{
			if (AEInputCheckTriggered(AEVK_SPACE))
			{
				Rhythm_Hit();  // Hit the note
			}

			//// Check if song finished - ADD THIS HERE
			//if (Rhythm_IsSongFinished()) {
			//	// Option 1: Auto-return to main screen after delay
			//	next = GS_MAIN_SCREEN;

			//	// Option 2: Wait for player to press E
			//	// (don't auto-exit, let them see final score)
			//}

			// Exit rhythm game with E key
			if (AEInputCheckTriggered(AEVK_E))
			{
				next = GS_MAIN_SCREEN;
			}
		}


		// Check for state transition
		if (next != current && !TR_IsActive())
		{
			TR_Start(current, next);
		}

		if (TR_Update())
		{
			if (fpFree)   fpFree();
			if (current != GS_EXIT && fpUnload) fpUnload();

			previous = current;
			current = next;
			GSM_Update();

			if (current != GS_EXIT)
			{
				if (fpLoad)       fpLoad();
				if (fpInitialize) fpInitialize();
			}
		}

		// Update and draw current state
		if (current != GS_EXIT) {
			if (fpUpdate) fpUpdate();
			if (fpDraw) fpDraw();
		}
		else {
			gGameRunning = 0;  // Exit game loop
		}

		// Informing the system about the loop's end
		AESysFrameEnd();
	}

	// ---------------------------------------------------------------------------
	// Cleanup
	// ---------------------------------------------------------------------------

	// Free the system
	AESysExit();

	return 0;
}