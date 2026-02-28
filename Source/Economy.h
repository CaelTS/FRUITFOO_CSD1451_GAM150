#ifndef ECONOMY_H
#define ECONOMY_H

#include <stdio.h>
#include <utility> // for std::pair
#include <AETypes.h>

//Placeholder inventory function declarations
void Inventory_RemoveFruit(u8 amount);  // Removes fruit from the inventory

// Forward declarations of functions
void Economy_Init();
void Economy_Update(float dt);
void Economy_AddMoney(int amount);
bool Economy_SpendMoney(int amount);

// Getter functions (read-only)
int Economy_GetTotalMoney();
float Economy_GetMultiplier();

// Helper functions
f32 random_time(f32 min, f32 max);
u8 random_range(u8 min, u8 max);
std::pair<f32, f32> random_range_pair(f32 min1, f32 max1, f32 min2, f32 max2);

// Global variables (only if you need to expose them; otherwise, keep them static in the source file)
extern u64 total_money;
extern u64 max_money;
extern f32 money_multiplier;

#endif // ECONOMY_H