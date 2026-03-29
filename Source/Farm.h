#pragma once
#include <vector>

// ------------------------------------------------------------
// FARM DATA STRUCTURE (EXPOSED TO OTHER FILES)
// ------------------------------------------------------------
struct FarmPlot
{
    bool isUnlocked = false;

    bool isPlanted = false;
    bool isReady = false;
    float growTimer = 0.0f;
    int seedType = -1;

    bool rhythmTriggered = false;
    bool waitingForRhythm = false;
    bool growthFrozen = false;

    // Track which growth milestones have been reached for audio
    bool milestoneReached[4] = { false, false, false, false };  // 0=25%, 1=50%, 2=75%, 3=100%
};

// ------------------------------------------------------------
// GLOBAL ACCESS (FOR HELPER, UI, ETC)
// ------------------------------------------------------------
extern std::vector<FarmPlot> farmPlots;

// ------------------------------------------------------------
// CORE FARM FUNCTIONS
// ------------------------------------------------------------
void Farm_Load();
void Farm_Initialize();
void Farm_Update();
void Farm_Render();
void Farm_Free();
void Farm_Unload();

// ------------------------------------------------------------
// PLOT INTERACTION
// ------------------------------------------------------------
void Farm_PlantSeed(int plotIndex, int seedType);
bool Farm_IsPlotPlanted(int plotIndex);
void Farm_ClearPlot(int index);
bool Farm_IsPlotLocked(int index);

// Setter to change a plot's unlocked state (updates profile)
void Farm_SetPlotUnlocked(int index, bool unlocked);

// ------------------------------------------------------------
// RHYTHM SYSTEM
// ------------------------------------------------------------
bool Farm_ShouldStartRhythm();
int  Farm_GetRhythmPlotIndex();
void Farm_ClearRhythmRequest();
void Farm_ClearRhythmFlag();
void Farm_OnRhythmResult(bool success);

void Farm_SetRhythmPaused(bool paused);
bool Farm_IsRhythmPaused();
bool Farm_IsWaitingForRhythm();
int  Farm_GetRhythmSeedType();
float Farm_GetGrowTime();