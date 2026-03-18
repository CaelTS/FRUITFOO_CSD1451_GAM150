#pragma once

void Farm_Load();
void Farm_Initialize();
void Farm_Update();
void Farm_Render();
void Farm_Free();
void Farm_Unload();

struct Plot
{
    float x;
    float y;
    int plantedSeed;   // -1 = empty
};

void Farm_PlantSeed(int plotIndex, int seedType);
bool Farm_IsPlotPlanted(int plotIndex);
void Farm_ClearPlot(int index);
bool Farm_IsPlotLocked(int index);

bool Farm_ShouldStartRhythm();
int  Farm_GetRhythmPlotIndex();
void Farm_ClearRhythmRequest();
void Farm_ClearRhythmFlag();
void Farm_OnRhythmResult(bool success);

void Farm_SetRhythmPaused(bool paused);
bool Farm_IsRhythmPaused();
bool Farm_IsWaitingForRhythm();
int  Farm_GetRhythmSeedType(); // seed type of the plot that triggered rhythm