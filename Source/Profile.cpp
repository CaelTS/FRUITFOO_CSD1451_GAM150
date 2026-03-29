#include <crtdbg.h>
#include "AEEngine.h"
#include "Transition.h"
#include "GameStateManager.h"
#include "StartScreen.h"
#include "Economy.h"
#include "Inventory.h"
#include "Crate.h"

extern AEGfxVertexList* g_pMeshFullScreen;
extern s8 fontId;
extern int currentState;

// UI Textures
AEGfxTexture* pTexButtonLong = NULL;
AEGfxTexture* pTexButtonLongPressed = NULL;
AEGfxTexture* pTexButtonSquare = NULL;
AEGfxTexture* pTexInputRect = NULL;
AEGfxTexture* pTexCrossIcon = NULL;
AEGfxTexture* pTexPanel = NULL;
AEGfxTexture* pTexEditIcon = NULL;

// Profile data structure
constexpr auto MAX_PROFILES = 3;
constexpr auto PROFILE_NAME_MAX_LEN = 32;
constexpr auto MAX_FARM_PLOTS = 4;
constexpr auto MAX_CRATES = 3;

typedef struct {
    bool exists;
    char name[PROFILE_NAME_MAX_LEN];
    // Play stats
    float   play_time;      // total hours played
    int     session_count;  // number of sessions
    // Economy data
    unsigned long long total_money;
    unsigned long long max_money;
    float money_multiplier;
    // Inventory data
    int total_fruits;   // apples + pears + bananas (cached sum)
    int total_seeds;    // seed_apple + seed_pear + seed_banana (cached sum)
    int apples;
    int pears;
    int bananas;
    int seeds[3];   // seeds[0]=apple, seeds[1]=pear, seeds[2]=banana
    // Farm plot data (4 plots)
    bool plot_unlocked[MAX_FARM_PLOTS];
    bool plot_planted[MAX_FARM_PLOTS];
    bool plot_ready[MAX_FARM_PLOTS];
    float plot_timer[MAX_FARM_PLOTS];
    int plot_seed_type[MAX_FARM_PLOTS];
    // Crate data (3 crates: 0=apple, 1=pear, 2=banana)
    // Crate 0 always unlocked; crates 1 & 2 must be purchased.
    bool crate_unlocked[MAX_CRATES];
    int  crate_fruit_count[MAX_CRATES];
} Profile;

static Profile profiles[MAX_PROFILES] = {
    { false, "", 0.0f, 0, 0ULL, 255ULL, 1.0f, /*total_fruits*/0, /*total_seeds*/0, 0, 0, 0, {0,0,0},
      {true,false,false,false}, {false,false,false,false}, {false,false,false,false},
      {0.0f,0.0f,0.0f,0.0f}, {-1,-1,-1,-1},
      {true,false,false}, {0,0,0} },
    { false, "", 0.0f, 0, 0ULL, 255ULL, 1.0f, /*total_fruits*/0, /*total_seeds*/0, 0, 0, 0, {0,0,0},
      {true,false,false,false}, {false,false,false,false}, {false,false,false,false},
      {0.0f,0.0f,0.0f,0.0f}, {-1,-1,-1,-1},
      {true,false,false}, {0,0,0} },
    { false, "", 0.0f, 0, 0ULL, 255ULL, 1.0f, /*total_fruits*/0, /*total_seeds*/0, 0, 0, 0, {0,0,0},
      {true,false,false,false}, {false,false,false,false}, {false,false,false,false},
      {0.0f,0.0f,0.0f,0.0f}, {-1,-1,-1,-1},
      {true,false,false}, {0,0,0} }
};

// Popup state
static bool  popupActive = false;
static bool  popupEditMode = false;         // true = editing existing profile name
static int   popupSlotIndex = -1;           // which slot triggered the popup
static char  popupInputBuf[PROFILE_NAME_MAX_LEN] = "";
static int   popupInputLen = 0;

// Active profile slot (-1 = none selected)
static int   activeSlot = -1;
static bool  selectMode = false; // true = Continue path (click loads), false = manage path (click renames)

// Hover state (-1 = none)
static int   hoveredProfileSlot = -1;
static int   hoveredDeleteSlot = -1;
static int   hoveredEditSlot = -1;

static bool wentBack = false;
bool Profile_WentBack() { return wentBack; }

bool ProfileScreen_IsPopupActive() { return popupActive; }

void ProfileScreen_SetSelectMode(bool mode) { selectMode = mode; }

int Profile_GetActiveSlot() { return activeSlot; }

const char* Profile_GetName() {
    if (activeSlot >= 0 && activeSlot < MAX_PROFILES && profiles[activeSlot].exists)
        return profiles[activeSlot].name;
    return "";
}

// ---------------------------------------------------------------------------
// Persistence helpers
// ---------------------------------------------------------------------------
static const char* PROFILES_FILE = "profiles.txt";

// File format - one block per profile slot separated by blank lines:
//   [PROFILE_0]
//   EXISTS=1
//   NAME=dan
//   coins=1250

