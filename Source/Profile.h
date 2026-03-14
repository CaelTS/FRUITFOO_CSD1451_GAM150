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

bool Profile_WentBack();

// ---------------------------------------------------------------------------
// Economy <-> Profile
// ---------------------------------------------------------------------------
void Economy_SaveToProfile(int slot);
void Economy_LoadFromProfile(int slot);

// ---------------------------------------------------------------------------
// Profile slot management
// ---------------------------------------------------------------------------

// Creates a brand-new slot with all fields zeroed/defaulted and saves to disk.
void Profile_CreateSlot(int slot, const char* name);

// Sets the active slot, restores economy globals, and rewrites the save file.
// Use this when loading a profile from any screen (e.g. Continue button).
void Profile_SetActiveSlot(int slot);


// ---------------------------------------------------------------------------
// Inventory persistence
// ---------------------------------------------------------------------------

// Flush inventory counts into the slot and save.
void Profile_SaveInventory(int slot, int apples, int pears, int bananas,
    int seedApple, int seedPear, int seedBanana);

// Getters (read from active slot).
int Profile_GetApples();
int Profile_GetPears();
int Profile_GetBananas();
int Profile_GetSeed(int idx); // 0=apple, 1=pear, 2=banana
