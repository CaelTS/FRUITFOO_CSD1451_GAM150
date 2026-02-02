#pragma once
#include <vector>
#include "UI.h"

// Save/Load functions
void SaveGame(int goldParam, int energyParam, int inventoryparam[3]);
bool LoadGame(int goldParam, int energyParam, int inventoryparam[3]);

// MainScreen lifecycle functions
void MainScreen_Update();
void MainScreen_Render();
void MainScreen_Load();
void MainScreen_Initialize();
void MainScreen_Free();
void MainScreen_Unload();

// Game state access functions
int GetGold();
int GetEnergy();
int GetSelectedFruit();
int GetInventory(int index);
float GetEnergyTimer();
float GetFruitGrowthTimer();


const std::vector<struct Fruit>& GetFruits();

struct FruitBasket
{
    int fruitType;        // 0 Apple, 1 Pear, 2 Banana
    float x, y;           // world position
    float width, height;  // hover size
};

// Global access (read-only)

const std::vector<FruitBasket>& GetFruitBaskets();