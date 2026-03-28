#include "Farm.h"
#include "AEEngine.h"
#include "AEAudio.h"
#include "UI.h"
#include "Profile.h"
#include <iostream>
#include <vector>
#include <cmath>
#include "Inventory.h"
#include "Crate.h"
#include "UIAudio.h"

extern AEGfxVertexList* g_pMeshFullScreen;

// ------------------------------------------------------------
// FARM AUDIO
// ------------------------------------------------------------
static AEAudio      g_farmTickSFX;
static AEAudioGroup g_farmTickGroup;
static AEAudio      g_farmHarvestSFX;
static AEAudioGroup g_farmHarvestGroup;

// ------------------------------------------------------------
// FARM DATA STRUCTURE
// ------------------------------------------------------------

//struct FarmPlot
//{
  //  bool isUnlocked = false;

  //  bool isPlanted = false;
   // bool isReady = false;
   // float growTimer = 0.0f;
   // int seedType = -1;

 //   bool rhythmTriggered = false;
  //  bool waitingForRhythm = false;
  //  bool growthFrozen = false;  // NEW: permanently freeze growth when X is clicked
//};

static bool g_rhythmUsed = false;
static bool g_requestRhythm = false;
static int  g_rhythmPlotIndex = -1;
static bool g_rhythmPaused = false;
static int  g_rhythmPausedPlotIndex = -1;

// ---------------------------------------------------------------------------
// Harvest destination popup
// When SPACE is pressed on a ready plot, we pause and ask the player:
//   [Inventory]   [Crate]
// The plot index waiting for a choice is stored here (-1 = none pending).
// ---------------------------------------------------------------------------
static int  g_harvestPopupPlotIndex = -1;  // which plot is waiting for destination choice
static bool g_harvestPopupOpen = false;


std::vector<FarmPlot> farmPlots;
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
static AEGfxTexture* fruitAppleTexture = nullptr;

// ---------------------------------------------------------------------------
// Flying fruit animation (harvest -> crate)
// ---------------------------------------------------------------------------
struct FlyingFruit
{
    float startX, startY;   // origin: plot position
    float endX, endY;     // destination: crate slot position
    float t;                // 0..1 progress
    float duration;         // seconds for full flight
    bool  active;
    int   fruitType;        // 0=apple, 1=pear, 2=banana
};

static constexpr int MAX_FLYING = 8;
static FlyingFruit g_flyingFruits[MAX_FLYING];

static void FlyingFruit_Init()
{
    for (int i = 0; i < MAX_FLYING; i++)
        g_flyingFruits[i].active = false;
}

static void FlyingFruit_Spawn(float sx, float sy, float ex, float ey, int fruitType)
{
    for (int i = 0; i < MAX_FLYING; i++)
    {
        if (!g_flyingFruits[i].active)
        {
            g_flyingFruits[i] = { sx, sy, ex, ey, 0.0f, 0.7f, true, fruitType };
            return;
        }
    }
}

static void FlyingFruit_Update(float dt)
{
    for (int i = 0; i < MAX_FLYING; i++)
    {
        if (!g_flyingFruits[i].active) continue;
        g_flyingFruits[i].t += dt / g_flyingFruits[i].duration;
        if (g_flyingFruits[i].t >= 1.0f)
            g_flyingFruits[i].active = false;
    }
}

// Quadratic bezier helper: arc midpoint is raised in Y for a nice arc
static float BezierX(float t, float x0, float x1, float x2)
{
    return (1 - t) * (1 - t) * x0 + 2 * (1 - t) * t * x1 + t * t * x2;
}
static float BezierY(float t, float y0, float y1, float y2)
{
    return (1 - t) * (1 - t) * y0 + 2 * (1 - t) * t * y1 + t * t * y2;
}

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
    fruitAppleTexture = AEGfxTextureLoad("Assets/Fruit_Apple.png");

    // Load farm growth sounds
    g_farmTickSFX = AEAudioLoadSound("Assets/Tick.wav");
    g_farmTickGroup = AEAudioCreateGroup();
    g_farmHarvestSFX = AEAudioLoadSound("Assets/Harvest.wav");
    g_farmHarvestGroup = AEAudioCreateGroup();

    FlyingFruit_Init();
}

