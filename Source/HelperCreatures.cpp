#include "HelperCreatures.h"
#include "Farm.h"
#include "UI.h"
#include "Inventory.h"
#include "Crate.h"
#include <cmath>
#include <vector>
#include "SpawnFruits.h"
#include "Main.h"

extern std::vector<Fruit> fruits;
extern AEGfxVertexList* g_pMeshFullScreen;

// ------------------------------------------------------------
// GLOBAL STATE
// ------------------------------------------------------------

static float facing = 1.0f;
static int targetFruit = -1;
static float targetX = 0.0f;

static bool isPicking = false;
static float hopTimer = 0.0f;

// ------------------------------------------------------------
// PATROL STATE (idle walking across the stall)
// ------------------------------------------------------------
static const float PATROL_LEFT = 50.0f;
static const float PATROL_RIGHT = 450.0f;
static float patrolDir = 1.0f;   // 1 = right, -1 = left
static const float PATROL_SPEED = 60.0f;

// ------------------------------------------------------------
// HELPER STRUCT
// ------------------------------------------------------------

struct HelperCreature
{
    float x, y;
    float speed;
};

static HelperCreature g_bunny;

// textures
static AEGfxTexture* bunnyWalk1 = nullptr;
static AEGfxTexture* bunnyWalk2 = nullptr;
static AEGfxTexture* bunnyIdle = nullptr;

static float animTimer = 0.0f;
static bool altFrame = false;

// ------------------------------------------------------------
// INIT
// ------------------------------------------------------------

void Helper_Init()
{
    g_bunny.x = 250.0f;
    g_bunny.y = -300.0f;
    g_bunny.speed = 100.0f;

    bunnyWalk1 = AEGfxTextureLoad("Assets/bunny1_walk1.png");
    bunnyWalk2 = AEGfxTextureLoad("Assets/bunny1_walk2.png");
    bunnyIdle = AEGfxTextureLoad("Assets/bunny1_stand.png");
}

// ------------------------------------------------------------
// FIND FRUIT
// ------------------------------------------------------------

static int Helper_FindFruit()
{
    int best = -1;
    float bestDist = 999999.0f;

    for (int i = 0; i < fruits.size(); i++)
    {
        if (fruits[i].isCollected)
            continue;

        // ignore fruits still high in air
        if (fruits[i].y > -200.0f)
            continue;

        float dx = fruits[i].x - g_bunny.x;
        float dist = fabs(dx);

        if (dist < bestDist)
        {
            bestDist = dist;
            best = i;
        }
    }

    return best;
}

// ------------------------------------------------------------
// UPDATE
// ------------------------------------------------------------

void Helper_Update(float dt)
{
    // reset if fruit gone
    if (targetFruit != -1 && fruits[targetFruit].isCollected)
        targetFruit = -1;

    // assign new target
    if (targetFruit == -1)
    {
        targetFruit = Helper_FindFruit();

        if (targetFruit != -1)
            targetX = fruits[targetFruit].x; // lock position
    }



    // --------------------------------------------------------
    // PATROL when no fruit to collect
    // --------------------------------------------------------
    if (targetFruit == -1)
    {
        // move in current patrol direction
        g_bunny.x += patrolDir * PATROL_SPEED * dt;

        // flip at boundaries
        if (g_bunny.x >= PATROL_RIGHT)
        {
            g_bunny.x = PATROL_RIGHT;
            patrolDir = -1.0f;
        }
        else if (g_bunny.x <= PATROL_LEFT)
        {
            g_bunny.x = PATROL_LEFT;
            patrolDir = 1.0f;
        }

        facing = patrolDir;

        // hop while patrolling
        hopTimer += dt * 1.5f;
        float hop = sinf(hopTimer * 8.0f);
        hop = hop * hop;
        g_bunny.y = -300.0f + hop * 6.0f;

        // walk animation
        animTimer += dt;
        if (animTimer > 0.3f)
        {
            altFrame = !altFrame;
            animTimer = 0.0f;
        }

        return;
    }

    float dx = targetX - g_bunny.x;
    float dist = fabs(dx);

    if (dist < 1.5f)
    {
        dx = 0.0f;
        dist = 0.0f;
    }

    // facing
    if (fabs(dx) > 0.01f)
        facing = (dx > 0) ? 1.0f : -1.0f;

    // --------------------------------------------------------
    // SMOOTH MOVEMENT (no jitter)
    // --------------------------------------------------------
    if (!isPicking)
    {
        if (dist > 2.0f)
        {
            float followSpeed = 6.0f;

            // smooth slowdown near target
            float speedMultiplier = dist / 50.0f;
            if (speedMultiplier > 1.0f) speedMultiplier = 1.0f;

            g_bunny.x += dx * followSpeed * speedMultiplier * dt;
        }
        else
        {
            if (!isPicking)
            {
                isPicking = true;

                // collect immediately — no delay
                fruits[targetFruit].isCollected = true;
                fruits[targetFruit].y += 10.0f;
                Inventory_AddFruit(1, 0);
                MainScreen_OnHelperCollect(1);

                isPicking = false;
                targetFruit = -1;
            }
        }
    }

    // animation (for sprite switching only)
    animTimer += dt;
    if (animTimer > 0.3f)
    {
        altFrame = !altFrame;
        animTimer = 0.0f;
    }

    // separate smooth hop timer
    hopTimer += dt * 1.5f;

    if (targetFruit != -1 && targetFruit < fruits.size())
    {
        float targetY = fruits[targetFruit].y;

        float hop = 0.0f;

        // ONLY hop when moving
        if (!isPicking && dist > 2.0f)
        {
            hop = sinf(hopTimer * 8.0f);
            hop = hop * hop;
        }
        else
        {
            hop = 0.0f; // stop bouncing cleanly
        }

        g_bunny.y = targetY + hop * 6.0f;
    }
    else
    {
        g_bunny.y = -300.0f;
    }
}

// ------------------------------------------------------------
// DRAW
// ------------------------------------------------------------

void Helper_Draw()
{
    // walk animation plays at all times (idle wander or chasing fruit)
    AEGfxTexture* tex = altFrame ? bunnyWalk1 : bunnyWalk2;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    AEMtx33 scale, trans, final;

    float scaleY = 80.0f;

    // crouch when picking
    if (isPicking)
        scaleY = 65.0f;

    AEMtx33Scale(&scale, 80 * facing, scaleY);
    AEMtx33Trans(&trans, g_bunny.x, g_bunny.y);
    AEMtx33Concat(&final, &trans, &scale);

    AEGfxSetTransform(final.m);
    AEGfxTextureSet(tex, 0, 0);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
}