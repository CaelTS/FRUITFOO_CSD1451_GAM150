#pragma once
#include <vector>

// MainScreen lifecycle functions
void MainScreen_Load();
void MainScreen_Initialize();
void MainScreen_Update();
void MainScreen_Render();
void MainScreen_Free();
void MainScreen_Unload();

// Called by UI.cpp when the player toggles BGM in Settings
void MainBGM_SetEnabled(bool enabled);

// Called by HelperCreatures.cpp when the bunny collects a fruit
void MainScreen_OnHelperCollect(int amount);
