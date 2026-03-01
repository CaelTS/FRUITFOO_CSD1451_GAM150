
#pragma once

void Farm_Load();
void Farm_Initialize();
void Farm_Update();
void Farm_Render();
void Farm_Free();
void Farm_Unload();

void Farm_PlantSeed(int seedType);


struct Plot
{
    float x;
    float y;
    int plantedSeed;   // -1 = empty
};

void Farm_PlantSeed(int plotIndex, int seedType);
bool Farm_IsPlanted();