void Farm_Initialize()
{
    farmPlots.clear();
    farmPlots.resize(4);

    // Load farm state from active profile
    for (int i = 0; i < 4; i++) {
        farmPlots[i].isUnlocked = Profile_GetPlotUnlocked(i);
        farmPlots[i].isPlanted = Profile_GetPlotPlanted(i);
        farmPlots[i].isReady = Profile_GetPlotReady(i);
        farmPlots[i].growTimer = Profile_GetPlotTimer(i);
        farmPlots[i].seedType = Profile_GetPlotSeedType(i);
        farmPlots[i].rhythmTriggered = false;
        farmPlots[i].waitingForRhythm = false;
        farmPlots[i].growthFrozen = false;  // Reset freeze state on init
        for (int m = 0; m < 4; m++) farmPlots[i].milestoneReached[m] = false;
    }

    // Migration guard: ensure plot 0 is always unlocked
    bool anyUnlocked = false;
    for (int i = 0; i < 4; i++)
        if (farmPlots[i].isUnlocked) { anyUnlocked = true; break; }

    if (!anyUnlocked)
    {
        farmPlots[0].isUnlocked = true;
        Profile_SetPlotUnlocked(0, true);
        std::cout << "Farm migration: plot 0 force-unlocked (no farm data in save)\n";
    }

    // Reset rhythm state on fresh load
    g_rhythmPaused = false;
    g_rhythmPausedPlotIndex = -1;
    g_rhythmPlotIndex = -1;
    g_requestRhythm = false;

    // Reset harvest popup state
    g_harvestPopupOpen = false;
    g_harvestPopupPlotIndex = -1;

    std::cout << "Farm initialized from profile\n";
}

// ------------------------------------------------------------
// UPDATE
// ------------------------------------------------------------

bool Farm_IsPlotLocked(int index)
{
    if (index < 0 || index >= (int)farmPlots.size()) return true;
    return !farmPlots[index].isUnlocked;
}

