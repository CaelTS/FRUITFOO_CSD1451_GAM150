#pragma once

enum GameState {
    GS_MAIN_SCREEN = 0,
    GS_NEXT_SCREEN = 1,
    GS_EXIT = 2
};

// Function pointer type
typedef void(*FP)(void);

// GSM state variables
extern int current, previous, next;

// Function pointers for current state
extern FP fpLoad, fpInitialize, fpUpdate, fpDraw, fpFree, fpUnload;

void GSM_Initialize(int startingState);
void GSM_Update();