static void Profiles_Save() {
    FILE* f = nullptr;
    if (fopen_s(&f, PROFILES_FILE, "w") != 0 || !f) {
        OutputDebugStringA("ERROR: Could not open profiles.txt for writing.\n");
        return;
    }
    for (int i = 0; i < MAX_PROFILES; i++) {
        fprintf(f, "[PROFILE_%d]\n", i);
        fprintf(f, "EXISTS=%d\n", profiles[i].exists ? 1 : 0);
        fprintf(f, "NAME=%s\n", profiles[i].name);
        if (profiles[i].exists) {
            fprintf(f, "\n");
            fprintf(f, "[economy]\n");
            fprintf(f, "total_money=%llu\n", profiles[i].total_money);
            fprintf(f, "max_money=%llu\n", profiles[i].max_money);
            fprintf(f, "money_multiplier=%.3f\n", profiles[i].money_multiplier);
            fprintf(f, "\n");
            fprintf(f, "[inventory]\n");
            fprintf(f, "total_fruits=%d\n", profiles[i].apples + profiles[i].pears + profiles[i].bananas);
            fprintf(f, "total_seeds=%d\n", profiles[i].seeds[0] + profiles[i].seeds[1] + profiles[i].seeds[2]);
            fprintf(f, "apples=%d\n", profiles[i].apples);
            fprintf(f, "pears=%d\n", profiles[i].pears);
            fprintf(f, "bananas=%d\n", profiles[i].bananas);
            fprintf(f, "seeds=%d,%d,%d\n",
                profiles[i].seeds[0], profiles[i].seeds[1], profiles[i].seeds[2]);
            fprintf(f, "\n");
            fprintf(f, "[farm]\n");
            // Plot unlocked states (1=unlocked, 0=locked)
            fprintf(f, "plot_unlocked=%d,%d,%d,%d\n",
                profiles[i].plot_unlocked[0] ? 1 : 0,
                profiles[i].plot_unlocked[1] ? 1 : 0,
                profiles[i].plot_unlocked[2] ? 1 : 0,
                profiles[i].plot_unlocked[3] ? 1 : 0);
            // Plot planted states
            fprintf(f, "plot_planted=%d,%d,%d,%d\n",
                profiles[i].plot_planted[0] ? 1 : 0,
                profiles[i].plot_planted[1] ? 1 : 0,
                profiles[i].plot_planted[2] ? 1 : 0,
                profiles[i].plot_planted[3] ? 1 : 0);
            // Plot ready states
            fprintf(f, "plot_ready=%d,%d,%d,%d\n",
                profiles[i].plot_ready[0] ? 1 : 0,
                profiles[i].plot_ready[1] ? 1 : 0,
                profiles[i].plot_ready[2] ? 1 : 0,
                profiles[i].plot_ready[3] ? 1 : 0);
            // Plot timers (growth progress)
            fprintf(f, "plot_timer=%.3f,%.3f,%.3f,%.3f\n",
                profiles[i].plot_timer[0],
                profiles[i].plot_timer[1],
                profiles[i].plot_timer[2],
                profiles[i].plot_timer[3]);
            // Plot seed types (-1=empty)
            fprintf(f, "plot_seed_type=%d,%d,%d,%d\n",
                profiles[i].plot_seed_type[0],
                profiles[i].plot_seed_type[1],
                profiles[i].plot_seed_type[2],
                profiles[i].plot_seed_type[3]);
            fprintf(f, "\n");
            fprintf(f, "[crate]\n");
            // Crate unlock states (crate 0 always unlocked; 1 & 2 purchasable)
            fprintf(f, "crate_unlocked=%d,%d,%d\n",
                profiles[i].crate_unlocked[0] ? 1 : 0,
                profiles[i].crate_unlocked[1] ? 1 : 0,
                profiles[i].crate_unlocked[2] ? 1 : 0);
            // Fruit stock per crate
            fprintf(f, "crate_fruit_count=%d,%d,%d\n",
                profiles[i].crate_fruit_count[0],
                profiles[i].crate_fruit_count[1],
                profiles[i].crate_fruit_count[2]);
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

static void Profiles_Load() {
    FILE* f = nullptr;
    if (fopen_s(&f, PROFILES_FILE, "r") != 0 || !f)
        return; // No save file yet - keep the defaults

    int  slotIndex = -1;
    bool inEconomy = false;
    bool inInventory = false;
    bool inFarm = false;
    bool inCrate = false;
    char line[256] = "";

    while (fgets(line, sizeof(line), f)) {
        int len = (int)strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
        if (len > 0 && line[len - 1] == '\r') line[--len] = '\0';
        if (len == 0) continue;

        if (line[0] == '[') {
            if (strcmp(line, "[economy]") == 0) {
                inEconomy = true; inInventory = false; inFarm = false;
            }
            else if (strcmp(line, "[inventory]") == 0) {
                inInventory = true; inEconomy = false; inFarm = false;
            }
            else if (strcmp(line, "[farm]") == 0) {
                inFarm = true; inEconomy = false; inInventory = false; inCrate = false;
            }
            else if (strcmp(line, "[crate]") == 0) {
                inCrate = true; inFarm = false; inEconomy = false; inInventory = false;
            }
            else {
                inEconomy = false; inInventory = false; inFarm = false; inCrate = false;
                sscanf_s(line, "[PROFILE_%d]", &slotIndex);
            }
            continue;
        }

        if (slotIndex < 0 || slotIndex >= MAX_PROFILES) continue;

        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        const char* key = line;
        const char* value = eq + 1;

        if (inEconomy) {
            if (strcmp(key, "total_money") == 0)
                profiles[slotIndex].total_money = (unsigned long long)_strtoui64(value, nullptr, 10);
            else if (strcmp(key, "max_money") == 0)
                profiles[slotIndex].max_money = (unsigned long long)_strtoui64(value, nullptr, 10);
            else if (strcmp(key, "money_multiplier") == 0)
                profiles[slotIndex].money_multiplier = (float)atof(value);
        }
        else if (inInventory) {
            if (strcmp(key, "total_fruits") == 0) profiles[slotIndex].total_fruits = atoi(value);
            else if (strcmp(key, "total_seeds") == 0) profiles[slotIndex].total_seeds = atoi(value);
            else if (strcmp(key, "apples") == 0) profiles[slotIndex].apples = atoi(value);
            else if (strcmp(key, "pears") == 0) profiles[slotIndex].pears = atoi(value);
            else if (strcmp(key, "bananas") == 0) profiles[slotIndex].bananas = atoi(value);
            else if (strcmp(key, "seeds") == 0)
                sscanf_s(value, "%d,%d,%d",
                    &profiles[slotIndex].seeds[0],
                    &profiles[slotIndex].seeds[1],
                    &profiles[slotIndex].seeds[2]);
        }
        else if (inFarm) {
            if (strcmp(key, "plot_unlocked") == 0) {
                int u0, u1, u2, u3;
                sscanf_s(value, "%d,%d,%d,%d", &u0, &u1, &u2, &u3);
                profiles[slotIndex].plot_unlocked[0] = (u0 != 0);
                profiles[slotIndex].plot_unlocked[1] = (u1 != 0);
                profiles[slotIndex].plot_unlocked[2] = (u2 != 0);
                profiles[slotIndex].plot_unlocked[3] = (u3 != 0);
            }
            else if (strcmp(key, "plot_planted") == 0) {
                int p0, p1, p2, p3;
                sscanf_s(value, "%d,%d,%d,%d", &p0, &p1, &p2, &p3);
                profiles[slotIndex].plot_planted[0] = (p0 != 0);
                profiles[slotIndex].plot_planted[1] = (p1 != 0);
                profiles[slotIndex].plot_planted[2] = (p2 != 0);
                profiles[slotIndex].plot_planted[3] = (p3 != 0);
            }
            else if (strcmp(key, "plot_ready") == 0) {
                int r0, r1, r2, r3;
                sscanf_s(value, "%d,%d,%d,%d", &r0, &r1, &r2, &r3);
                profiles[slotIndex].plot_ready[0] = (r0 != 0);
                profiles[slotIndex].plot_ready[1] = (r1 != 0);
                profiles[slotIndex].plot_ready[2] = (r2 != 0);
                profiles[slotIndex].plot_ready[3] = (r3 != 0);
            }
            else if (strcmp(key, "plot_timer") == 0) {
                sscanf_s(value, "%f,%f,%f,%f",
                    &profiles[slotIndex].plot_timer[0],
                    &profiles[slotIndex].plot_timer[1],
                    &profiles[slotIndex].plot_timer[2],
                    &profiles[slotIndex].plot_timer[3]);
            }
            else if (strcmp(key, "plot_seed_type") == 0) {
                sscanf_s(value, "%d,%d,%d,%d",
                    &profiles[slotIndex].plot_seed_type[0],
                    &profiles[slotIndex].plot_seed_type[1],
                    &profiles[slotIndex].plot_seed_type[2],
                    &profiles[slotIndex].plot_seed_type[3]);
            }
        }
        else if (inCrate) {
            int v0, v1, v2;
            if (strcmp(key, "crate_unlocked") == 0) {
                if (sscanf_s(value, "%d,%d,%d", &v0, &v1, &v2) == 3) {
                    profiles[slotIndex].crate_unlocked[0] = (v0 != 0);
                    profiles[slotIndex].crate_unlocked[1] = (v1 != 0);
                    profiles[slotIndex].crate_unlocked[2] = (v2 != 0);
                }
            }
            else if (strcmp(key, "crate_fruit_count") == 0) {
                if (sscanf_s(value, "%d,%d,%d", &v0, &v1, &v2) == 3) {
                    profiles[slotIndex].crate_fruit_count[0] = v0;
                    profiles[slotIndex].crate_fruit_count[1] = v1;
                    profiles[slotIndex].crate_fruit_count[2] = v2;
                }
            }
        }
        else {
            if (strcmp(key, "EXISTS") == 0)
                profiles[slotIndex].exists = (atoi(value) != 0);
            else if (strcmp(key, "NAME") == 0)
                strncpy_s(profiles[slotIndex].name, PROFILE_NAME_MAX_LEN, value, _TRUNCATE);
            else if (strcmp(key, "play_time") == 0)
                sscanf_s(value, "%f", &profiles[slotIndex].play_time);
            else if (strcmp(key, "session_count") == 0)
                profiles[slotIndex].session_count = atoi(value);
            // Backward compatibility: old 'coins' field → total_money
            else if (strcmp(key, "coins") == 0)
                profiles[slotIndex].total_money = (unsigned long long)_strtoui64(value, nullptr, 10);
        }
    }
    fclose(f);

    // Migration: Initialize inventory for existing profiles that have all-zero inventory
    for (int i = 0; i < MAX_PROFILES; i++) {
        if (profiles[i].exists) {
            // Check if this profile has no inventory (all zeros) - indicates old profile
            bool hasNoInventory = (profiles[i].apples == 0 &&
                profiles[i].pears == 0 &&
                profiles[i].bananas == 0 &&
                profiles[i].seeds[0] == 0 &&
                profiles[i].seeds[1] == 0 &&
                profiles[i].seeds[2] == 0);

            if (hasNoInventory) {
                // Set default starting inventory for migrated profiles
                profiles[i].apples = 10;
                profiles[i].seeds[0] = 10;  // apple seeds
                // pears, bananas, and other seeds remain 0
            }
        }
    }

    Profiles_Save(); // re-save to upgrade old files missing new sections
}

// Public data-only reload — call this instead of ProfileScreen_Load() when
// you only need fresh profile data and NOT the ProfileScreen UI textures.
void Profiles_Reload() {
    Profiles_Load();
}

// ---------------------------------------------------------------------------
// Play-time / session tracking
// ---------------------------------------------------------------------------

static float s_playTimeAccum = 0.0f;    // Seconds since last auto-save
static float s_sessionStartTime = 0.0f; // Track when current session started
static bool  s_sessionActive = false;     // Whether a session is currently active

void Profile_StartSession() {
    int slot = activeSlot;
    if (slot < 0 || slot >= MAX_PROFILES || !profiles[slot].exists) return;

    if (!s_sessionActive) {
        // Increment session count once per session
        profiles[slot].session_count++;
        s_sessionActive = true;
        s_sessionStartTime = 0.0f; // Will accumulate via UpdatePlayTime
        s_playTimeAccum = 0.0f;

        // Save immediately to persist the new session count
        Profiles_Save();
    }
}

void Profile_UpdatePlayTime(float dt) {
    int slot = activeSlot;
    if (slot < 0 || slot >= MAX_PROFILES || !profiles[slot].exists) return;
    if (!s_sessionActive) return; // Don't track if session hasn't started

    // Accumulate play time (convert seconds to hours)
    profiles[slot].play_time += dt / 3600.0f;
    s_playTimeAccum += dt;

    // Auto-save every 60 seconds so play time survives a crash
    if (s_playTimeAccum >= 60.0f) {
        Profiles_Save();
        s_playTimeAccum = 0.0f;
    }
}

void Profile_EndSession() {
    int slot = activeSlot;
    if (slot < 0 || slot >= MAX_PROFILES || !profiles[slot].exists) return;

    if (s_sessionActive) {
        // Final save to ensure all data is persisted
        Profiles_Save();
        s_sessionActive = false;
        s_playTimeAccum = 0.0f;
    }
}

float Profile_GetPlayTime() {
    return (activeSlot >= 0) ? profiles[activeSlot].play_time : 0.0f;
}

int Profile_GetSessionCount() {
    return (activeSlot >= 0) ? profiles[activeSlot].session_count : 0;
}

// ---------------------------------------------------------------------------
// Economy <-> Profile bridge
// ---------------------------------------------------------------------------


// Call this before Profiles_Save() to flush live economy state into the slot.
void Economy_SaveToProfile(int slot) {
    if (slot < 0 || slot >= MAX_PROFILES) return;
    if (!profiles[slot].exists) return;
    profiles[slot].total_money = (unsigned long long)Economy_GetTotalMoney();
    profiles[slot].max_money = (unsigned long long)Economy_GetMaxMoney();
    profiles[slot].money_multiplier = Economy_GetMultiplier();
    Profiles_Save();
}

// Write inventory counts into the active profile and save.
void Profile_SaveInventory(int slot, int p_apples, int p_pears, int p_bananas,
    int seedApple, int seedPear, int seedBanana) {
    if (slot < 0 || slot >= MAX_PROFILES || !profiles[slot].exists) return;
    profiles[slot].apples = p_apples;
    profiles[slot].pears = p_pears;
    profiles[slot].bananas = p_bananas;
    profiles[slot].seeds[0] = seedApple;
    profiles[slot].seeds[1] = seedPear;
    profiles[slot].seeds[2] = seedBanana;
    // Keep cached totals in sync
    profiles[slot].total_fruits = p_apples + p_pears + p_bananas;
    profiles[slot].total_seeds = seedApple + seedPear + seedBanana;
    Profiles_Save();
}

// Getters for inventory  (read from active slot).
int   Profile_GetApples() { return (activeSlot >= 0) ? profiles[activeSlot].apples : 0; }
int   Profile_GetPears() { return (activeSlot >= 0) ? profiles[activeSlot].pears : 0; }
int   Profile_GetBananas() { return (activeSlot >= 0) ? profiles[activeSlot].bananas : 0; }
int   Profile_GetSeed(int idx) { return (activeSlot >= 0 && idx >= 0 && idx < 3) ? profiles[activeSlot].seeds[idx] : 0; }

// ---------------------------------------------------------------------------
// Farm <-> Profile bridge (similar to Inventory)
// ---------------------------------------------------------------------------

// Save farm plot data to profile slot
void Farm_SaveToProfile(int slot) {
    if (slot < 0 || slot >= MAX_PROFILES || !profiles[slot].exists) return;
    // Farm plot data is already in profiles[slot], just save to disk
    Profiles_Save();
}

// Load farm plot data from profile slot
void Farm_LoadFromProfile(int slot) {
    if (slot < 0 || slot >= MAX_PROFILES || !profiles[slot].exists) return;
    // Farm plot data is already in profiles[slot] after Profiles_Load()
    // No need to copy anywhere, Farm.cpp will read it via getters
}

// Setters for farm plots (called by Farm.cpp)
void Profile_SetPlotUnlocked(int plotIndex, bool unlocked) {
    if (activeSlot < 0 || activeSlot >= MAX_PROFILES) return;
    if (plotIndex < 0 || plotIndex >= MAX_FARM_PLOTS) return;
    profiles[activeSlot].plot_unlocked[plotIndex] = unlocked;
    Profiles_Save();
}

void Profile_SetPlotData(int plotIndex, bool planted, bool ready, float timer, int seedType) {
    if (activeSlot < 0 || activeSlot >= MAX_PROFILES) return;
    if (plotIndex < 0 || plotIndex >= MAX_FARM_PLOTS) return;
    profiles[activeSlot].plot_planted[plotIndex] = planted;
    profiles[activeSlot].plot_ready[plotIndex] = ready;
    profiles[activeSlot].plot_timer[plotIndex] = timer;
    profiles[activeSlot].plot_seed_type[plotIndex] = seedType;
    Profiles_Save();
}

// Getters for farm plots (read from active slot)
bool Profile_GetPlotUnlocked(int plotIndex) {
    if (activeSlot < 0 || plotIndex < 0 || plotIndex >= MAX_FARM_PLOTS) return false;
    return profiles[activeSlot].plot_unlocked[plotIndex];
}

bool Profile_GetPlotPlanted(int plotIndex) {
    if (activeSlot < 0 || plotIndex < 0 || plotIndex >= MAX_FARM_PLOTS) return false;
    return profiles[activeSlot].plot_planted[plotIndex];
}

bool Profile_GetPlotReady(int plotIndex) {
    if (activeSlot < 0 || plotIndex < 0 || plotIndex >= MAX_FARM_PLOTS) return false;
    return profiles[activeSlot].plot_ready[plotIndex];
}

float Profile_GetPlotTimer(int plotIndex) {
    if (activeSlot < 0 || plotIndex < 0 || plotIndex >= MAX_FARM_PLOTS) return 0.0f;
    return profiles[activeSlot].plot_timer[plotIndex];
}

int Profile_GetPlotSeedType(int plotIndex) {
    if (activeSlot < 0 || plotIndex < 0 || plotIndex >= MAX_FARM_PLOTS) return -1;
    return profiles[activeSlot].plot_seed_type[plotIndex];
}

// ---------------------------------------------------------------------------
// Crate <-> Profile bridge
// ---------------------------------------------------------------------------

void Crate_SaveToProfile(int slot) {
    if (slot < 0 || slot >= MAX_PROFILES || !profiles[slot].exists) return;
    Profiles_Save();
}

void Crate_LoadFromProfile(int slot) {
    if (slot < 0 || slot >= MAX_PROFILES || !profiles[slot].exists) return;
    // Data is already in profiles[slot] after Profiles_Load().
    // Crate.cpp reads it via the getters below.
}

// Setters (called by Crate.cpp — each call triggers a save)
void Profile_SetCrateUnlocked(int crateIndex, bool unlocked) {
    if (activeSlot < 0 || activeSlot >= MAX_PROFILES) return;
    if (crateIndex < 0 || crateIndex >= MAX_CRATES) return;
    profiles[activeSlot].crate_unlocked[crateIndex] = unlocked;
    Profiles_Save();
}

void Profile_SetCrateFruitCount(int crateIndex, int count) {
    if (activeSlot < 0 || activeSlot >= MAX_PROFILES) return;
    if (crateIndex < 0 || crateIndex >= MAX_CRATES) return;
    if (count < 0) count = 0;
    profiles[activeSlot].crate_fruit_count[crateIndex] = count;
    Profiles_Save();
}

// Getters (read from active slot)
bool Profile_GetCrateUnlocked(int crateIndex) {
    if (activeSlot < 0 || crateIndex < 0 || crateIndex >= MAX_CRATES) return false;
    return profiles[activeSlot].crate_unlocked[crateIndex];
}

int Profile_GetCrateFruitCount(int crateIndex) {
    if (activeSlot < 0 || crateIndex < 0 || crateIndex >= MAX_CRATES) return 0;
    return profiles[activeSlot].crate_fruit_count[crateIndex];
}

// Call this after a profile is loaded to restore economy state from the slot.
void Economy_LoadFromProfile(int slot) {
    if (slot < 0 || slot >= MAX_PROFILES || !profiles[slot].exists) return;
    total_money = profiles[slot].total_money;
    max_money = profiles[slot].max_money;
    money_multiplier = profiles[slot].money_multiplier;
}

// Creates a new profile slot with economy defaults and saves the full file.
// Call this from any screen that creates a new profile (e.g. StartScreen).
void Profile_CreateSlot(int slot, const char* name) {
    if (slot < 0 || slot >= MAX_PROFILES) return;
    profiles[slot].exists = true;
    profiles[slot].play_time = 0.0f;
    profiles[slot].session_count = 0;
    profiles[slot].total_money = 0ULL;
    profiles[slot].max_money = 255ULL;
    profiles[slot].money_multiplier = 1.0f;
    // Default starting inventory: 10 apples and 10 apple seeds
    profiles[slot].apples = 10;
    profiles[slot].pears = 0;
    profiles[slot].bananas = 0;
    profiles[slot].seeds[0] = 10;  // apple seeds
    profiles[slot].seeds[1] = 0;   // pear seeds
    profiles[slot].seeds[2] = 0;   // banana seeds
    profiles[slot].total_fruits = 10; // apples only at start
    profiles[slot].total_seeds = 10; // apple seeds only at start
    // Default farm state: plot 0 unlocked, all empty
    profiles[slot].plot_unlocked[0] = true;
    profiles[slot].plot_unlocked[1] = false;
    profiles[slot].plot_unlocked[2] = false;
    profiles[slot].plot_unlocked[3] = false;
    for (int i = 0; i < MAX_FARM_PLOTS; i++) {
        profiles[slot].plot_planted[i] = false;
        profiles[slot].plot_ready[i] = false;
        profiles[slot].plot_timer[i] = 0.0f;
        profiles[slot].plot_seed_type[i] = -1;
    }
    // Default crate state: crate 0 unlocked (apple crate), 1 & 2 locked
    profiles[slot].crate_unlocked[0] = true;
    profiles[slot].crate_unlocked[1] = false;
    profiles[slot].crate_unlocked[2] = false;
    for (int i = 0; i < MAX_CRATES; i++)
        profiles[slot].crate_fruit_count[i] = 0;
    strncpy_s(profiles[slot].name, PROFILE_NAME_MAX_LEN, name, _TRUNCATE);
    Profiles_Save();
}

// Sets the active slot, restores economy globals, and rewrites the save file.
// Call this from any screen that loads a profile (e.g. Continue button).
void Profile_SetActiveSlot(int slot) {
    if (slot < 0 || slot >= MAX_PROFILES || !profiles[slot].exists) return;
    activeSlot = slot;

    // Load all subsystems from the in-memory profile data (no disk I/O)
    Economy_LoadFromProfile(slot);
    Inventory_LoadFromProfile(slot);
    Farm_LoadFromProfile(slot);
    Crate_LoadFromProfile(slot);

    // Flush live state back into the struct, then save ONCE
    profiles[slot].total_money = (unsigned long long)Economy_GetTotalMoney();
    profiles[slot].max_money = (unsigned long long)Economy_GetMaxMoney();
    profiles[slot].money_multiplier = Economy_GetMultiplier();

    Profiles_Save(); // single write
}

static const float SCREEN_WIDTH = 1600.0f;
static const float SCREEN_HEIGHT = 900.0f;
static const float BUTTON_WIDTH_PX = 400.0f;
static const float BUTTON_HEIGHT_PX = 80.0f;
static const float DELETE_BUTTON_SIZE_PX = 40.0f;
static const float EDIT_BUTTON_SIZE_PX = 40.0f;
static const float PROFILE_SPACING_PX = 120.0f;
static const float START_Y_PX = 150.0f;

// ProfileScreen-specific resources
static AEGfxVertexList* pMeshButtonLong = NULL;
static AEGfxVertexList* pMeshButtonSquare = NULL;
static AEGfxVertexList* pMeshNextObject = NULL;

// Helper function to convert pixels to NDC
static float PixelsToNDC_X(float pixels) {
    return pixels / (SCREEN_WIDTH * 0.5f);
}

static float PixelsToNDC_Y(float pixels) {
    return pixels / (SCREEN_HEIGHT * 0.5f);
}

// Helper function to draw textured quad with color tint
static void DrawTexturedQuad(AEGfxTexture* texture, AEGfxVertexList* mesh,
    float x, float y, float width, float height,
    float r, float g, float b, float a) {
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(r, g, b, a);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxTextureSet(texture, 0, 0);

    AEMtx33 trans, scale, transform;
    // Convert NDC sizes back to pixel-space for the transform matrix
    AEMtx33Scale(&scale, width * (SCREEN_WIDTH * 0.5f), height * (SCREEN_HEIGHT * 0.5f));
    AEMtx33Trans(&trans, x * (SCREEN_WIDTH * 0.5f), y * (SCREEN_HEIGHT * 0.5f));
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);

    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

// Helper function to draw colored quad
static void DrawColoredQuad(float x, float y, float width, float height,
    float r, float g, float b, float a) {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(r, g, b, a);

    AEMtx33 trans, scale, transform;
    // Convert NDC sizes back to pixel-space for the transform matrix
    AEMtx33Scale(&scale, width * (SCREEN_WIDTH * 0.5f), height * (SCREEN_HEIGHT * 0.5f));
    AEMtx33Trans(&trans, x * (SCREEN_WIDTH * 0.5f), y * (SCREEN_HEIGHT * 0.5f));
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);

    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
}

void ProfileScreen_Load() {
    // Load Textures
    if (!pTexButtonLong)        pTexButtonLong = AEGfxTextureLoad("Assets/buttonLong_brown.png");
    if (!pTexButtonLongPressed) pTexButtonLongPressed = AEGfxTextureLoad("Assets/buttonLong_brown_pressed.png");
    if (!pTexButtonSquare)      pTexButtonSquare = AEGfxTextureLoad("Assets/buttonSquare_brown.png");
    if (!pTexInputRect)         pTexInputRect = AEGfxTextureLoad("Assets/input_outline_rectangle.png");
    if (!pTexCrossIcon)         pTexCrossIcon = AEGfxTextureLoad("Assets/iconCross_blue.png");
    if (!pTexPanel)             pTexPanel = AEGfxTextureLoad("Assets/panel_brown.png");
    if (!pTexEditIcon)          pTexEditIcon = AEGfxTextureLoad("Assets/edit_white.png");

    // Load saved profile data (overwrites the hardcoded defaults if a save exists)
    Profiles_Load();
}

void ProfileScreen_Initialize() {
    fontId = AEGfxCreateFont("Assets/liberation-mono.ttf", 26);
    if (fontId < 0)
        OutputDebugStringA("ERROR: Failed to load 'Assets/liberation-mono.ttf'.\n");

    // Create mesh for long buttons (profile slots)
    AEGfxMeshStart();
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f
    );
    AEGfxTriAdd(
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f
    );
    pMeshButtonLong = AEGfxMeshEnd();

    // Create mesh for square buttons (delete buttons)
    AEGfxMeshStart();
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f
    );
    AEGfxTriAdd(
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f
    );
    pMeshButtonSquare = AEGfxMeshEnd();
}

