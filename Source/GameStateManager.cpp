#include "GameStateManager.h"
#include "AEEngine.h"
#include "Main.h"
#include "Next.h"

// GSM state variables
int current = 0, previous = 0, next = 0;

// Function pointers for current state
FP fpLoad = nullptr, fpInitialize = nullptr, fpUpdate = nullptr,
fpDraw = nullptr, fpFree = nullptr, fpUnload = nullptr;

void GSM_Initialize(int startingState) {
    current = previous = next = startingState;

    // Initialize function pointers for starting state
    GSM_Update();
}

void GSM_Update() {
    // Set function pointers based on current state
    switch (current) {
    case GS_MAIN_SCREEN:
        fpLoad = MainScreen_Load;
        fpInitialize = MainScreen_Initialize;
        fpUpdate = MainScreen_Update;
        fpDraw = MainScreen_Render;
        fpFree = MainScreen_Free;
        fpUnload = MainScreen_Unload;
        break;

    case GS_NEXT_SCREEN:
        fpLoad = NextScreen_Load;
        fpInitialize = NextScreen_Initialize;
        fpUpdate = NextScreen_Update;
        fpDraw = NextScreen_Render;
        fpFree = NextScreen_Free;
        fpUnload = NextScreen_Unload;
        break;

    case GS_EXIT:
        // Clear function pointers for exit state
        fpLoad = nullptr;
        fpInitialize = nullptr;
        fpUpdate = nullptr;
        fpDraw = nullptr;
        fpFree = nullptr;
        fpUnload = nullptr;
        break;
    }

    if (next != current) {
        previous = current;
        current = next;
    }
}