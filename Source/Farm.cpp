#include "Farm.h"
#include "AEEngine.h"
#include <iostream>

extern AEGfxVertexList* g_pMeshFullScreen;   
// ------------------------------------------------------------
// FARM STATE
// ------------------------------------------------------------

static float growTimer = 0.0f;
static bool isPlanted = false;
static bool isReady = false;
static int plantedSeedType = -1;

AEGfxTexture* plantedTexture = nullptr;

const float GROW_TIME = 3.0f; // seconds
static const float plotX = -630.0f;
static const float plotY = 150.0f;

// ------------------------------------------------------------

void Farm_Load()
{
    std::cout << "Farm_Load\n";

    plantedTexture = AEGfxTextureLoad("Assets/First Plot_Plant_v1_solo.png");

    if (!plantedTexture)
        std::cout << "ERROR: Plant texture failed to load\n";
}

void Farm_Initialize()
{
    std::cout << "Farm_Initialize\n";

    growTimer = 0.0f;
    isPlanted = false;     // IMPORTANT: start empty
    isReady = false;
    plantedSeedType = -1;
}

void Farm_Update()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();

    // Growing logic
    if (isPlanted && !isReady)
    {
        growTimer += dt;

        static int lastSecond = -1;
        int currentSecond = (int)growTimer;

        if (currentSecond != lastSecond)
        {
            lastSecond = currentSecond;
            std::cout << "Growing... " << currentSecond << "s\n";
        }

        if (growTimer >= GROW_TIME)
        {
            isReady = true;
            std::cout << "Crop ready to harvest!\n";
        }
    }

    // Harvest with SPACE
    if (isReady && AEInputCheckTriggered(AEVK_SPACE))
    {
        std::cout << "Harvested crop!\n";

        isPlanted = false;
        isReady = false;
        plantedSeedType = -1;
        growTimer = 0.0f;
    }
}

void Farm_Render()
{
    if (!isPlanted)
        return;

    float width = 120.0f;
    float height = 120.0f;

    AEMtx33 scale, trans, transform;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    AEGfxTextureSet(plantedTexture, 0, 0);

    AEMtx33Scale(&scale, width, height);
    AEMtx33Trans(&trans, plotX, plotY);
    AEMtx33Concat(&transform, &trans, &scale);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
}

void Farm_Free()
{
    std::cout << "Farm_Free\n";
}

void Farm_Unload()
{
    if (plantedTexture)
    {
        AEGfxTextureUnload(plantedTexture);
        plantedTexture = nullptr;
    }

    std::cout << "Farm_Unload\n";
}

// ------------------------------------------------------------
// PUBLIC FUNCTION (Called from UI)
// ------------------------------------------------------------

void Farm_PlantSeed(int seedType)
{
    if (isPlanted)
        return; // already occupied

    plantedSeedType = seedType;
    isPlanted = true;
    isReady = false;
    growTimer = 0.0f;

    std::cout << "Seed planted!\n";
}