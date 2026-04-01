#include "GameStateManager.h"
#include "AEEngine.h"
#include "Main.h"
#include "Profile.h"
#include "Farm.h"
#include "Rhythm.h"
#include "StartScreen.h"
#include "Splash.h"
#include <iostream>


// GSM state variables
int currentState = 0, previousState = 0, nextState = 0;

// Function pointers for current state
FP fpLoad = nullptr, fpInitialize = nullptr, fpUpdate = nullptr,
fpDraw = nullptr, fpFree = nullptr, fpUnload = nullptr;

void GSM_Initialize(int startingState)
{
    previousState = currentState = -1;
    nextState = startingState;

    fpLoad = fpInitialize = fpUpdate =
        fpDraw = fpFree = fpUnload = nullptr;
}



// ---------------------------------------------------------------------------
// StartScreen GSM wrappers
// StartScreen uses Init/Update/Draw - map them to the GSM lifecycle slots

//static void StartScreen_GSM_Load() { StartScreen_Load(); }
static void StartScreen_GSM_Initialize() { StartScreen_Init(); }
static void StartScreen_GSM_Update()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();
    StartScreen_Update(dt);

    if (!StartScreen_IsActive())
    {
        if (nextState == currentState) // nothing chosen yet
            nextState = GS_MAIN_SCREEN;
        // else: Continue set GS_NEXT_SCREEN, New Game set GS_MAIN_SCREEN, etc. � keep it
    }
}
static void StartScreen_GSM_Render() { StartScreen_Draw(); }
static void StartScreen_GSM_Free() { /* nothing to free */ }
static void StartScreen_GSM_Unload() { StartScreen_Unload(); }
// ---------------------------------------------------------------------------

void GSM_Update()
{
    std::cout << "Current state: " << currentState << "\n";
    switch (currentState)
    {
    case GS_SPLASH:
        fpLoad = Splash_Load;
        fpInitialize = Splash_Initialize;
        fpUpdate = Splash_Update;
        fpDraw = Splash_Draw;
        fpFree = Splash_Free;
        fpUnload = Splash_Unload;
        break;

    case GS_START_SCREEN:
        /*fpLoad = StartScreen_GSM_Load;*/
        fpInitialize = StartScreen_GSM_Initialize;
        fpUpdate = StartScreen_GSM_Update;
        fpDraw = StartScreen_GSM_Render;
        fpFree = StartScreen_GSM_Free;
        fpUnload = StartScreen_GSM_Unload;
        break;

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