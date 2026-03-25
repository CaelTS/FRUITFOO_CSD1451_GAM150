#include "Inventory.h"
#include "Profile.h"
#include <AETypes.h>
#include <stdio.h>
#include <algorithm>

// Global inventory variables
u8 total_fruits = 10;
u8 total_seeds = 10;

// Separate fruit types (for future use)
u8 apples = 10;
u8 pears = 0;
u8 bananas = 0;

// Separate seed types
u8 seed_apple = 10;
u8 seed_pear = 0;
u8 seed_banana = 0;

// Runtime-configurable inventory capacity (default 10)
static int g_inventoryCapacity = 10;

//placeholder inventory stock function
u8 Inventory_GetFruitStock() {
	return total_fruits; //assume always have fruit for now
}
//placeholder function to remove fruit from inventory function
void Inventory_RemoveFruit(u8 amount) {
	// For now, remove from apples (can be enhanced to remove from mixed types)
	if (apples >= amount) {
		apples -= amount;
	}
	else {
		// If not enough apples, remove what we can from total_fruits
		if (total_fruits >= amount) {
			total_fruits -= amount;
		}
	}

	// Update totals
	total_fruits = apples + pears + bananas;

	printf("Removed %d fruits from inventory.\n", amount);
	printf("Fruits left in inventory: %d\n", total_fruits);

	// Save to active profile
	Inventory_SaveToProfile(Profile_GetActiveSlot());
}

bool Inventory_RemoveFruitTyped(u8 amount, u8 fruitType) {

	// Remove the specific fruit type
	switch (fruitType) {
	case 0: apples -= amount; break;
	case 1: pears -= amount; break;
	case 2: bananas -= amount; break;
	default: return false;
	}

	// Update totals
	total_fruits = apples + pears + bananas;

	printf("Removed %d fruits of type %d from inventory.\n", amount, fruitType);
	printf("Fruits in inventory: %d (A:%d P:%d B:%d)\n", total_fruits, apples, pears, bananas);

	// Save to active profile
	Inventory_SaveToProfile(Profile_GetActiveSlot());
	return true;
}


// inventory panel getters

int GetInventoryCount()
{
	return static_cast<int>(total_fruits);
}

int GetInventoryLimit()
{

	return 100; // or inventory_capacity if you add it

	return g_inventoryCapacity;
}

void SetInventoryLimit(int limit)
{
	if (limit < 0) return; // ignore invalid values
	g_inventoryCapacity = limit;

	// If current stock exceeds new capacity, trim least-priority fruits (apples -> pears -> bananas)
	int total = apples + pears + bananas;
	if (total > g_inventoryCapacity)
	{
		int toRemove = total - g_inventoryCapacity;

		int rem = std::min<int>(toRemove, static_cast<int>(apples));
		bananas -= rem;
		toRemove -= rem;

		rem = std::min<int>(toRemove, static_cast<int>(pears));
		pears -= rem;
		toRemove -= rem;

		rem = std::min<int>(toRemove, static_cast<int>(bananas));
		apples -= rem;
		toRemove -= rem;

		// Update totals and persist changes
		total_fruits = apples + pears + bananas;
		Inventory_SaveToProfile(Profile_GetActiveSlot());
	}

	printf("Inventory capacity set to %d\n", g_inventoryCapacity);

}


int GetFruitCount()
{
	return static_cast<int>(total_fruits);
}

int GetSeedCount()
{
	return static_cast<int>(total_seeds);
}

// Individual fruit type getters
int GetAppleCount()
{
	return static_cast<int>(apples);
}

int GetPearCount()
{
	return static_cast<int>(pears);
}

int GetBananaCount()
{
	return static_cast<int>(bananas);
}

// Individual seed type getters
int GetAppleSeedCount()
{
	return static_cast<int>(seed_apple);
}

int GetPearSeedCount()
{
	return static_cast<int>(seed_pear);
}

int GetBananaSeedCount()
{
	return static_cast<int>(seed_banana);
}

// ---------------------------------------------------------------------------
// Inventory modification functions (with auto-save)
// ---------------------------------------------------------------------------

