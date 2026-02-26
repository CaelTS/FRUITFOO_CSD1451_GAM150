#pragma once

enum GameState {
    GS_MAIN_SCREEN = 0,
    GS_FARM_SCREEN = 1,
    GS_NEXT_SCREEN = 2,
    GS_RHYTHM_SCREEN = 3,
    GS_EXIT = 4
};

// Function pointer type
typedef void(*FP)(void);

// GSM state variables
extern int current, previous, next;

// Function pointers for current state
extern FP fpLoad, fpInitialize, fpUpdate, fpDraw, fpFree, fpUnload;

void GSM_Initialize(int startingState);
void GSM_Update();