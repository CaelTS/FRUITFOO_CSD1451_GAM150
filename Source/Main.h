#pragma once
#include <vector>

// MainScreen lifecycle functions
void MainScreen_Load();
void MainScreen_Initialize();
void MainScreen_Update();
void MainScreen_Render();
void MainScreen_Free();
void MainScreen_Unload();

// BGM control — called by UIAudio_SetMusicEnabled to start/stop main screen BGM
void MainBGM_SetEnabled(bool enabled);