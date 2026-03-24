#pragma once

// ---------------------------------------------------------------------------
// Tutorial system
//
// Lifecycle - call from StartScreen.cpp:
//   Tutorial_Load()    once alongside other StartScreen_Load texture loads
//   Tutorial_Unload()  once in StartScreen_Unload
//   Tutorial_Update()  each frame inside StartScreen_Update
//   Tutorial_Draw()    at the very end of StartScreen_Draw
// ---------------------------------------------------------------------------

void Tutorial_Load();
void Tutorial_Unload();
void Tutorial_Update(float slideOffset);
void Tutorial_Draw(float slideOffset, float fadeOut);
bool Tutorial_IsOpen();
