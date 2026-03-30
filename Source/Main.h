#pragma once
#include <vector>

// MainScreen lifecycle functions
void MainScreen_Load();
void MainScreen_Initialize();
void MainScreen_Update();
void MainScreen_Render();
void MainScreen_Free();
void MainScreen_Unload();

<<<<<<< HEAD
void MainScreen_OnHelperCollect(int amount);
=======
// BGM control — called by UIAudio_SetMusicEnabled to start/stop main screen BGM
void MainBGM_SetEnabled(bool enabled);
>>>>>>> parent of b2bb844 (hiu)