void ProfileScreen_Update() {
    // --- Popup active: handle text input ---
    if (popupActive) {
        // Confirm with Enter
        if (AEInputCheckTriggered(AEVK_RETURN)) {
            if (popupInputLen > 0 && popupSlotIndex >= 0 && popupSlotIndex < MAX_PROFILES) {
                strncpy_s(profiles[popupSlotIndex].name, PROFILE_NAME_MAX_LEN,
                    popupInputBuf, _TRUNCATE);
                if (!popupEditMode) {
                    // Creating a new profile - set economy defaults directly,
                    // do NOT flush live economy globals (Economy is not running yet)
                    profiles[popupSlotIndex].total_money = 0ULL;
                    profiles[popupSlotIndex].max_money = 255ULL;
                    profiles[popupSlotIndex].money_multiplier = 1.0f;
                    profiles[popupSlotIndex].exists = true;
                    Profiles_Save();
                }
                else {
                    // Renaming an existing in-game profile - flush live economy into slot
                    Economy_SaveToProfile(popupSlotIndex);
                }
                // In edit mode we only update the name; coins stay intact
            }
            popupActive = false;
            popupEditMode = false;
            popupSlotIndex = -1;
            popupInputBuf[0] = '\0';
            popupInputLen = 0;
        }
        // Cancel with Escape
        else if (AEInputCheckTriggered(AEVK_ESCAPE)) {
            popupActive = false;
            popupEditMode = false;
            popupSlotIndex = -1;
            popupInputBuf[0] = '\0';
            popupInputLen = 0;
        }
        // Backspace
        else if (AEInputCheckTriggered(AEVK_BACK) && popupInputLen > 0) {
            popupInputLen--;
            popupInputBuf[popupInputLen] = '\0';
        }
        else {
            // Printable ASCII characters (letters, numbers, spaces, symbols)
            for (u8 key = 32; key < 127; key++) {
                if (AEInputCheckTriggered(key)) {
                    bool shift = AEInputCheckCurr(AEVK_RSHIFT) || AEInputCheckCurr(AEVK_LSHIFT);
                    char c = (char)key;

                    // Letters: apply shift for uppercase
                    if (c >= 'A' && c <= 'Z') {
                        if (!shift) c = c + 32; // lowercase by default
                    }
                    // Digits row with shift = symbols
                    else if (shift) {
                        switch (c) {
                        case '1': c = '!'; break; case '2': c = '@'; break;
                        case '3': c = '#'; break; case '4': c = '$'; break;
                        case '5': c = '%'; break; case '6': c = '^'; break;
                        case '7': c = '&'; break; case '8': c = '*'; break;
                        case '9': c = '('; break; case '0': c = ')'; break;
                        case '-': c = '_'; break; case '=': c = '+'; break;
                        default: break;
                        }
                    }

                    if (popupInputLen < PROFILE_NAME_MAX_LEN - 1) {
                        popupInputBuf[popupInputLen++] = c;
                        popupInputBuf[popupInputLen] = '\0';
                    }
                }
            }
        }
        return; // Block all other input while popup is open
    }

    // --- Normal update: Escape to go back ---
    // Guard against popupActive so closing the popup's ESC never leaks through
    if (!popupActive && AEInputCheckTriggered(AEVK_ESCAPE)) {
        wentBack = true;
        nextState = GS_MAIN_SCREEN;
        selectMode = false;
    }
    // Mouse position in NDC
    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    float mNDC_X = ((float)mouseX / (SCREEN_WIDTH * 0.5f)) - 1.0f;
    float mNDC_Y = 1.0f - ((float)mouseY / (SCREEN_HEIGHT * 0.5f));

    float buttonW = PixelsToNDC_X(BUTTON_WIDTH_PX);
    float buttonH = PixelsToNDC_Y(BUTTON_HEIGHT_PX);
    float deleteSize = PixelsToNDC_X(DELETE_BUTTON_SIZE_PX);
    float editSize = PixelsToNDC_X(EDIT_BUTTON_SIZE_PX);
    float deleteH = deleteSize * (SCREEN_WIDTH / SCREEN_HEIGHT);
    float editH = editSize * (SCREEN_WIDTH / SCREEN_HEIGHT);
    float spacing = PixelsToNDC_Y(PROFILE_SPACING_PX);
    float startY = PixelsToNDC_Y(START_Y_PX);

    // Reset hover state each frame
    hoveredProfileSlot = -1;
    hoveredDeleteSlot = -1;
    hoveredEditSlot = -1;

    for (int i = 0; i < MAX_PROFILES; i++) {
        float yPos = startY - (i * spacing);
        float halfW = buttonW * 0.5f;
        float halfH = buttonH * 0.5f;

        if (profiles[i].exists) {
            // Check delete button hover
            float deleteX = buttonW * 0.5f + deleteSize * 1.0f;
            float editX = deleteX + deleteSize + editSize * 0.5f;
            float dHalfW = deleteSize * 0.5f;
            float dHalfH = deleteH * 0.5f;
            float eHalfW = editSize * 0.5f;
            float eHalfH = editH * 0.5f;

            //check delete button hover
            if (mNDC_X >= deleteX - dHalfW && mNDC_X <= deleteX + dHalfW &&
                mNDC_Y >= yPos - dHalfH && mNDC_Y <= yPos + dHalfH) {
                hoveredDeleteSlot = i;
            }
            //Check edit button hover
            else if (mNDC_X >= editX - eHalfW && mNDC_X <= editX + eHalfW &&
                mNDC_Y >= yPos - eHalfH && mNDC_Y <= yPos + eHalfH) {
                hoveredEditSlot = i;
            }
            // Check profile button hover (excluding delete area)
            else if (mNDC_X >= -halfW && mNDC_X <= halfW &&
                mNDC_Y >= yPos - halfH && mNDC_Y <= yPos + halfH) {
                hoveredProfileSlot = i;
            }
        }
        else {
            // Empty slot hover
            if (mNDC_X >= -halfW && mNDC_X <= halfW &&
                mNDC_Y >= yPos - halfH && mNDC_Y <= yPos + halfH) {
                hoveredProfileSlot = i;
            }
        }
    }

    if (AEInputCheckTriggered(VK_LBUTTON)) {
        for (int i = 0; i < MAX_PROFILES; i++) {
            float yPos = startY - (i * spacing);
            float halfW = buttonW * 0.5f;
            float halfH = buttonH * 0.5f;

            if (profiles[i].exists) {
                // Calculate button positions (MUST recalculate here!)
                float deleteX = buttonW * 0.5f + deleteSize * 1.0f;
                float editX = deleteX + deleteSize + editSize * 0.5f;

                float dHalfW = deleteSize * 0.5f;
                float dHalfH = deleteH * 0.5f;
                float eHalfW = editSize * 0.5f;
                float eHalfH = editH * 0.5f;

                // Delete button click
                if (mNDC_X >= deleteX - dHalfW && mNDC_X <= deleteX + dHalfW &&
                    mNDC_Y >= yPos - dHalfH && mNDC_Y <= yPos + dHalfH) {
                    profiles[i].exists = false;
                    profiles[i].name[0] = '\0';
                    profiles[i].play_time = 0.0f;
                    profiles[i].session_count = 0;
                    profiles[i].total_money = 0ULL;
                    profiles[i].max_money = 255ULL;
                    profiles[i].money_multiplier = 1.0f;
                    profiles[i].apples = profiles[i].pears = profiles[i].bananas = 0;
                    profiles[i].seeds[0] = profiles[i].seeds[1] = profiles[i].seeds[2] = 0;
                    Profiles_Save();
                    break;
                }
                // Edit button click - RENAME
                else if (mNDC_X >= editX - eHalfW && mNDC_X <= editX + eHalfW &&
                    mNDC_Y >= yPos - eHalfH && mNDC_Y <= yPos + eHalfH) {
                    popupActive = true;
                    popupEditMode = true;
                    popupSlotIndex = i;
                    strncpy_s(popupInputBuf, PROFILE_NAME_MAX_LEN, profiles[i].name, _TRUNCATE);
                    popupInputLen = (int)strnlen_s(popupInputBuf, PROFILE_NAME_MAX_LEN);
                    break;
                }
                // Profile button click - ALWAYS LOAD
                else if (mNDC_X >= -halfW && mNDC_X <= halfW &&
                    mNDC_Y >= yPos - halfH && mNDC_Y <= yPos + halfH) {
                    // Always load this profile
                    activeSlot = i;
                    selectMode = false;
                    wentBack = false;
                    Economy_LoadFromProfile(i);
                    Economy_SaveToProfile(i);
                    nextState = GS_MAIN_SCREEN;
                    break;
                }
            }
            else {
                // Empty slot click -> open create popup
                if (mNDC_X >= -halfW && mNDC_X <= halfW &&
                    mNDC_Y >= yPos - halfH && mNDC_Y <= yPos + halfH) {
                    popupActive = true;
                    popupEditMode = false;
                    popupSlotIndex = i;
                    popupInputBuf[0] = '\0';
                    popupInputLen = 0;
                    break;
                }
            }
        }
    }
}

