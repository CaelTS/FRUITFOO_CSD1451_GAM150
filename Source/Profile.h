#pragma once

// ProfileScreen lifecycle functions
void ProfileScreen_Load();
void ProfileScreen_Initialize();
void ProfileScreen_Update();
void ProfileScreen_Render();
void ProfileScreen_Free();
void ProfileScreen_Unload();

// Returns true when the name-entry popup is open
bool ProfileScreen_IsPopupActive();