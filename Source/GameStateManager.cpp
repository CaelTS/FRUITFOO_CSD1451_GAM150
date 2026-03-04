#include "GameStateManager.h"
#include "AEEngine.h"
#include "Main.h"
#include "Profile.h"
#include "Farm.h"
#include "Rhythm.h"
#include <iostream>


// GSM state variables
int current = 0, previous = 0, next = 0;

// Function pointers for current state
FP fpLoad = nullptr, fpInitialize = nullptr, fpUpdate = nullptr,
fpDraw = nullptr, fpFree = nullptr, fpUnload = nullptr;

void GSM_Initialize(int startingState)
{
    previous = current = -1;
    next = startingState;

    fpLoad = fpInitialize = fpUpdate =
        fpDraw = fpFree = fpUnload = nullptr;
}



void GSM_Update()
{

    std::cout << "Current state: " << current << "\n";
    switch (current)
    {
    case GS_MAIN_SCREEN:
        fpLoad = MainScreen_Load;
        fpInitialize = MainScreen_Initialize;
        fpUpdate = MainScreen_Update;
        fpDraw = MainScreen_Render;
        fpFree = MainScreen_Free;
        fpUnload = MainScreen_Unload;
        break;

    case GS_NEXT_SCREEN:
        fpLoad = ProfileScreen_Load;
        fpInitialize = ProfileScreen_Initialize;
        fpUpdate = ProfileScreen_Update;
        fpDraw = ProfileScreen_Render;
        fpFree = ProfileScreen_Free;
        fpUnload = ProfileScreen_Unload;
        break;

    case GS_FARM_SCREEN:
        fpLoad = Farm_Load;
        fpInitialize = Farm_Initialize;
        fpUpdate = Farm_Update;
        fpDraw = Farm_Render;
        fpFree = Farm_Free;
        fpUnload = Farm_Unload;
        break;

    case GS_RHYTHM_SCREEN:
        fpLoad = Rhythm_Load;
        fpInitialize = Rhythm_Initialize;
        fpUpdate = Rhythm_Update;
        fpDraw = Rhythm_Render;
        fpFree = Rhythm_Free;
        fpUnload = Rhythm_Unload;
        break;

    case GS_EXIT:
        fpLoad = fpInitialize = fpUpdate =
            fpDraw = fpFree = fpUnload = nullptr;
        break;
    }
}
