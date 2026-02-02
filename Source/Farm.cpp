#include "Farm.h"
#include <Windows.h>
#include "AEEngine.h"
#include <iostream>

// ---------- FARM STATE (PROPOSAL-ALIGNED) ----------

static float growTimer = 0.0f;
static bool isPlanted = false;
static bool isReady = false;

const float GROW_TIME = 3.0f; // seconds to grow (short for testing)

void Farm_Load()
{
    std::cout << "Farm_Load\n";
}

void Farm_Initialize()
{
    std::cout << "Farm_Initialize\n";

    growTimer = 0.0f;
    isPlanted = true;
    isReady = false;

    std::cout << "Planting seed...\n";
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
            std::cout << (buffer);
        }

        if (growTimer >= GROW_TIME)
        {
            isReady = true;
            std::cout << "Crop ready to harvest!\n";
        }
    }

    // Simulate harvest with SPACE
    if (isReady && AEInputCheckTriggered(AEVK_SPACE))
    {
        std::cout << "Harvested crop!\n";

        // Reset cycle
        growTimer = 0.0f;
        isPlanted = true;
        isReady = false;

        std::cout << "Planting seed...\n";
    }
}

void Farm_Render()
{
    // No visuals yet (by design)
}

void Farm_Free()
{
    std::cout << "Farm_Free\n";
}

void Farm_Unload()
{
    std::cout << "Farm_Unload\n";
}
