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
// Play-time / session tracking
// ---------------------------------------------------------------------------

// Call once when the player enters the main game (starts accumulating time).
void Profile_StartSession();

// Call every frame with delta time while the game is running.
void Profile_UpdatePlayTime(float dt);

// Call when the player exits / the session ends. Flushes a final save.
void Profile_EndSession();

// Returns total hours played for the active slot (0 if none).
float Profile_GetPlayTime();

// Returns total session count for the active slot (0 if none).
int Profile_GetSessionCount();

// ---------------------------------------------------------------------------
// Economy <-> Profile
// ---------------------------------------------------------------------------
void Economy_SaveToProfile(int slot);
void Economy_LoadFromProfile(int slot);

// ---------------------------------------------------------------------------
// Inventory <-> Profile
// ---------------------------------------------------------------------------
void Inventory_SaveToProfile(int slot);
void Inventory_LoadFromProfile(int slot);

// ---------------------------------------------------------------------------
// Farm <-> Profile
// ---------------------------------------------------------------------------
void Farm_SaveToProfile(int slot);
void Farm_LoadFromProfile(int slot);

// Setters for farm plots (auto-save)
void Profile_SetPlotUnlocked(int plotIndex, bool unlocked);
void Profile_SetPlotData(int plotIndex, bool planted, bool ready, float timer, int seedType);

// Getters for farm plots (read from active slot)
bool Profile_GetPlotUnlocked(int plotIndex);
bool Profile_GetPlotPlanted(int plotIndex);
bool Profile_GetPlotReady(int plotIndex);
float Profile_GetPlotTimer(int plotIndex);
int Profile_GetPlotSeedType(int plotIndex);

// ---------------------------------------------------------------------------
// Crate <-> Profile
// ---------------------------------------------------------------------------
void Crate_SaveToProfile(int slot);
void Crate_LoadFromProfile(int slot);

// Setters (auto-save on each call)
void Profile_SetCrateUnlocked(int crateIndex, bool unlocked);
void Profile_SetCrateFruitCount(int crateIndex, int count);
void Profile_SetCrateFruitType(int crateIndex, int fruitType);

// Getters (read from active slot)
bool Profile_GetCrateUnlocked(int crateIndex);
int  Profile_GetCrateFruitCount(int crateIndex);
int  Profile_GetCrateFruitType(int crateIndex);

// ---------------------------------------------------------------------------
// Profile slot management
// ---------------------------------------------------------------------------

// Creates a brand-new slot with all fields zeroed/defaulted and saves to disk.
void Profile_CreateSlot(int slot, const char* name);

// Sets the active slot, restores economy globals, and writes ONE save.
// Use this when loading a profile from any screen (e.g. Continue button).
void Profile_SetActiveSlot(int slot);

// Reload profile data from disk without loading any UI textures.
// Use this in place of ProfileScreen_Load() when you only need fresh data
// (e.g. the Continue button in StartScreen).
void Profiles_Reload();

// Pending profile slot (set by StartScreen to defer activating a slot until the main screen initializes)
extern int g_pendingProfileSlot;

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