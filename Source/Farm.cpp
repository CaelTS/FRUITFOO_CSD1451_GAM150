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
static AEGfxTexture* fruitStage25 = nullptr;
static AEGfxTexture* fruitStage50 = nullptr;
static AEGfxTexture* fruitStage75 = nullptr;
static AEGfxTexture* fruitStageFull = nullptr;
static AEGfxTexture* Droplet = nullptr;
static AEGfxTexture* Leaf = nullptr;


const float GROW_TIME = 8.0f;

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


    fruitStage25 = AEGfxTextureLoad("Assets/fruitStage25.png");
    fruitStage50 = AEGfxTextureLoad("Assets/fruitStage50.png");
    fruitStage75 = AEGfxTextureLoad("Assets/fruitStage75.png");
    fruitStageFull = AEGfxTextureLoad("Assets/fruitStageFull.png");
    Droplet = AEGfxTextureLoad("Assets/Droplet.png");
    Leaf = AEGfxTextureLoad("Assets/Leaf.png");


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
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetTransparency(1.0f);

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

        AEGfxSetColorToMultiply(1, 1, 1, 1);


        AEGfxTextureSet(plantedTexture, 0, 0);

        AEMtx33Scale(&scale, 120.0f, 120.0f);
        AEMtx33Trans(&trans, plotX, plotY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        AEGfxTexture* stageTexture = nullptr;
        //Growth Stages
        if (ratio < 0.25f)
            stageTexture = fruitStage25;
        else if (ratio < 0.5f)
            stageTexture = fruitStage50;
        else if (ratio < 0.75f)
            stageTexture = fruitStage75;
        else
            stageTexture = fruitStageFull;

        float size = 120.0f;

        // Pulse effect when fruit is ready
        if (farmPlots[i].isReady)
        {
            float pulse = sinf((float)clock() * 0.006f) * 3.0f;
            size += pulse;
        }


        AEGfxTextureSet(stageTexture, 0, 0);

        AEMtx33Scale(&scale, size, size);
        AEMtx33Trans(&trans, plotX, plotY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        if (ratio >= 0.9f && Leaf)
        {
            float leafSize = 25.0f;

            // gentle floating motion
            float leafX = plotX + sinf((float)clock() * 0.006f) * 15.0f;
            float leafY = plotY + cosf((float)clock() * 0.004f) * 10.0f;

            // small breathing scale
            float leafPulse = sinf((float)clock() * 0.008f) * 2.0f;
            leafSize += leafPulse;

            AEGfxTextureSet(Leaf, 0, 0);

            AEMtx33Scale(&scale, leafSize, leafSize);
            AEMtx33Trans(&trans, leafX, leafY);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
        }


        //Water
        if (ratio < 0.75f && Droplet)
        {
            float alpha = 0.8f + sinf((float)clock() * 0.01f) * 0.2f;
            AEGfxSetTransparency(alpha);
            float bob = sinf((float)clock() * 0.005f) * 3.0f;
            float decorSize = 30.0f;

            float offsetX = 40.0f;   // right side of plot
            float offsetY = 40.0f;   // top of plot

            float decorX = plotX + offsetX;
            float decorY = plotY + offsetY + bob;


            AEGfxTextureSet(Droplet, 0, 0);

            AEMtx33Scale(&scale, decorSize, decorSize);
            AEMtx33Trans(&trans, decorX, decorY);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            AEGfxSetTransparency(1.0f);
        }

        // --------------------------
        // 2 Draw Delete X AFTER
        // --------------------------
        if (deleteIcon)
        {
            float xSize = 40.0f;

            // Move LEFT instead of right
            float offsetX = -55.0f;   // move further left
            float offsetY = 55.0f;    // move higher

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

            if (success)
            {
                // resume growth past rhythm point
                plot.growTimer = GROW_TIME * 0.5f + 0.01f;
            }
            else
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