void Inventory_AddFruit(u8 amount, u8 fruitType) {
	switch (fruitType) {
	case 0: // Apple
		apples += amount;
		break;
	case 1: // Pear
		pears += amount;
		break;
	case 2: // Banana
		bananas += amount;
		break;
	default:
		apples += amount; // default to apples
		break;
	}

	// Update totals
	total_fruits = apples + pears + bananas;

	// Clamp to capacity
	if (total_fruits > g_inventoryCapacity)
	{
		int overflow = total_fruits - g_inventoryCapacity;
		// remove overflow from the same fruitType we just added (best-effort)
		switch (fruitType) {
		case 2:
			bananas = (bananas >= overflow) ? bananas - overflow : 0;
			break;
		case 1:
			pears = (pears >= overflow) ? pears - overflow : 0;
			break;
		default:
			apples = (apples >= overflow) ? apples - overflow : 0;
			break;
		}
		// recalc totals
		total_fruits = apples + pears + bananas;
	}

	printf("Added %d fruits to inventory.\n", amount);
	printf("Fruits in inventory: %d (A:%d P:%d B:%d)\n", total_fruits, apples, pears, bananas);

	// Save to active profile
	Inventory_SaveToProfile(Profile_GetActiveSlot());
}

void Inventory_AddSeed(u8 amount, u8 seedType) {
	switch (seedType) {
	case 0: // Apple seed
		seed_apple += amount;
		break;
	case 1: // Pear seed
		seed_pear += amount;
		break;
	case 2: // Banana seed
		seed_banana += amount;
		break;
	default:
		seed_apple += amount; // default to apple seeds
		break;
	}

	// Update totals
	total_seeds = seed_apple + seed_pear + seed_banana;

	printf("Added %d seeds to inventory.\n", amount);
	printf("Seeds in inventory: %d (A:%d P:%d B:%d)\n", total_seeds, seed_apple, seed_pear, seed_banana);

	// Save to active profile
	Inventory_SaveToProfile(Profile_GetActiveSlot());
}

bool Inventory_RemoveSeed(u8 amount, u8 seedType) {
	// Check first
	bool hasEnough = false;
	switch (seedType) {
	case 0: hasEnough = (seed_apple >= amount); break;
	case 1: hasEnough = (seed_pear >= amount); break;
	case 2: hasEnough = (seed_banana >= amount); break;
	}

	if (!hasEnough) {
		printf("Not enough seeds of type %d to remove %d\n", seedType, amount);
		return false;
	}

	// Remove
	switch (seedType) {
	case 0: seed_apple -= amount; break;
	case 1: seed_pear -= amount; break;
	case 2: seed_banana -= amount; break;
	default: return false;
	}

	total_seeds = seed_apple + seed_pear + seed_banana;
	printf("Removed %d seeds from inventory.\n", amount);
	printf("Seeds in inventory: %d (A:%d P:%d B:%d)\n", total_seeds, seed_apple, seed_pear, seed_banana);
	Inventory_SaveToProfile(Profile_GetActiveSlot());
	return true;
}

// ---------------------------------------------------------------------------
// Inventory <-> Profile bridge (mirrors Economy pattern)
// ---------------------------------------------------------------------------

// Save current inventory state to profile slot
void Inventory_SaveToProfile(int slot) {
	// Update total counts
	total_fruits = apples + pears + bananas;
	total_seeds = seed_apple + seed_pear + seed_banana;

	// Save to profile (uses the existing Profile_SaveInventory function)
	Profile_SaveInventory(slot,
		static_cast<int>(apples),
		static_cast<int>(pears),
		static_cast<int>(bananas),
		static_cast<int>(seed_apple),
		static_cast<int>(seed_pear),
		static_cast<int>(seed_banana));
}

// Load inventory state from profile slot
void Inventory_LoadFromProfile(int slot) {

	(void)slot; // suppress C4100 — active slot used implicitly

	// Load from profile getters
	apples = static_cast<u8>(Profile_GetApples());
	pears = static_cast<u8>(Profile_GetPears());
	bananas = static_cast<u8>(Profile_GetBananas());

	seed_apple = static_cast<u8>(Profile_GetSeed(0));
	seed_pear = static_cast<u8>(Profile_GetSeed(1));
	seed_banana = static_cast<u8>(Profile_GetSeed(2));

	// Update total counts
	total_fruits = apples + pears + bananas;
	total_seeds = seed_apple + seed_pear + seed_banana;

	// Ensure loaded totals respect current capacity
	if (total_fruits > g_inventoryCapacity) {
		// clamp down as in SetInventoryLimit (remove from bananas->pears->apples)
		int toRemove = total_fruits - g_inventoryCapacity;
		int rem = std::min<int>(toRemove, static_cast<int>(bananas)); bananas -= rem; toRemove -= rem;
		rem = std::min<int>(toRemove, static_cast<int>(pears)); pears -= rem; toRemove -= rem;
		rem = std::min<int>(toRemove, static_cast<int>(apples)); apples -= rem; toRemove -= rem;
		total_fruits = apples + pears + bananas;
		Inventory_SaveToProfile(Profile_GetActiveSlot());
	}
}