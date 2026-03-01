#include "Farm.h"
#include "AEEngine.h"
#include "UI.h"
#include <iostream>
#include <vector>

extern AEGfxVertexList* g_pMeshFullScreen;

// ------------------------------------------------------------
// FARM DATA STRUCTURE
// ------------------------------------------------------------

struct FarmPlot
{
    bool isPlanted = false;
    bool isReady = false;
    float growTimer = 0.0f;
    int seedType = -1;
};

static std::vector<FarmPlot> farmPlots;
static AEGfxTexture* plantedTexture = nullptr;

const float GROW_TIME = 3.0f;

// ------------------------------------------------------------
// LOAD / INITIALIZE
// ------------------------------------------------------------

void Farm_Load()
{
    std::cout << "Farm_Load\n";

    plantedTexture = AEGfxTextureLoad("Assets/PlotPlant.png");

    if (!plantedTexture)
        std::cout << "FAILED TO LOAD PlotPlant.png\n";
    else
        std::cout << "PlotPlant.png loaded successfully\n";
}

void Farm_Initialize()
{
    std::cout << "Farm_Initialize\n";

    farmPlots.clear();
    farmPlots.resize(4);   // 4 plot slots
}

// ------------------------------------------------------------
// UPDATE
// ------------------------------------------------------------

void Farm_Update()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();

    for (auto& plot : farmPlots)
    {
        if (!plot.isPlanted || plot.isReady)
            continue;

        plot.growTimer += dt;

        if (plot.growTimer >= GROW_TIME)
        {
            plot.isReady = true;
            std::cout << "Plot ready!\n";
        }
    }

    // Harvest all ready plots with SPACE
    if (AEInputCheckTriggered(AEVK_SPACE))
    {
        for (auto& plot : farmPlots)
        {
            if (plot.isReady)
            {
                plot.isPlanted = false;
                plot.isReady = false;
                plot.seedType = -1;
                plot.growTimer = 0.0f;

                std::cout << "Harvested plot\n";
            }
        }
    }
}

// ------------------------------------------------------------
// RENDER
// ------------------------------------------------------------

void Farm_Render()
{
    if (!plantedTexture)
        return;

    AEMtx33 scale, trans, transform;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1, 1, 1, 1);

    for (int i = 0; i < farmPlots.size(); i++)
    {
        if (!farmPlots[i].isPlanted)
            continue;

        float plotX = UI_GetPlotSlotX(i);
        float plotY = UI_GetPlotSlotY(i);

        AEGfxTextureSet(plantedTexture, 0, 0);

        AEMtx33Scale(&scale, 120.0f, 120.0f);
        AEMtx33Trans(&trans, plotX, plotY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
    }
  //  std::cout << "Rendering farm...\n";
 
}

// ------------------------------------------------------------
// FREE / UNLOAD
// ------------------------------------------------------------

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
// PUBLIC FUNCTIONS
// ------------------------------------------------------------

void Farm_PlantSeed(int plotIndex, int seedType)
{
    if (plotIndex < 0 || plotIndex >= farmPlots.size())
        return;

    FarmPlot& plot = farmPlots[plotIndex];

    if (plot.isPlanted)
        return;

    plot.isPlanted = true;
    plot.isReady = false;
    plot.seedType = seedType;
    plot.growTimer = 0.0f;
    std::cout << "Planting at plot: " << plotIndex << "\n";
}

bool Farm_IsPlotPlanted(int plotIndex)
{
    if (plotIndex < 0 || plotIndex >= farmPlots.size())
        return false;

    return farmPlots[plotIndex].isPlanted;
}