void ProfileScreen_Render() {
    // Lighter background to contrast with brown buttons
    AEGfxSetBackgroundColor(0.08f, 0.06f, 0.04f);

    // Title - Centered at top
    if (fontId >= 0) {
        AEGfxPrint(fontId, selectMode ? "SELECT PROFILE TO LOAD" : "MANAGE PROFILES", -0.30f, 0.75f, 1.0f, 1.0f, 0.8f, 0.4f, 1.0f);
    }

    // Calculate center positions
    float buttonW = PixelsToNDC_X(BUTTON_WIDTH_PX);
    float buttonH = PixelsToNDC_Y(BUTTON_HEIGHT_PX);
    float deleteSize = PixelsToNDC_X(DELETE_BUTTON_SIZE_PX);
    float spacing = PixelsToNDC_Y(PROFILE_SPACING_PX);
    float startY = PixelsToNDC_Y(START_Y_PX);

    // Render up to 3 profile slots - Centered horizontally
    for (int i = 0; i < MAX_PROFILES; i++) {
        float yPos = startY - (i * spacing);

        if (profiles[i].exists) {
            // Shadow behind button
            DrawColoredQuad(0.0f, yPos - 0.005f, buttonW + 0.01f, buttonH + 0.01f,
                0.0f, 0.0f, 0.0f, 0.5f);

            // Hover brightens the profile button
            float btnTint = (hoveredProfileSlot == i) ? 1.35f : 1.0f;
            DrawTexturedQuad(pTexButtonLong, pMeshButtonLong,
                0.0f, yPos, buttonW, buttonH,
                btnTint, btnTint, btnTint, 1.0f);

            // Profile name - inside button, upper half
            if (fontId >= 0) {
                AEGfxPrint(fontId, profiles[i].name,
                    -0.10f, yPos + 0.02f,
                    0.75f, 1.0f, 0.95f, 0.8f, 1.0f);

                // coins - inside button, lower half
                char infoText[64];
                sprintf_s(infoText, sizeof(infoText), "Coins:%llu",
                    profiles[i].total_money);
                AEGfxPrint(fontId, infoText,
                    -0.13f, yPos - 0.03f,
                    0.5f, 0.85f, 0.75f, 0.6f, 1.0f);

                // Show edit hint on hover (same row as name, to the right)
                if (hoveredProfileSlot == i) {
                    const char* hint = "[click to load]";
                    AEGfxPrint(fontId, hint,
                        0.05f, yPos + 0.045f,
                        0.45f, 1.0f, 0.9f, 0.5f, 1.0f);
                }
            }

            // Delete (X) square button - right of long button
            float deleteH = deleteSize * (SCREEN_WIDTH / SCREEN_HEIGHT);
            float deleteX = buttonW * 0.5f + deleteSize * 1.0f;

            // Hover darkens the delete button
            float delTint = (hoveredDeleteSlot == i) ? 0.65f : 1.0f;

            DrawColoredQuad(deleteX + 0.003f, yPos - 0.003f,
                deleteSize + 0.006f, deleteH + 0.006f,
                0.0f, 0.0f, 0.0f, 0.5f);

            DrawTexturedQuad(pTexButtonSquare, pMeshButtonSquare,
                deleteX, yPos, deleteSize, deleteH,
                delTint, delTint, delTint, 1.0f);

            DrawTexturedQuad(pTexCrossIcon, pMeshButtonSquare,
                deleteX, yPos, deleteSize * 0.55f, deleteH * 0.55f,
                1.0f, 1.0f, 1.0f, 1.0f);

            float editSize = PixelsToNDC_X(EDIT_BUTTON_SIZE_PX);
            float editH = editSize * (SCREEN_WIDTH / SCREEN_HEIGHT);
            float editX = deleteX + deleteSize + editSize * 0.5f; // Position beside delete button

            // Hover tint for edit button
            float editTint = (hoveredEditSlot == i) ? 0.85f : 1.0f;

            // Edit button shadow
            DrawColoredQuad(editX + 0.003f, yPos - 0.003f,
                editSize + 0.006f, editH + 0.006f,
                0.0f, 0.0f, 0.0f, 0.5f);

            // Edit button background
            DrawTexturedQuad(pTexButtonSquare, pMeshButtonSquare,
                editX, yPos, editSize, editH,
                editTint, editTint, editTint, 1.0f);

            // Edit icon
            DrawTexturedQuad(pTexEditIcon, pMeshButtonSquare,
                editX, yPos, editSize * 0.55f, editH * 0.55f,
                1.0f, 1.0f, 1.0f, 1.0f);
        }

        else {
            // Empty Slot - brown button darkened to indicate create action
            DrawColoredQuad(0.0f, yPos - 0.005f, buttonW + 0.01f, buttonH + 0.01f,
                0.0f, 0.0f, 0.0f, 0.3f);

            // Hover brightens the empty slot slightly
            float emptyTint = (hoveredProfileSlot == i) ? 0.85f : 0.6f;
            float emptyG = (hoveredProfileSlot == i) ? 0.70f : 0.5f;
            float emptyB = (hoveredProfileSlot == i) ? 0.55f : 0.4f;
            DrawTexturedQuad(pTexButtonLong, pMeshButtonLong,
                0.0f, yPos, buttonW, buttonH,
                emptyTint, emptyG, emptyB, 1.0f);

            if (fontId >= 0) {
                AEGfxPrint(fontId, "+ NEW PROFILE",
                    -0.13f, yPos - 0.015f,
                    0.7f, 0.95f, 0.88f, 0.75f, 1.0f);
            }
        }
    }

    // Back instruction - centered at bottom
    if (fontId >= 0) {
        AEGfxPrint(fontId, "Press Esc to go back", -0.22f, -0.85f, 0.7f, 0.7f, 0.7f, 0.7f, 1.0f);
    }

    // --- Popup overlay ---
    if (popupActive) {
        // Dim background
        DrawColoredQuad(0.0f, 0.0f, 2.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.6f);

        // Panel size and position
        float panelW = PixelsToNDC_X(520.0f);
        float panelH = PixelsToNDC_Y(300.0f);

        // Panel background (brown panel texture)
        DrawTexturedQuad(pTexPanel, pMeshButtonLong,
            0.0f, 0.05f, panelW, panelH,
            1.0f, 1.0f, 1.0f, 1.0f);

        if (fontId >= 0) {
            // Reset render state before text so prior DrawTexturedQuad/DrawColoredQuad
            // calls don't leave a stale color multiplier that hides the text
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            // Title inside panel
            AEGfxPrint(fontId, popupEditMode ? "RENAME PROFILE" : "ENTER PROFILE NAME",
                -0.22f, 0.22f,
                0.75f, 1.0f, 0.9f, 0.6f, 1.0f);

            // Input field outline rectangle
            float inputW = PixelsToNDC_X(360.0f);
            float inputH = PixelsToNDC_Y(55.0f);
            DrawTexturedQuad(pTexInputRect, pMeshButtonLong,
                0.0f, 0.04f, inputW, inputH,
                1.0f, 1.0f, 1.0f, 1.0f);

            // Reset again after DrawTexturedQuad before printing typed text
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            // Typed text inside the input box (with blinking cursor bar)
            char displayText[PROFILE_NAME_MAX_LEN + 2];
            sprintf_s(displayText, sizeof(displayText), "%s|", popupInputBuf);
            AEGfxPrint(fontId, displayText,
                -0.20f, 0.025f,
                0.75f, 0.0f, 0.0f, 0.0f, 1.0f);

            // Reset before hint text too
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            // Hint text below input
            AEGfxPrint(fontId, "Press ENTER to confirm, ESC to cancel",
                -0.35f, -0.11f,
                0.45f, 0.75f, 0.75f, 0.75f, 1.0f);
        }
    }

    // Transition overlay
    if (TR_IsActive()) {
        float alpha = TR_GetAlpha();

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(0, 0, 0, alpha);

        AEMtx33 trans, scale, transform;
        // Fullscreen quad covers NDC space [-1,1], so scale = screen pixel size
        AEMtx33Scale(&scale, SCREEN_WIDTH, SCREEN_HEIGHT);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
    }
}

