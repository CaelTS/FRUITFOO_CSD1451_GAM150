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


// Call before entering GS_NEXT_SCREEN to set the mode:
//   selectMode = true: clicking an existing slot loads it
//   selectMode = false: clicking an existing slot renames it
void ProfileScreen_SetSelectMode(bool selectMode);

// Returns the slot index of the profile chosen via Continue (-1 if none yet)
int  Profile_GetActiveSlot();

// Returns the name of the active profile (empty string if none)
const char* Profile_GetName();

// Returns level/score of active profile
int Profile_Getcoins();