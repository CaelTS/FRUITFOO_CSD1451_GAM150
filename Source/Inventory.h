#ifndef INVENTORY_H
#define INVENTORY_H
#include <AETypes.h>


void Inventory_RemoveFruit(u8 amount);  // Removes fruit from the inventory
void Inventory_RemoveSeed(u8 amount);
u8 Inventory_GetFruitStock();

// Inventory modification functions (with auto-save)
void Inventory_AddFruit(u8 amount, u8 fruitType);    // fruitType: 0=apple, 1=pear, 2=banana
void Inventory_AddSeed(u8 amount, u8 seedType);      // seedType: 0=apple, 1=pear, 2=banana
bool Inventory_RemoveSeed(u8 amount, u8 seedType);   // seedType: 0=apple, 1=pear, 2=banana
bool Inventory_RemoveFruitTyped(u8 amount, u8 fruitType);

// Inventory panel getters
int GetInventoryCount();
int GetInventoryLimit();

int GetFruitCount();
int GetSeedCount();

// Individual fruit/seed type getters
int GetAppleCount();
int GetPearCount();
int GetBananaCount();
int GetAppleSeedCount();
int GetPearSeedCount();
int GetBananaSeedCount();

// Inventory <-> Profile bridge (mirrors Economy pattern)
void Inventory_SaveToProfile(int slot);
void Inventory_LoadFromProfile(int slot);

// Global inventory variables
extern u8 total_seeds;
extern u8 total_fruits;

// Separate fruit types
extern u8 apples;
extern u8 pears;
extern u8 bananas;

// Separate seed types
extern u8 seed_apple;
extern u8 seed_pear;
extern u8 seed_banana;

#endif