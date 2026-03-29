#include "Crate.h"
#include "Profile.h"
#include <iostream>

// ---------------------------------------------------------------------------
// Constants (kept here to avoid redefinition with Profile.cpp's MAX_CRATES)
// ---------------------------------------------------------------------------
static constexpr int CRATE_COUNT = 3;   // must match MAX_CRATES in Profile.cpp

// Make max stock configurable at runtime instead of compile-time constexpr.
static int g_maxCrateStock = 9;  // default max fruit per crate

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------

struct CrateData
{
    bool isUnlocked = false;
    int  fruitCount = 0;       // current stock
};

static CrateData g_crates[CRATE_COUNT];

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void Crate_Load()
{
    // Nothing to load from disk here — Profile handles persistence.
    std::cout << "Crate_Load\n";
}

void Crate_Initialize()
{
    // Load state from the active profile.
    for (int i = 0; i < CRATE_COUNT; i++)
    {
        g_crates[i].isUnlocked = Profile_GetCrateUnlocked(i);
        g_crates[i].fruitCount = Profile_GetCrateFruitCount(i);
    }

    // Migration guard: if every crate comes back locked (old save with no
    // [crate] section), force crate 0 unlocked and persist it.
    bool anyUnlocked = false;
    for (int i = 0; i < CRATE_COUNT; i++)
        if (g_crates[i].isUnlocked) { anyUnlocked = true; break; }

    if (!anyUnlocked)
    {
        g_crates[0].isUnlocked = true;
        Profile_SetCrateUnlocked(0, true);
        std::cout << "Crate migration: crate 0 force-unlocked (no crate data in save)\n";
    }

    // Ensure existing stocks don't exceed current max (safe for profile upgrades)
    for (int i = 0; i < CRATE_COUNT; ++i)
    {
        if (g_crates[i].fruitCount > g_maxCrateStock)
        {
            g_crates[i].fruitCount = g_maxCrateStock;
            Profile_SetCrateFruitCount(i, g_crates[i].fruitCount);
        }
    }

    std::cout << "Crate_Initialize: loaded from profile\n";
    for (int i = 0; i < CRATE_COUNT; i++)
        std::cout << "  Crate[" << i << "]: unlocked=" << g_crates[i].isUnlocked
        << " stock=" << g_crates[i].fruitCount << "\n";
}

void Crate_Free()
{
    std::cout << "Crate_Free\n";
}

// ---------------------------------------------------------------------------
// Unlock state
// ---------------------------------------------------------------------------

bool Crate_IsUnlocked(int crateIndex)
{
    if (crateIndex < 0 || crateIndex >= CRATE_COUNT) return false;
    return g_crates[crateIndex].isUnlocked;
}

void Crate_SetUnlocked(int crateIndex, bool unlocked)
{
    if (crateIndex < 0 || crateIndex >= CRATE_COUNT) return;
    g_crates[crateIndex].isUnlocked = unlocked;
    Profile_SetCrateUnlocked(crateIndex, unlocked); // persists immediately
    std::cout << "Crate[" << crateIndex << "] unlocked=" << unlocked << "\n";
}

// ---------------------------------------------------------------------------
// Max stock accessor / mutator
// ---------------------------------------------------------------------------

int Crate_GetMaxStock()
{
    return g_maxCrateStock;
}

void Crate_SetMaxStock(int maxStock)
{
    if (maxStock < 0) return; // ignore invalid
    g_maxCrateStock = maxStock;

    // Clamp existing crate stock to new max and persist any changes.
    for (int i = 0; i < CRATE_COUNT; ++i)
    {
        if (g_crates[i].fruitCount > g_maxCrateStock)
        {
            g_crates[i].fruitCount = g_maxCrateStock;
            Profile_SetCrateFruitCount(i, g_crates[i].fruitCount);
        }
    }

    std::cout << "Crate max stock set to " << g_maxCrateStock << "\n";
}

// ---------------------------------------------------------------------------
// Stock queries & mutation
// ---------------------------------------------------------------------------

int Crate_GetFruitCount(int crateIndex)
{
    if (crateIndex < 0 || crateIndex >= CRATE_COUNT) return -1;
    if (!g_crates[crateIndex].isUnlocked)           return -1;
    return g_crates[crateIndex].fruitCount;
}

bool Crate_AddFruit(int crateIndex, int amount)
{
    if (crateIndex < 0 || crateIndex >= CRATE_COUNT) return false;
    if (!g_crates[crateIndex].isUnlocked)           return false;
    if (amount <= 0)                                return false;

    int before = g_crates[crateIndex].fruitCount;
    g_crates[crateIndex].fruitCount += amount;
    if (g_crates[crateIndex].fruitCount > g_maxCrateStock)
        g_crates[crateIndex].fruitCount = g_maxCrateStock;

    bool added = (g_crates[crateIndex].fruitCount > before);
    if (added)
    {
        Profile_SetCrateFruitCount(crateIndex, g_crates[crateIndex].fruitCount);
        std::cout << "Crate[" << crateIndex << "] +fruit -> stock="
            << g_crates[crateIndex].fruitCount << "\n";
    }
    return added;
}

bool Crate_AddFruitTyped(int fruitType, int amount)
{
    if (fruitType < 0 || fruitType >= CRATE_COUNT) return false;
    return Crate_AddFruit(fruitType, amount);
}

bool Crate_RemoveFruit(int crateIndex)
{
    return Crate_RemoveFruitAmount(crateIndex, 1);
}

bool Crate_RemoveFruitAmount(int crateIndex, int amount)
{
    if (crateIndex < 0 || crateIndex >= CRATE_COUNT) return false;
    if (!g_crates[crateIndex].isUnlocked)           return false;
    if (amount <= 0)                                return false;
    if (g_crates[crateIndex].fruitCount < amount)   return false;

    g_crates[crateIndex].fruitCount -= amount;
    Profile_SetCrateFruitCount(crateIndex, g_crates[crateIndex].fruitCount);

    std::cout << "Crate[" << crateIndex << "] -" << amount << " fruit -> stock="
        << g_crates[crateIndex].fruitCount << "\n";
    return true;
}

bool Crate_RemoveFruitTyped(int fruitType, int amount)
{
    if (fruitType < 0 || fruitType >= CRATE_COUNT) return false;
    return Crate_RemoveFruitAmount(fruitType, amount);
}