void ProfileScreen_Free() {
    // Free ProfileScreen-specific resources
    if (pMeshNextObject) {
        AEGfxMeshFree(pMeshNextObject);
        pMeshNextObject = NULL;
    }
    if (pMeshButtonLong) {
        AEGfxMeshFree(pMeshButtonLong);
        pMeshButtonLong = NULL;
    }
    if (pMeshButtonSquare) {
        AEGfxMeshFree(pMeshButtonSquare);
        pMeshButtonSquare = NULL;
    }
}

void ProfileScreen_Unload() {
    // Unload textures
    if (pTexButtonLong) {
        AEGfxTextureUnload(pTexButtonLong);
        pTexButtonLong = NULL;
    }
    if (pTexButtonLongPressed) {
        AEGfxTextureUnload(pTexButtonLongPressed);
        pTexButtonLongPressed = NULL;
    }
    if (pTexButtonSquare) {
        AEGfxTextureUnload(pTexButtonSquare);
        pTexButtonSquare = NULL;
    }
    if (pTexInputRect) {
        AEGfxTextureUnload(pTexInputRect);
        pTexInputRect = NULL;
    }
    if (pTexCrossIcon) {
        AEGfxTextureUnload(pTexCrossIcon);
        pTexCrossIcon = NULL;
    }
    if (pTexPanel) {
        AEGfxTextureUnload(pTexPanel);
        pTexPanel = NULL;
    }
}