#pragma once
#ifndef SPLASH_H
#define SPLASH_H

// GSM lifecycle slots — wire these up in GameStateManager.cpp for GS_SPLASH
void Splash_Load();
void Splash_Initialize();
void Splash_Update();
void Splash_Draw();
void Splash_Free();
void Splash_Unload();

#endif // SPLASH_H