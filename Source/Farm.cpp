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
    bool isUnlocked = false;

    bool isPlanted = false;
    bool isReady = false;
    float growTimer = 0.0f;
    int seedType = -1;

    bool rhythmTriggered = false;
    bool waitingForRhythm = false;

};
static bool g_rhythmUsed = false;
static bool g_requestRhythm = false;
static int  g_rhythmPlotIndex = -1;
static bool g_rhythmPaused = false;


static std::vector<FarmPlot> farmPlots;
static AEGfxTexture* plantedTexture = nullptr;
static AEGfxTexture* deleteIcon = nullptr;
static AEGfxTexture* fruitStage25 = nullptr;
static AEGfxTexture* fruitStage50 = nullptr;
static AEGfxTexture* fruitStage75 = nullptr;
static AEGfxTexture* fruitStageFull = nullptr;
static AEGfxTexture* Droplet = nullptr;
static AEGfxTexture* Leaf = nullptr;
static AEGfxTexture* rhythmPrompt = nullptr;
static AEGfxTexture* tickIcon = nullptr;
static AEGfxTexture* crossIcon = nullptr;
static AEGfxTexture* lockedPlot = nullptr;


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
    rhythmPrompt = AEGfxTextureLoad("Assets/rhythmprompt.png");
    tickIcon = AEGfxTextureLoad("Assets/tick.png");
    crossIcon = AEGfxTextureLoad("Assets/cross.png");
    lockedPlot = AEGfxTextureLoad("Assets/lockedplot.png");



}

void Farm_Initialize()
{
    farmPlots.clear();
    farmPlots.resize(4);

    farmPlots[0].isUnlocked = true; // first plot available


}

// ------------------------------------------------------------
// UPDATE
// ------------------------------------------------------------

//getter for unlocked plots/locked plots
bool Farm_IsPlotLocked(int index)
{
    if (index == 0)
        return false;   // first plot always unlocked

    return true;        // others locked for now
}

void Farm_Update()
{


    if (!UI_IsMenuOpen())
        return;
    float dt = (float)AEFrameRateControllerGetFrameTime();

    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    // convert to world coordinates
    float worldX = (float)mouseX - 800.0f;
    float worldY = 450.0f - (float)mouseY;
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



    for (int i = 0; i < farmPlots.size(); i++)
    {

        FarmPlot& plot = farmPlots[i];

        if (!plot.isPlanted)
            continue;

        if (plot.waitingForRhythm)
        {
            if (AEInputCheckTriggered(AEVK_LBUTTON))
            {
                float iconSize = 50.0f;

                // Tick position
                float tickX = -70.0f;
                float tickY = -40.0f;

                // Cross position
                float crossX = 70.0f;
                float crossY = -40.0f;

                // --- Tick Click ---
                if (worldX >= tickX - iconSize / 2 &&
                    worldX <= tickX + iconSize / 2 &&
                    worldY >= tickY - iconSize / 2 &&
                    worldY <= tickY + iconSize / 2)
                {
                    plot.waitingForRhythm = false;

                    g_requestRhythm = true;
                    g_rhythmPlotIndex = i;

                    std::cout << "Tick clicked\n";
                    return;
                }

                // --- Cross Click ---
                if (worldX >= crossX - iconSize / 2 &&
                    worldX <= crossX + iconSize / 2 &&
                    worldY >= crossY - iconSize / 2 &&
                    worldY <= crossY + iconSize / 2)
                {
                    plot.waitingForRhythm = false;
                    g_rhythmPlotIndex = -1;

                    std::cout << "Cross clicked\n";
                    return;
                }
            }

            continue;
        }
        // Pause growth if rhythm is starting
        if (g_requestRhythm)
            continue;

        // Grow
        plot.growTimer += dt;

        float ratio = plot.growTimer / GROW_TIME;

        if (ratio >= 0.5f && !plot.rhythmTriggered && g_rhythmPlotIndex == -1)
        {
            plot.rhythmTriggered = true;
            plot.waitingForRhythm = true;
            g_rhythmPlotIndex = i;
        }

        if (ratio >= 1.0f)
            plot.isReady = true;
    }




}

// ------------------------------------------------------------
// RENDER
// ------------------------------------------------------------

void Farm_Render()
{
    if (!UI_IsMenuOpen())
        return;
    if (!plantedTexture)
        return;

    AEMtx33 scale, trans, transform;
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetTransparency(1.0f);

    for (int i = 0; i < farmPlots.size(); i++)
    {


        // Get plot center once
        float plotX = UI_GetPlotSlotX(i);
        float plotY = UI_GetPlotSlotY(i);


        FarmPlot& plot = farmPlots[i];

        // --------------------------
        // DRAW LOCKED PLOT
        // --------------------------
        if (!plot.isUnlocked)
        {
            float size = 120.0f;

            AEGfxTextureSet(lockedPlot, 0, 0);

            AEMtx33Scale(&scale, size, size);
            AEMtx33Trans(&trans, plotX, plotY);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            continue;
        }

        if (!plot.isPlanted)
            continue;
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


        if (farmPlots[i].waitingForRhythm)
        {
            AEGfxSetColorToMultiply(1.0f, 0.9f, 0.6f, 1.0f);
        }

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
        // Draw Rhythm UI (CENTER SCREEN)
        // --------------------------
        if (farmPlots[i].waitingForRhythm && g_rhythmPlotIndex == i)
        {
            AEGfxSetColorToMultiply(1, 1, 1, 1);   // reset tint
            // Draw prompt in screen center
            if (rhythmPrompt)
            {
                float promptW = 350.0f;
                float promptH = 200.0f;

                AEGfxTextureSet(rhythmPrompt, 0, 0);

                AEMtx33Scale(&scale, promptW, promptH);
                AEMtx33Trans(&trans, 0.0f, 0.0f);
                AEMtx33Concat(&transform, &trans, &scale);

                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }

            float iconSize = 50.0f;

            //  Tick
            if (tickIcon)
            {
                float tickX = -70.0f;
                float tickY = -40.0f;

                AEGfxTextureSet(tickIcon, 0, 0);

                AEMtx33Scale(&scale, iconSize, iconSize);
                AEMtx33Trans(&trans, tickX, tickY);
                AEMtx33Concat(&transform, &trans, &scale);

                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }

            //  Cross
            if (crossIcon)
            {
                float crossX = 70.0f;
                float crossY = -40.0f;

                AEGfxTextureSet(crossIcon, 0, 0);

                AEMtx33Scale(&scale, iconSize, iconSize);
                AEMtx33Trans(&trans, crossX, crossY);
                AEMtx33Concat(&transform, &trans, &scale);

                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }
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
    g_rhythmUsed = false;

    if (plotIndex < 0 || plotIndex >= farmPlots.size())
        return;

    FarmPlot& plot = farmPlots[plotIndex];

    if (plot.isPlanted)
        return;

    plot.isPlanted = true;
    plot.isReady = false;
    plot.seedType = seedType;
    plot.growTimer = 0.0f;
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