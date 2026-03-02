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

    bool rhythmTriggered = false;
    bool waitingForRhythm = false;
};

static bool g_requestRhythm = false;
static int  g_rhythmPlotIndex = -1;

static std::vector<FarmPlot> farmPlots;
static AEGfxTexture* plantedTexture = nullptr;
static AEGfxTexture* deleteIcon = nullptr;


const float GROW_TIME = 3.0f;

// ------------------------------------------------------------
// LOAD / INITIALIZE
// ------------------------------------------------------------

void Farm_Load()
{
    std::cout << "Farm_Load\n";

    plantedTexture = AEGfxTextureLoad("Assets/PlotPlant.png");
    deleteIcon = AEGfxTextureLoad("Assets/X.png");

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

    for (int i = 0; i < farmPlots.size(); i++)
    {
        FarmPlot& plot = farmPlots[i];

        if (!plot.isPlanted)
            continue;

        if (plot.waitingForRhythm)
            continue;

        // Grow
        plot.growTimer += dt;

        float ratio = plot.growTimer / GROW_TIME;

        // Trigger rhythm at 50%
        if (ratio >= 0.5f && !plot.rhythmTriggered)
        {
            plot.rhythmTriggered = true;
            plot.waitingForRhythm = true;

            g_requestRhythm = true;
            g_rhythmPlotIndex = i;

            break;
        }

        if (ratio >= 1.0f)
        {
            plot.isReady = true;
        }
    }

    // Harvest
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
                plot.rhythmTriggered = false;
                plot.waitingForRhythm = false;
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

        // Get plot center once
        float plotX = UI_GetPlotSlotX(i);
        float plotY = UI_GetPlotSlotY(i);

        // --------------------------
        // 1 Draw Apple FIRST
        // --------------------------
        float ratio = farmPlots[i].growTimer / GROW_TIME;
        if (ratio > 1.0f) ratio = 1.0f;

        // --------------------------
        // Draw Apple (base)
        // --------------------------
        AEGfxTextureSet(plantedTexture, 0, 0);

        AEMtx33Scale(&scale, 120.0f, 120.0f);
        AEMtx33Trans(&trans, plotX, plotY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);


        // --------------------------
        // 2 Draw Delete X AFTER
        // --------------------------
        if (deleteIcon)
        {
            float xSize = 25.0f;

            // Move LEFT instead of right
            float offsetX = -45.0f;
            float offsetY = 45.0f;   // keep it at top

            float xPos = plotX + offsetX;
            float yPos = plotY + offsetY;

            AEGfxTextureSet(deleteIcon, 0, 0);

            AEMtx33Scale(&scale, xSize, xSize);
            AEMtx33Trans(&trans, xPos, yPos);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
        }
    }
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


    if (deleteIcon)
    {
        AEGfxTextureUnload(deleteIcon);
        deleteIcon = nullptr;
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

void Farm_ClearPlot(int index)
{
    if (index < 0 || index >= farmPlots.size())
        return;

    farmPlots[index].isPlanted = false;
    farmPlots[index].isReady = false;
    farmPlots[index].growTimer = 0.0f;
    farmPlots[index].seedType = -1;
}

void Farm_OnRhythmResult(bool success)
{
    for (auto& plot : farmPlots)
    {
        if (plot.waitingForRhythm)
        {
            plot.waitingForRhythm = false;

            if (!success)
            {
                plot.growTimer -= 0.5f;
                if (plot.growTimer < 0.0f)
                    plot.growTimer = 0.0f;
            }

            break;
        }
    }
}


bool Farm_ShouldStartRhythm()
{
    return g_requestRhythm;
}

int Farm_GetRhythmPlotIndex()
{
    return g_rhythmPlotIndex;
}

void Farm_ClearRhythmRequest()
{
    g_requestRhythm = false;
    g_rhythmPlotIndex = -1;
}