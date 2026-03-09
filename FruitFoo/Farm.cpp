#include "Farm.h"
#include <Windows.h>
#include "AEEngine.h"

// ---------- FARM STATE (PROPOSAL-ALIGNED) ----------

static float growTimer = 0.0f;
static bool isPlanted = false;
static bool isReady = false;

const float GROW_TIME = 3.0f; // seconds to grow (short for testing)

void Farm_Load()
{
    OutputDebugStringA("Farm_Load\n");
}

void Farm_Initialize()
{
    OutputDebugStringA("Farm_Initialize\n");

    growTimer = 0.0f;
    isPlanted = true;
    isReady = false;

    OutputDebugStringA("Planting seed...\n");
}

void Farm_Update()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();

    if (isPlanted && !isReady)
    {
        growTimer += dt;

        // Print once per second (clean output)
        static int lastSecond = -1;
        int currentSecond = (int)growTimer;

        if (currentSecond != lastSecond)
        {
            lastSecond = currentSecond;
            char buffer[64];
            sprintf_s(buffer, "Growing... %ds\n", currentSecond);
            OutputDebugStringA(buffer);
        }

        if (growTimer >= GROW_TIME)
        {
            isReady = true;
            OutputDebugStringA("Crop ready to harvest!\n");
        }
    }

    // Simulate harvest with SPACE
    if (isReady && AEInputCheckTriggered(AEVK_SPACE))
    {
        OutputDebugStringA("Harvested crop!\n");

        // Reset cycle
        growTimer = 0.0f;
        isPlanted = true;
        isReady = false;

        OutputDebugStringA("Planting seed...\n");
    }
}

void Farm_Render()
{
    // No visuals yet (by design)
}

void Farm_Free()
{
    OutputDebugStringA("Farm_Free\n");
}

void Farm_Unload()
{
    OutputDebugStringA("Farm_Unload\n");
}
