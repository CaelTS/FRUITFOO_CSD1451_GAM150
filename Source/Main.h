#pragma once
#include <vector>

// Save/Load functions
void SaveGame(int gold, int energy, int inventory[3]);
bool LoadGame(int& gold, int& energy, int inventory[3]);

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