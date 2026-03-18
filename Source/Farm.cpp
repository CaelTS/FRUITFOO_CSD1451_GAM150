#include "Farm.h"
#include "AEEngine.h"
#include "UI.h"
#include "Profile.h"
#include <iostream>
#include <vector>
#include <cmath>
#include "Inventory.h"
#include "Crate.h"

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
    bool growthFrozen = false;  // NEW: permanently freeze growth when X is clicked
};

static bool g_rhythmUsed = false;
static bool g_requestRhythm = false;
static int  g_rhythmPlotIndex = -1;
static bool g_rhythmPaused = false;
static int  g_rhythmPausedPlotIndex = -1;


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
    if (!UI_IsMenuOpen())
        return;

    float dt = (float)AEFrameRateControllerGetFrameTime();

    FlyingFruit_Update(dt);

    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    float worldX = (float)mouseX - 800.0f;
    float worldY = 450.0f - (float)mouseY;

    // Harvest
    if (AEInputCheckTriggered(AEVK_SPACE))
    {
        for (int i = 0; i < (int)farmPlots.size(); i++)
        {
            auto& plot = farmPlots[i];
            if (plot.isReady)
            {
                int harvestedType = plot.seedType; // 0=apple, 1=pear, 2=banana

                plot.isPlanted = false;
                plot.isReady = false;
                plot.seedType = -1;
                plot.growTimer = 0.0f;
                plot.rhythmTriggered = false;
                plot.waitingForRhythm = false;
                plot.growthFrozen = false;  // Reset freeze on harvest

                Profile_SetPlotData(i, false, false, 0.0f, -1);
                std::cout << "Harvested plot " << i << "\n";

                // -- Add to inventory and crate --
                if (harvestedType >= 0)
                {
                    Inventory_AddFruit(1, static_cast<u8>(harvestedType));
                    Crate_AddFruitTyped(harvestedType, 1);
                    std::cout << "Added fruit type " << harvestedType
                        << " to inventory and crate\n";

                    // Spawn flying fruit animation from plot to its matching crate slot
                    float plotX = UI_GetPlotSlotX(i);
                    float plotY = UI_GetPlotSlotY(i);
                    float crateX = UI_GetCrateSlotX(harvestedType);
                    float crateY = UI_GetCrateSlotY(harvestedType);
                    FlyingFruit_Spawn(plotX, plotY, crateX, crateY, harvestedType);
                }
            }
        }
    }

    for (int i = 0; i < (int)farmPlots.size(); i++)
    {
        FarmPlot& plot = farmPlots[i];

        if (!plot.isPlanted)
            continue;

        // ----------------------------------------------------------------
        // FROZEN STATE: growth permanently stopped by clicking X on rhythm prompt
        // But we still show the rhythm prompt and allow re-launching
        // ----------------------------------------------------------------
        if (plot.growthFrozen)
        {
            // Check for click on tick to re-launch rhythm game
            if (AEInputCheckTriggered(AEVK_LBUTTON))
            {
                float iconSize = 50.0f;
                float tickX = -70.0f, tickY = -40.0f;

                // Tick => re-launch rhythm game (unfreeze and play)
                if (worldX >= tickX - iconSize / 2 && worldX <= tickX + iconSize / 2 &&
                    worldY >= tickY - iconSize / 2 && worldY <= tickY + iconSize / 2)
                {
                    plot.growthFrozen = false;  // Unfreeze
                    plot.rhythmTriggered = false;
                    plot.waitingForRhythm = true;
                    g_rhythmPlotIndex = i;
                    g_requestRhythm = true;

                    std::cout << "Frozen plot " << i << ": re-launching rhythm\n";
                    return;
                }
            }

            // Growth is frozen but prompt stays visible - skip growth logic
            continue;
        }

        // ----------------------------------------------------------------
        // PAUSED STATE: player ESC'd from rhythm game OR clicked X.
        // Growth is frozen until they click tick (re-launch) or cross (skip/freeze).
        // ----------------------------------------------------------------
        if (g_rhythmPaused && g_rhythmPausedPlotIndex == i)
        {
            if (AEInputCheckTriggered(AEVK_LBUTTON))
            {
                float iconSize = 50.0f;
                float tickX = -70.0f, tickY = -40.0f;
                float crossX = 70.0f, crossY = -40.0f;

                // Tick => re-launch rhythm game
                if (worldX >= tickX - iconSize / 2 && worldX <= tickX + iconSize / 2 &&
                    worldY >= tickY - iconSize / 2 && worldY <= tickY + iconSize / 2)
                {
                    g_rhythmPaused = false;
                    g_rhythmPausedPlotIndex = -1;
                    plot.rhythmTriggered = false;
                    plot.waitingForRhythm = true;
                    g_rhythmPlotIndex = i;
                    g_requestRhythm = true;

                    std::cout << "Paused Tick: re-launching rhythm\n";
                    return;
                }

                // Cross => PERMANENTLY freeze growth at current level
                if (worldX >= crossX - iconSize / 2 && worldX <= crossX + iconSize / 2 &&
                    worldY >= crossY - iconSize / 2 && worldY <= crossY + iconSize / 2)
                {
                    g_rhythmPaused = false;
                    g_rhythmPausedPlotIndex = -1;

                    // Mark as frozen - growth stops permanently
                    plot.growthFrozen = true;
                    plot.rhythmTriggered = true;
                    plot.waitingForRhythm = false;

                    std::cout << "Paused Cross: growth FROZEN permanently at "
                        << (plot.growTimer / GROW_TIME * 100.0f) << "%\n";
                    return;
                }
            }

            continue;   // growth frozen while prompt is visible
        }

        // ----------------------------------------------------------------
        // FIRST-TIME RHYTHM PROMPT: reached 50%, asking player to play.
        // ----------------------------------------------------------------
        if (plot.waitingForRhythm)
        {
            if (AEInputCheckTriggered(AEVK_LBUTTON))
            {
                float iconSize = 50.0f;
                float tickX = -70.0f, tickY = -40.0f;
                float crossX = 70.0f, crossY = -40.0f;

                // Tick => launch rhythm game
                if (worldX >= tickX - iconSize / 2 && worldX <= tickX + iconSize / 2 &&
                    worldY >= tickY - iconSize / 2 && worldY <= tickY + iconSize / 2)
                {
                    plot.waitingForRhythm = false;
                    g_requestRhythm = true;
                    g_rhythmPlotIndex = i;

                    std::cout << "Tick clicked\n";
                    return;
                }

                // Cross => pause growth, enter paused state
                if (worldX >= crossX - iconSize / 2 && worldX <= crossX + iconSize / 2 &&
                    worldY >= crossY - iconSize / 2 && worldY <= crossY + iconSize / 2)
                {
                    plot.waitingForRhythm = false;
                    plot.rhythmTriggered = true;
                    g_rhythmPaused = true;
                    g_rhythmPausedPlotIndex = i;
                    g_rhythmPlotIndex = -1;


                    std::cout << "Cross clicked: entering paused state\n";
                    return;
                }
            }

            continue;
        }

        // Pause growth if rhythm transition is pending
        if (g_requestRhythm)
            continue;

        // ----------------------------------------------------------------
        // NORMAL GROWTH (only if not frozen)
        // ----------------------------------------------------------------
        plot.growTimer += dt;

        float ratio = plot.growTimer / GROW_TIME;

        if (ratio >= 0.5f && !plot.rhythmTriggered && g_rhythmPlotIndex == -1)
        {
            plot.rhythmTriggered = true;
            plot.waitingForRhythm = true;
            g_rhythmPlotIndex = i;
        }

        if (ratio >= 1.0f && !plot.isReady)
        {
            plot.isReady = true;
            Profile_SetPlotData(i, plot.isPlanted, plot.isReady, plot.growTimer, plot.seedType);
            std::cout << "Plot " << i << " is ready for harvest!\n";
        }

        // Periodically save growth timer (~every 1 second)
        static float lastSaveTimer[4] = { -1.0f, -1.0f, -1.0f, -1.0f };
        if (plot.growTimer - lastSaveTimer[i] >= 1.0f) {
            Profile_SetPlotData(i, plot.isPlanted, plot.isReady, plot.growTimer, plot.seedType);
            lastSaveTimer[i] = plot.growTimer;
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
        bool showPrompt = (farmPlots[i].waitingForRhythm && g_rhythmPlotIndex == i)
            || (g_rhythmPaused && g_rhythmPausedPlotIndex == i)
            || farmPlots[i].growthFrozen;  // NEW: also show when frozen

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