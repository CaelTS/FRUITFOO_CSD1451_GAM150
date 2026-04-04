#pragma once

// ---------------------------------------------------------------------------
// Crate system
//
// 3 crates total (index 0, 1, 2).
// Crate 0 is always unlocked by default.
// Crates 1 and 2 must be purchased/unlocked.
//
// Each crate holds a stock count of a single fruit type:
//   crate 0 = apples, crate 1 = pears, crate 2 = bananas
// ---------------------------------------------------------------------------

// ------------------------------------------------------------
// Lifecycle (call from MainScreen Load / Initialize / Free)
// ------------------------------------------------------------
void Crate_Load();
void Crate_Initialize();
void Crate_Draw();
void Crate_Free();

// ------------------------------------------------------------
// Unlock state
// Crate 0 is always unlocked. Crates 1 & 2 are saved/loaded.
// ------------------------------------------------------------
bool Crate_IsUnlocked(int crateIndex);
void Crate_SetUnlocked(int crateIndex, bool unlocked); // auto-saves to profile

// ------------------------------------------------------------
// Stock queries & mutation
// ------------------------------------------------------------

// Returns the current fruit count in the crate (-1 if invalid/locked).
int  Crate_GetFruitCount(int crateIndex);

// Returns the fruit type stored in the crate (e.g. -1 or enum; -1 if invalid/locked).
int Crate_GetFruitType(int crateIndex);

// Sets the fruit type stored in the crate (e.g. -1 or enum; -1 if invalid/locked).
int Crate_SetFruitType(int crateIndex, int fruitType);

// Adds 'amount' fruit to the crate (clamped to MAX_CRATE_STOCK).
// Returns true if any fruit was added.
bool Crate_AddFruit(int crateIndex, int amount);

// Removes one fruit from the crate after a sale.
// Returns true if removal succeeded (stock was > 0).
bool Crate_RemoveFruit(int crateIndex);

// Removes 'amount' fruit in one call (e.g. bulk sale).
// Returns true if the full amount was available and removed.
bool Crate_RemoveFruitAmount(int crateIndex, int amount);

//Add specific fruit type to its corresponding crate(0 = apple, 1 = pear, 2 = banana)
bool Crate_AddFruitTyped(int fruitType, int amount);

// Remove fruit from specific crate by fruit type (0=apple, 1=pear, 2=banana)
bool Crate_RemoveFruitTyped(int fruitType, int amount);

// Get / set the per-crate capacity (runtime configurable)
int  Crate_GetMaxStock();
void Crate_SetMaxStock(int maxStock);

// ------------------------------------------------------------
// Profile bridge  (called internally and by Profile.cpp)
// ------------------------------------------------------------
void Crate_SaveToProfile(int slot);
void Crate_LoadFromProfile(int slot);