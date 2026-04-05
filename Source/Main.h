#pragma once
#include <vector>
#include "SpawnFruits.h"

// MainScreen lifecycle functions
void MainScreen_Load();
void MainScreen_Initialize();
void MainScreen_Update();
void MainScreen_Render();
void MainScreen_Free();
void MainScreen_Unload();

void MainScreen_OnHelperCollect(int amount , FruitType fruit);
void Toast_Push(const char* msg, float r = 1.0f, float g = 0.95f, float b = 0.6f);

// Expose global scale factors (defined in Main.cpp)
extern float gScaleX;
extern float gScaleY;