void Farm_Update()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();

    // Always update flying fruits
    FlyingFruit_Update(dt);

    // Mouse
    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    float worldX = (float)mouseX - 800.0f;
    float worldY = 450.0f - (float)mouseY;

    // Only restrict INPUT, not logic
    bool allowInput = UI_IsMenuOpen();

    // ---------------------------------------------------------------
    // HARVEST POPUP (FULL ORIGINAL LOGIC RESTORED)
    // ---------------------------------------------------------------
    if (g_harvestPopupOpen && g_harvestPopupPlotIndex >= 0)
    {
        if (allowInput && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            const float btnW = 130.0f, btnH = 45.0f;
            const float invBtnX = -80.0f, btnY = -40.0f;
            const float crateBtnX = 80.0f;

            bool clickedInventory =
                worldX >= invBtnX - btnW * 0.5f && worldX <= invBtnX + btnW * 0.5f &&
                worldY >= btnY - btnH * 0.5f && worldY <= btnY + btnH * 0.5f;

            bool clickedCrate =
                worldX >= crateBtnX - btnW * 0.5f && worldX <= crateBtnX + btnW * 0.5f &&
                worldY >= btnY - btnH * 0.5f && worldY <= btnY + btnH * 0.5f;

            if (clickedInventory || clickedCrate)
            {
                int idx = g_harvestPopupPlotIndex;
                FarmPlot& plot = farmPlots[idx];
                int harvestedType = plot.seedType;

                plot.isPlanted = false;
                plot.isReady = false;
                plot.seedType = -1;
                plot.growTimer = 0.0f;
                plot.rhythmTriggered = false;
                plot.waitingForRhythm = false;
                plot.growthFrozen = false;

                Profile_SetPlotData(idx, false, false, 0.0f, -1);

                if (harvestedType >= 0)
                {
                    float plotX = UI_GetPlotSlotX(idx);
                    float plotY = UI_GetPlotSlotY(idx);

                    if (clickedInventory)
                    {
                        Inventory_AddFruit(1, (u8)harvestedType);
                        FlyingFruit_Spawn(plotX, plotY, -600.0f, 100.0f, harvestedType);
                    }
                    else
                    {
                        Crate_AddFruitTyped(harvestedType, 1);
                        float crateX = UI_GetCrateSlotX(harvestedType);
                        float crateY = UI_GetCrateSlotY(harvestedType);
                        FlyingFruit_Spawn(plotX, plotY, crateX, crateY, harvestedType);
                    }
                }

                g_harvestPopupOpen = false;
                g_harvestPopupPlotIndex = -1;
                g_rhythmPlotIndex = -1;
                return;
            }
        }

        return;
    }

    // ---------------------------------------------------------------
    // HARVEST KEY
    // ---------------------------------------------------------------
    if (allowInput && AEInputCheckTriggered(AEVK_SPACE))
    {
        for (int i = 0; i < (int)farmPlots.size(); i++)
        {
            if (farmPlots[i].isReady)
            {
                g_harvestPopupPlotIndex = i;
                g_harvestPopupOpen = true;
                break;
            }
        }
    }

    // ---------------------------------------------------------------
    // MAIN FARM LOOP (FIXED)
    // ---------------------------------------------------------------
    for (int i = 0; i < (int)farmPlots.size(); i++)
    {
        FarmPlot& plot = farmPlots[i];

        if (!plot.isPlanted)
            continue;

        // Frozen
        if (plot.growthFrozen)
        {
            if (allowInput && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                float iconSize = 50.0f;
                float tickX = -70.0f, tickY = -40.0f;

                if (worldX >= tickX - iconSize / 2 && worldX <= tickX + iconSize / 2 &&
                    worldY >= tickY - iconSize / 2 && worldY <= tickY + iconSize / 2)
                {
                    plot.growthFrozen = false;
                    plot.rhythmTriggered = false;
                    plot.waitingForRhythm = true;
                    g_rhythmPlotIndex = i;
                    g_requestRhythm = true;
                    return;
                }
            }
            continue;
        }

        // Paused
        if (g_rhythmPaused && g_rhythmPausedPlotIndex == i)
        {
            if (allowInput && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                float iconSize = 50.0f;
                float tickX = -70.0f, tickY = -40.0f;
                float crossX = 70.0f, crossY = -40.0f;

                if (worldX >= tickX - iconSize / 2 && worldX <= tickX + iconSize / 2 &&
                    worldY >= tickY - iconSize / 2 && worldY <= tickY + iconSize / 2)
                {
                    g_rhythmPaused = false;
                    g_rhythmPausedPlotIndex = -1;
                    plot.waitingForRhythm = true;
                    g_rhythmPlotIndex = i;
                    g_requestRhythm = true;
                    return;
                }

                if (worldX >= crossX - iconSize / 2 && worldX <= crossX + iconSize / 2 &&
                    worldY >= crossY - iconSize / 2 && worldY <= crossY + iconSize / 2)
                {
                    g_rhythmPaused = false;
                    g_rhythmPausedPlotIndex = -1;
                    plot.growthFrozen = true;
                    plot.rhythmTriggered = true;
                    return;
                }
            }
            continue;
        }

        // Waiting
        if (plot.waitingForRhythm)
        {
            if (allowInput && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                float iconSize = 50.0f;
                float tickX = -70.0f, tickY = -40.0f;
                float crossX = 70.0f, crossY = -40.0f;

                if (worldX >= tickX - iconSize / 2 && worldX <= tickX + iconSize / 2 &&
                    worldY >= tickY - iconSize / 2 && worldY <= tickY + iconSize / 2)
                {
                    plot.waitingForRhythm = false;
                    g_requestRhythm = true;
                    g_rhythmPlotIndex = i;
                    return;
                }

                if (worldX >= crossX - iconSize / 2 && worldX <= crossX + iconSize / 2 &&
                    worldY >= crossY - iconSize / 2 && worldY <= crossY + iconSize / 2)
                {
                    plot.waitingForRhythm = false;
                    plot.rhythmTriggered = true;
                    return;
                }
            }
            continue;
        }

        if (g_requestRhythm)
            continue;

        //  GROWTH (FIXED)
        plot.growTimer += dt;

        float ratio = plot.growTimer / GROW_TIME;

        // Play tick sounds at growth milestones (25%, 50%, 75%)
        if (ratio >= 0.25f && !plot.milestoneReached[0])
        {
            plot.milestoneReached[0] = true;
            if (UIAudio_SFXEnabled() && AEAudioIsValidAudio(g_farmTickSFX) && AEAudioIsValidGroup(g_farmTickGroup))
            {
                AEAudioPlay(g_farmTickSFX, g_farmTickGroup, 1.0f, 1.0f, 0);
            }
        }

        if (ratio >= 0.5f && !plot.milestoneReached[1])
        {
            plot.milestoneReached[1] = true;
            if (UIAudio_SFXEnabled() && AEAudioIsValidAudio(g_farmTickSFX) && AEAudioIsValidGroup(g_farmTickGroup))
            {
                AEAudioPlay(g_farmTickSFX, g_farmTickGroup, 1.0f, 1.0f, 0);
            }
        }

        if (ratio >= 0.75f && !plot.milestoneReached[2])
        {
            plot.milestoneReached[2] = true;
            if (UIAudio_SFXEnabled() && AEAudioIsValidAudio(g_farmTickSFX) && AEAudioIsValidGroup(g_farmTickGroup))
            {
                AEAudioPlay(g_farmTickSFX, g_farmTickGroup, 1.0f, 1.0f, 0);
            }
        }

        if (ratio >= 0.5f && !plot.rhythmTriggered && g_rhythmPlotIndex == -1)
        {
            plot.rhythmTriggered = true;
            plot.waitingForRhythm = true;
            g_rhythmPlotIndex = i;
        }

        if (ratio >= 1.0f && !plot.isReady)
        {
            plot.isReady = true;
            plot.milestoneReached[3] = true;

            // Harvest sound — plot fully grown
            if (UIAudio_SFXEnabled() && AEAudioIsValidAudio(g_farmHarvestSFX) && AEAudioIsValidGroup(g_farmHarvestGroup))
            {
                AEAudioPlay(g_farmHarvestSFX, g_farmHarvestGroup, 3.0f, 1.0f, 0);
            }


            plot.waitingForRhythm = false;
            plot.growthFrozen = false;

            if (g_rhythmPlotIndex == i)
                g_rhythmPlotIndex = -1;

            if (g_rhythmPausedPlotIndex == i)
            {
                g_rhythmPaused = false;
                g_rhythmPausedPlotIndex = -1;
            }

            Profile_SetPlotData(i, plot.isPlanted, plot.isReady, plot.growTimer, plot.seedType);
        }
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

    for (int i = 0; i < (int)farmPlots.size(); i++)
    {
        float plotX = UI_GetPlotSlotX(i);
        float plotY = UI_GetPlotSlotY(i);

        FarmPlot& plot = farmPlots[i];

        // --------------------------
        // DRAW LOCKED PLOT (with dark tint)
        // --------------------------
        if (!plot.isUnlocked)
        {
            // Dark tint for locked plots
            AEGfxSetColorToMultiply(0.4f, 0.4f, 0.4f, 1.0f);
            AEGfxTextureSet(lockedPlot, 0, 0);
            AEMtx33Scale(&scale, 120.0f, 120.0f);
            AEMtx33Trans(&trans, plotX, plotY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            // Reset color for next draw
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            continue;
        }

        if (!plot.isPlanted)
            continue;

        float ratio = farmPlots[i].growTimer / GROW_TIME;
        if (ratio > 1.0f) ratio = 1.0f;

        // Draw base plant
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxTextureSet(plantedTexture, 0, 0);
        AEMtx33Scale(&scale, 120.0f, 120.0f);
        AEMtx33Trans(&trans, plotX, plotY);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        // Growth stage texture
        AEGfxTexture* stageTexture = nullptr;
        if (ratio < 0.25f)      stageTexture = fruitStage25;
        else if (ratio < 0.5f)  stageTexture = fruitStage50;
        else if (ratio < 0.75f) stageTexture = fruitStage75;
        else                    stageTexture = fruitStageFull;

        // Warm tint while waiting for rhythm (first-time prompt)
        if (farmPlots[i].waitingForRhythm)
            AEGfxSetColorToMultiply(1.0f, 0.9f, 0.6f, 1.0f);

        // Blue tint while paused
        if (g_rhythmPaused && g_rhythmPausedPlotIndex == i)
            AEGfxSetColorToMultiply(0.8f, 0.9f, 1.0f, 1.0f);

        // Gray tint while permanently frozen
        if (farmPlots[i].growthFrozen)
            AEGfxSetColorToMultiply(0.6f, 0.6f, 0.6f, 1.0f);

        float size = 120.0f;

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

        // Leaf
        if (ratio >= 0.9f && Leaf)
        {
            float leafSize = 25.0f;
            float leafX = plotX + sinf((float)clock() * 0.006f) * 15.0f;
            float leafY = plotY + cosf((float)clock() * 0.004f) * 10.0f;
            float leafPulse = sinf((float)clock() * 0.008f) * 2.0f;
            leafSize += leafPulse;

            AEGfxTextureSet(Leaf, 0, 0);
            AEMtx33Scale(&scale, leafSize, leafSize);
            AEMtx33Trans(&trans, leafX, leafY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
        }

        // Water droplet
        if (ratio < 0.75f && Droplet)
        {
            float alpha = 0.8f + sinf((float)clock() * 0.01f) * 0.2f;
            float bob = sinf((float)clock() * 0.005f) * 3.0f;
            float decorSize = 30.0f;
            float decorX = plotX + 40.0f;
            float decorY = plotY + 40.0f + bob;

            AEGfxSetTransparency(alpha);
            AEGfxTextureSet(Droplet, 0, 0);
            AEMtx33Scale(&scale, decorSize, decorSize);
            AEMtx33Trans(&trans, decorX, decorY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            AEGfxSetTransparency(1.0f);
        }

        // --------------------------
        // Draw Rhythm Prompt (for: waiting, paused, OR frozen states)
        // --------------------------
        bool showPrompt = !g_harvestPopupOpen &&
            ((farmPlots[i].waitingForRhythm && g_rhythmPlotIndex == i)
                || (g_rhythmPaused && g_rhythmPausedPlotIndex == i)
                || farmPlots[i].growthFrozen);  // NEW: also show when frozen

        if (showPrompt)
        {
            AEGfxSetColorToMultiply(1, 1, 1, 1);

            if (rhythmPrompt)
            {
                AEGfxTextureSet(rhythmPrompt, 0, 0);
                AEMtx33Scale(&scale, 350.0f, 200.0f);
                AEMtx33Trans(&trans, 0.0f, 0.0f);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }

            float iconSize = 50.0f;

            if (tickIcon)
            {
                AEGfxTextureSet(tickIcon, 0, 0);
                AEMtx33Scale(&scale, iconSize, iconSize);
                AEMtx33Trans(&trans, -70.0f, -40.0f);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }

            if (crossIcon)
            {
                AEGfxTextureSet(crossIcon, 0, 0);
                AEMtx33Scale(&scale, iconSize, iconSize);
                AEMtx33Trans(&trans, 70.0f, -40.0f);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }
        }

        // --------------------------
        // Draw Delete X
        // --------------------------
        if (deleteIcon)
        {
            float xSize = 40.0f;
            float xPos = plotX - 55.0f;
            float yPos = plotY + 55.0f;

            AEGfxTextureSet(deleteIcon, 0, 0);
            AEMtx33Scale(&scale, xSize, xSize);
            AEMtx33Trans(&trans, xPos, yPos);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
        }
    } // end for loop over farmPlots

    // --------------------------
    // Draw flying harvest fruits (arc animation toward crate)
    // --------------------------
    if (fruitAppleTexture)
    {
        for (int j = 0; j < MAX_FLYING; j++)
        {
            const FlyingFruit& ff = g_flyingFruits[j];
            if (!ff.active) continue;

            // Ease in-out: smooth t
            float t = ff.t;
            float easedT = t * t * (3.0f - 2.0f * t);

            // Arc control point: midpoint raised by 150px
            float midX = (ff.startX + ff.endX) * 0.5f;
            float midY = (ff.startY + ff.endY) * 0.5f + 150.0f;

            float fx = BezierX(easedT, ff.startX, midX, ff.endX);
            float fy = BezierY(easedT, ff.startY, midY, ff.endY);

            // Scale: shrink as it reaches the crate
            float fruitSize = 40.0f * (1.0f - easedT * 0.6f);
            float alpha = 1.0f - easedT * 0.3f; // slight fade

            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxSetTransparency(alpha);
            AEGfxTextureSet(fruitAppleTexture, 0, 0);
            AEMtx33Scale(&scale, fruitSize, fruitSize);
            AEMtx33Trans(&trans, fx, fy);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
        }
        AEGfxSetTransparency(1.0f);
    }

    // --------------------------
    // Draw harvest destination popup (drawn on top of everything else)
    // Shows when player presses SPACE on a ready plot.
    // Panel uses the same rhythmPrompt background texture for visual consistency.
    // --------------------------
    if (g_harvestPopupOpen && g_harvestPopupPlotIndex >= 0)
    {
        extern s8 fontId;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        // Panel background - solid color so the rhythmPrompt texture's
        // baked-in "ARE YOU READY TO PLAY" text doesn't bleed through.
        // Warm parchment: RGB (0.96, 0.89, 0.72)
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetColorToMultiply(0.96f, 0.89f, 0.72f, 1.0f);
        AEMtx33Scale(&scale, 350.0f, 160.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        // Border outline (darker brown)
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetColorToMultiply(0.55f, 0.35f, 0.15f, 1.0f);
        AEMtx33Scale(&scale, 358.0f, 168.0f);  // slightly larger = border
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        // Draw border behind panel by re-drawing panel on top (painter's algorithm)
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetColorToMultiply(0.96f, 0.89f, 0.72f, 1.0f);
        AEMtx33Scale(&scale, 350.0f, 160.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        // "Where to?" title text
        if (fontId >= 0)
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxPrint(fontId, "Send fruit to:", -0.21f, 0.06f, 1.5f, 0.15f, 0.08f, 0.02f, 1.0f);
        }

        // [Inventory] button  left,  world-space center (-80, -40)
        {
            const float btnW = 130.0f, btnH = 45.0f;
            const float btnX = -80.0f, btnY = -40.0f;

            // Check hover for highlight tint
            s32 mx, my;
            AEInputGetCursorPosition(&mx, &my);
            float wx = (float)mx - 800.0f;
            float wy = 450.0f - (float)my;
            bool hovered = wx >= btnX - btnW * 0.5f && wx <= btnX + btnW * 0.5f &&
                wy >= btnY - btnH * 0.5f && wy <= btnY + btnH * 0.5f;

            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            // Warm beige normally, slightly brighter on hover
            float r = hovered ? 0.95f : 0.82f;
            float g2 = hovered ? 0.85f : 0.72f;
            float b = hovered ? 0.65f : 0.55f;
            AEGfxSetColorToMultiply(r, g2, b, 1.0f);
            AEMtx33Scale(&scale, btnW, btnH);
            AEMtx33Trans(&trans, btnX, btnY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            if (fontId >= 0)
            {
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1, 1, 1, 1);
                AEGfxPrint(fontId, "Inventory", -0.17f, -0.11f, 1.0f, 0.15f, 0.08f, 0.02f, 1.0f);
            }
        }

        // [Crate] button  right, world-space center (80, -40)
        {
            const float btnW = 130.0f, btnH = 45.0f;
            const float btnX = 80.0f, btnY = -40.0f;

            s32 mx, my;
            AEInputGetCursorPosition(&mx, &my);
            float wx = (float)mx - 800.0f;
            float wy = 450.0f - (float)my;
            bool hovered = wx >= btnX - btnW * 0.5f && wx <= btnX + btnW * 0.5f &&
                wy >= btnY - btnH * 0.5f && wy <= btnY + btnH * 0.5f;

            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            float r = hovered ? 0.95f : 0.82f;
            float g2 = hovered ? 0.85f : 0.72f;
            float b = hovered ? 0.65f : 0.55f;
            AEGfxSetColorToMultiply(r, g2, b, 1.0f);
            AEMtx33Scale(&scale, btnW, btnH);
            AEMtx33Trans(&trans, btnX, btnY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            if (fontId >= 0)
            {
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1, 1, 1, 1);
                AEGfxPrint(fontId, "Crate", 0.05f, -0.11f, 1.0f, 0.15f, 0.08f, 0.02f, 1.0f);
            }
        }
    }
} // end Farm_Render

// ------------------------------------------------------------

void Farm_Free()
{
    std::cout << "Farm_Free\n";
}

void Farm_Unload()
{
    if (plantedTexture) { AEGfxTextureUnload(plantedTexture);     plantedTexture = nullptr; }
    if (deleteIcon) { AEGfxTextureUnload(deleteIcon);         deleteIcon = nullptr; }
    if (fruitAppleTexture) { AEGfxTextureUnload(fruitAppleTexture);  fruitAppleTexture = nullptr; }

    // Unload farm audio
    if (AEAudioIsValidAudio(g_farmTickSFX))      AEAudioUnloadAudio(g_farmTickSFX);
    if (AEAudioIsValidGroup(g_farmTickGroup))     AEAudioUnloadAudioGroup(g_farmTickGroup);
    if (AEAudioIsValidAudio(g_farmHarvestSFX))   AEAudioUnloadAudio(g_farmHarvestSFX);
    if (AEAudioIsValidGroup(g_farmHarvestGroup))  AEAudioUnloadAudioGroup(g_farmHarvestGroup);
    memset(&g_farmTickSFX, 0, sizeof(AEAudio));
    memset(&g_farmTickGroup, 0, sizeof(AEAudioGroup));
    memset(&g_farmHarvestSFX, 0, sizeof(AEAudio));
    memset(&g_farmHarvestGroup, 0, sizeof(AEAudioGroup));

    std::cout << "Farm_Unload\n";
}

// ------------------------------------------------------------
// PUBLIC FUNCTIONS
// ------------------------------------------------------------

void Farm_PlantSeed(int plotIndex, int seedType)
{

    g_rhythmUsed = false;

    if (plotIndex < 0 || plotIndex >= (int)farmPlots.size())
        return;

    FarmPlot& plot = farmPlots[plotIndex];

    if (plot.isPlanted)
        return;

    Inventory_RemoveSeed(1, static_cast<u8>(seedType));

    plot.isPlanted = true;
    plot.isReady = false;
    plot.seedType = seedType;
    plot.growTimer = 0.0f;
    plot.rhythmTriggered = false;
    plot.waitingForRhythm = false;
    plot.growthFrozen = false;  // Reset freeze state on new plant
    for (int m = 0; m < 4; m++) plot.milestoneReached[m] = false;

    // Clear any leftover rhythm state for this plot
    if (g_rhythmPlotIndex == plotIndex)
        g_rhythmPlotIndex = -1;

    if (g_rhythmPausedPlotIndex == plotIndex)
    {
        g_rhythmPaused = false;
        g_rhythmPausedPlotIndex = -1;
    }

    Profile_SetPlotData(plotIndex, plot.isPlanted, plot.isReady, plot.growTimer, plot.seedType);
    std::cout << "Planted seed " << seedType << " in plot " << plotIndex << "\n";
}

bool Farm_IsPlotPlanted(int plotIndex)
{
    if (plotIndex < 0 || plotIndex >= (int)farmPlots.size())
        return false;
    return farmPlots[plotIndex].isPlanted;
}

void Farm_ClearPlot(int index)
{
    if (index < 0 || index >= (int)farmPlots.size())
        return;

    farmPlots[index].isPlanted = false;
    farmPlots[index].isReady = false;
    farmPlots[index].growTimer = 0.0f;
    farmPlots[index].seedType = -1;
    farmPlots[index].rhythmTriggered = false;
    farmPlots[index].waitingForRhythm = false;
    farmPlots[index].growthFrozen = false;  // Reset freeze state on clear
    for (int m = 0; m < 4; m++) farmPlots[index].milestoneReached[m] = false;

    if (g_rhythmPlotIndex == index)
        g_rhythmPlotIndex = -1;

    if (g_rhythmPausedPlotIndex == index)
    {
        g_rhythmPaused = false;
        g_rhythmPausedPlotIndex = -1;
    }

    Profile_SetPlotData(index, false, false, 0.0f, -1);
    std::cout << "Cleared plot " << index << "\n";
}

void Farm_OnRhythmResult(bool success)
{
    for (int i = 0; i < (int)farmPlots.size(); i++)
    {
        auto& plot = farmPlots[i];
        if (plot.waitingForRhythm || plot.growthFrozen)  // Check both states
        {
            plot.waitingForRhythm = false;

            if (success)
            {
                plot.growthFrozen = false;  // UNFREEZE on success
                plot.growTimer = GROW_TIME * 0.5f + 0.01f;
                plot.rhythmTriggered = true;  // Mark as triggered so we don't prompt again
                std::cout << "Rhythm success! Plot " << i << " growth boosted and unfrozen\n";
            }
            else
            {
                plot.growTimer -= 0.5f;
                if (plot.growTimer < 0.0f)
                    plot.growTimer = 0.0f;
                std::cout << "Rhythm failed! Plot " << i << " growth reduced\n";
            }

            Profile_SetPlotData(i, plot.isPlanted, plot.isReady, plot.growTimer, plot.seedType);
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

// Clears only the launch flag, preserving g_rhythmPlotIndex for ESC recovery
void Farm_ClearRhythmFlag()
{
    g_requestRhythm = false;
    // deliberately NOT clearing g_rhythmPlotIndex here
}

// ------------------------------------------------------------
// RHYTHM PAUSE
// ------------------------------------------------------------

void Farm_SetRhythmPaused(bool paused)
{
    g_rhythmPaused = paused;

    if (paused)
    {
        g_rhythmPausedPlotIndex = g_rhythmPlotIndex;

        if (g_rhythmPausedPlotIndex >= 0 &&
            g_rhythmPausedPlotIndex < (int)farmPlots.size())
        {
            farmPlots[g_rhythmPausedPlotIndex].waitingForRhythm = false;
            farmPlots[g_rhythmPausedPlotIndex].rhythmTriggered = true;
        }
    }
    else
    {
        g_rhythmPausedPlotIndex = -1;
    }
}

bool Farm_IsRhythmPaused()
{
    return g_rhythmPaused;
}

bool Farm_IsWaitingForRhythm()
{
    for (const auto& plot : farmPlots)
        if (plot.waitingForRhythm) return true;
    return false;
}

int Farm_GetRhythmSeedType()
{
    int idx = g_rhythmPlotIndex;
    if (idx < 0 || idx >= (int)farmPlots.size()) return 0;
    return farmPlots[idx].seedType;
}

float Farm_GetGrowTime()
{
    return GROW_TIME;
}

// Setter: unlock or lock a farm plot and persist to profile.
void Farm_SetPlotUnlocked(int index, bool unlocked)
{
    if (index < 0 || index >= (int)farmPlots.size())
        return;

    farmPlots[index].isUnlocked = unlocked;
    Profile_SetPlotUnlocked(index, unlocked); // persist immediately

    std::cout << "Farm: plot " << index << " unlocked=" << (unlocked ? "true" : "false") << "\n";

    // If locking a plot, clear any active state to avoid dangling data.
    if (!unlocked)
    {
        farmPlots[index].isPlanted = false;
        farmPlots[index].isReady = false;
        farmPlots[index].growTimer = 0.0f;
        farmPlots[index].seedType = -1;
        farmPlots[index].rhythmTriggered = false;
        farmPlots[index].waitingForRhythm = false;
        farmPlots[index].growthFrozen = false;
        for (int m = 0; m < 4; ++m) farmPlots[index].milestoneReached[m] = false;

        Profile_SetPlotData(index, false, false, 0.0f, -1);
    }
}