#pragma once

enum GameState {
    GS_SPLASH = 0,
    GS_START_SCREEN = 1,
    GS_MAIN_SCREEN = 2,
    GS_FARM_SCREEN = 3,
    GS_NEXT_SCREEN = 4,
    GS_RHYTHM_SCREEN = 5,
    GS_EXIT = 6,
    GS_CREDITS = 7
};

// Function pointer type
typedef void(*FP)(void);

// GSM state variables
extern int currentState, previousState, nextState;

// Function pointers for current state
extern FP fpLoad, fpInitialize, fpUpdate, fpDraw, fpFree, fpUnload;

void GSM_Initialize(int startingState);
void GSM_Update();