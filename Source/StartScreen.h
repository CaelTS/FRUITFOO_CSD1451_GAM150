#ifndef STARTSCREEN_H
#define STARTSCREEN_H

#include "AEEngine.h"

// Scale factors defined in Main.cpp
extern float gScaleX;
extern float gScaleY;

//extern variables
extern bool startScreenActive;

// ------------------------------------------------------------
// Public interface
// ------------------------------------------------------------

// Call this to check if the start screen is still active (for Main.cpp to know when to switch to main menu)
bool StartScreen_IsActive();

// Load textures and meshes (called once by GSM Load slot)
void StartScreen_Load();

// Reset animation state and button positions (called by GSM Initialize slot)
void StartScreen_Init();

// Update the start screen (hover, clicks, etc.)
void StartScreen_Update(float dt);

// Draw the start screen
void StartScreen_Draw();

void StartScreen_Unload();

#endif // STARTSCREEN_H