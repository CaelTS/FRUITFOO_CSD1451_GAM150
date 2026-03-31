#include "Crate.h"
#include "Profile.h"
#include <iostream>
#include "Main.h"
#include "AEEngine.h"
#include "StartScreen.h"
#include "SpawnFruits.h"

// ---------------------------------------------------------------------------
// Constants (kept here to avoid redefinition with Profile.cpp's MAX_CRATES)
// ---------------------------------------------------------------------------
static constexpr int CRATE_COUNT = 3;   // must match MAX_CRATES in Profile.cpp

// Make max stock configurable at runtime instead of compile-time constexpr.
static int g_maxCrateStock = 9;  // default max fruit per crate

// ---------------------------------------------------------------------------
// Textures (can be loaded in Crate_Initialize and freed in Crate_Free)
// ---------------------------------------------------------------------------
static AEGfxTexture* gApple_1_FirstTexture = nullptr;
static AEGfxTexture* gApple_2_FirstTexture = nullptr;
static AEGfxTexture* gApple_3_FirstTexture = nullptr;
static AEGfxTexture* gCrate_0Texture = nullptr;

// Helper mesh for drawing crate icons
static AEGfxVertexList* pApple_0Mesh = nullptr;
static AEGfxVertexList* pCrateMesh_0 = nullptr;

static AEGfxVertexList* CreateMesh()
{
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);

    return AEGfxMeshEnd();
}


// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------

struct CrateData
{
    bool isUnlocked = false;

    FruitType fruitType;     // which fruit this crate holds (0=apple, 1=pear, 2=banana) 

    int  fruitCount = 0;       // current stock

    AEGfxVertexList* CrateMesh = NULL;
    AEGfxVertexList* AppleMesh = NULL;

    AEGfxTexture* crate_texture = nullptr;
    f32 crate_x = 0.0, crate_y = 0.0;

    AEGfxTexture* apple_1_texture = nullptr;;
    AEGfxTexture* apple_2_texture = nullptr;;
    AEGfxTexture* apple_3_texture = nullptr;;

    f32 apple_x_1 = 0.0, apple_y_1 = 0.0;
    f32 apple_x_2 = 0.0, apple_y_2 = 0.0;
    f32 apple_x_3 = 0.0, apple_y_3 = 0.0;
};

static CrateData g_crates[CRATE_COUNT];

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void Crate_Load()
{
    // Nothing to load from disk here — Profile handles persistence.
    std::cout << "Crate_Load\n";
}

void Crate_Initialize()
{
    // Load state from the active profile.
    for (int i = 0; i < CRATE_COUNT; i++)
    {
        g_crates[i].isUnlocked = Profile_GetCrateUnlocked(i);
        g_crates[i].fruitCount = Profile_GetCrateFruitCount(i);
    }

    // Migration guard: if every crate comes back locked (old save with no
    // [crate] section), force crate 0 unlocked and persist it.
    bool anyUnlocked = false;
    for (int i = 0; i < CRATE_COUNT; i++)
        if (g_crates[i].isUnlocked) { anyUnlocked = true; break; }

    if (!anyUnlocked)
    {
        g_crates[0].isUnlocked = true;
        Profile_SetCrateUnlocked(0, true);
        std::cout << "Crate migration: crate 0 force-unlocked (no crate data in save)\n";
    }

    // Ensure existing stocks don't exceed current max (safe for profile upgrades)
    for (int i = 0; i < CRATE_COUNT; ++i)
    {
        if (g_crates[i].fruitCount > g_maxCrateStock)
        {
            g_crates[i].fruitCount = g_maxCrateStock;
            Profile_SetCrateFruitCount(i, g_crates[i].fruitCount);
        }
    }

    std::cout << "Crate_Initialize: loaded from profile\n";
    for (int i = 0; i < CRATE_COUNT; i++)
        std::cout << "  Crate[" << i << "]: unlocked=" << g_crates[i].isUnlocked
        << " stock=" << g_crates[i].fruitCount << "\n";

    //--------------------------------------------------------------------------------------
    // Create meshes for drawing 

    // First Crate
    CrateData& crate0 = g_crates[0];

    crate0.CrateMesh = CreateMesh();
    crate0.AppleMesh = CreateMesh();

    //Textures First Crate
    crate0.apple_1_texture = AEGfxTextureLoad("Assets/Apple_1_First.png");
    crate0.apple_2_texture = AEGfxTextureLoad("Assets/Apple_2_First.png");
    crate0.apple_3_texture = AEGfxTextureLoad("Assets/Apple_3_First.png");
    if (!crate0.apple_1_texture) OutputDebugStringA("ERROR: Failed to load 'Assets/Apple_1_First.png'.\n");
    crate0.crate_texture = AEGfxTextureLoad("Assets/Crate_First.png");

    crate0.crate_x = 143.0f;
    crate0.crate_y = -148.0f;

    crate0.apple_x_1 = crate0.crate_x;
    crate0.apple_y_1 = crate0.crate_y - 20;

    crate0.apple_x_2 = crate0.crate_x + 2;
    crate0.apple_y_2 = crate0.crate_y - 9.0f;

    crate0.apple_x_3 = crate0.crate_x + 5.5;
    crate0.apple_y_3 = crate0.crate_y - 1;

    printf("Crate stock: %d\n", crate0.fruitCount);

}

void Crate_Update(float dt)
{
    // Nothing to update for now, but we could add animations or effects here later.

    // Update How many apples are in the crate for testing

}

void Crate_Draw()
{
    CrateData& crate0 = g_crates[0];

    if (g_crates[0].isUnlocked) {
        // Render Crate
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);  // Use texture rendering mode
        AEGfxTextureSet(crate0.crate_texture, 0, 0);  // Set the texture

        AEMtx33 scale, trans, rotation, transform, rotscale;

        AEMtx33Scale(&scale, 186 * gScaleX, 99 * gScaleY);

        AEMtx33Rot(&rotation, 0);  // Create rotation matrix 

        AEMtx33Trans(&trans, crate0.crate_x, crate0.crate_y);  // Apply position transformation

        AEMtx33Concat(&rotscale, &rotation, &scale);
        AEMtx33Concat(&transform, &trans, &rotscale);

        AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple

        AEGfxMeshDraw(crate0.CrateMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad

        // Render Apples based on stock count
		float stockPercent = crate0.fruitCount / float(g_maxCrateStock); // <<--- make global later when we have more crates


        /* if (AEInputCheckReleased(AEVK_LBUTTON)) {
             hello = !hello;
         }*/

        if (stockPercent > 0.0f) {
            // If stock is 30% or less
            if (stockPercent <= 0.3f) {
                AEGfxTextureSet(crate0.apple_1_texture, 0, 0);  // Set the texture
                AEMtx33Scale(&scale, 145 * gScaleX, 48 * gScaleY);
                AEMtx33Trans(&trans, crate0.apple_x_1, crate0.apple_y_1);  // Apply position transformation
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(crate0.AppleMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
            }

            // If stock is between 30% and 70%
            if (stockPercent > 0.3f && stockPercent <= 0.7f) {
                AEGfxTextureSet(crate0.apple_2_texture, 0, 0);  // Set the texture
                AEMtx33Scale(&scale, 150 * gScaleX, 74 * gScaleY);
                AEMtx33Trans(&trans, crate0.apple_x_2, crate0.apple_y_2);  // Apply position transformation
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(crate0.AppleMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
            }

            // If stock is above 70%
            if (stockPercent > 0.7f) {
                AEGfxTextureSet(crate0.apple_3_texture, 0, 0);  // Set the texture
                AEMtx33Scale(&scale, 158 * gScaleX, 94 * gScaleY);
                AEMtx33Trans(&trans, crate0.apple_x_3, crate0.apple_y_3);  // Apply position transformation
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(crate0.AppleMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
            }




        }

    }
}


void Crate_Free()
{
    std::cout << "Crate_Free\n";
}

// ---------------------------------------------------------------------------
// Unlock state
// ---------------------------------------------------------------------------

bool Crate_IsUnlocked(int crateIndex)
{
    if (crateIndex < 0 || crateIndex >= CRATE_COUNT) return false;
    return g_crates[crateIndex].isUnlocked;
}

void Crate_SetUnlocked(int crateIndex, bool unlocked)
{
    if (crateIndex < 0 || crateIndex >= CRATE_COUNT) return;
    g_crates[crateIndex].isUnlocked = unlocked;
    Profile_SetCrateUnlocked(crateIndex, unlocked); // persists immediately
    std::cout << "Crate[" << crateIndex << "] unlocked=" << unlocked << "\n";
}

// ---------------------------------------------------------------------------
// Max stock accessor / mutator
// ---------------------------------------------------------------------------

int Crate_GetMaxStock()
{
    return g_maxCrateStock;
}

void Crate_SetMaxStock(int maxStock)
{
    if (maxStock < 0) return; // ignore invalid
    g_maxCrateStock = maxStock;

    // Clamp existing crate stock to new max and persist any changes.
    for (int i = 0; i < CRATE_COUNT; ++i)
    {
        if (g_crates[i].fruitCount > g_maxCrateStock)
        {
            g_crates[i].fruitCount = g_maxCrateStock;
            Profile_SetCrateFruitCount(i, g_crates[i].fruitCount);
        }
    }

    std::cout << "Crate max stock set to " << g_maxCrateStock << "\n";
}

// ---------------------------------------------------------------------------
// Stock queries & mutation
// ---------------------------------------------------------------------------

int Crate_GetFruitCount(int crateIndex)
{
    if (crateIndex < 0 || crateIndex >= CRATE_COUNT) return -1;
    if (!g_crates[crateIndex].isUnlocked)           return -1;
    return g_crates[crateIndex].fruitCount;
}

int Crate_GetFruitType(int crateIndex)
{
    if (crateIndex < 0 || crateIndex >= CRATE_COUNT) return -1;
    if (!g_crates[crateIndex].isUnlocked)           return -1;
    return g_crates[crateIndex].fruitType;
}

int Crate_SetFruitType(int crateIndex, int fruitType)
{
    if (crateIndex < 0 || crateIndex >= CRATE_COUNT) return -1;
    if (!g_crates[crateIndex].isUnlocked)           return -1;
    g_crates[crateIndex].fruitType = static_cast<FruitType>(fruitType);
    return g_crates[crateIndex].fruitType;
}

bool Crate_AddFruit(int crateIndex, int amount)
{
    if (crateIndex < 0 || crateIndex >= CRATE_COUNT) return false;
    if (!g_crates[crateIndex].isUnlocked)           return false;
    if (amount <= 0)                                return false;

    int before = g_crates[crateIndex].fruitCount;
    g_crates[crateIndex].fruitCount += amount;
    if (g_crates[crateIndex].fruitCount > g_maxCrateStock)
        g_crates[crateIndex].fruitCount = g_maxCrateStock;

    bool added = (g_crates[crateIndex].fruitCount > before);
    if (added)
    {
        Profile_SetCrateFruitCount(crateIndex, g_crates[crateIndex].fruitCount);
        std::cout << "Crate[" << crateIndex << "] +fruit -> stock="
            << g_crates[crateIndex].fruitCount << "\n";
    }
    return added;
}

bool Crate_AddFruitTyped(int fruitType, int amount)
{
    if (fruitType < 0 || fruitType >= CRATE_COUNT) return false;
    return Crate_AddFruit(fruitType, amount);
}

bool Crate_RemoveFruit(int crateIndex)
{
    return Crate_RemoveFruitAmount(crateIndex, 1);
}

bool Crate_RemoveFruitAmount(int crateIndex, int amount)
{
    if (crateIndex < 0 || crateIndex >= CRATE_COUNT) return false;
    if (!g_crates[crateIndex].isUnlocked)           return false;
    if (amount <= 0)                                return false;
    if (g_crates[crateIndex].fruitCount < amount)   return false;

    g_crates[crateIndex].fruitCount -= amount;
    Profile_SetCrateFruitCount(crateIndex, g_crates[crateIndex].fruitCount);

    std::cout << "Crate[" << crateIndex << "] -" << amount << " fruit -> stock="
        << g_crates[crateIndex].fruitCount << "\n";
    return true;
}

bool Crate_RemoveFruitTyped(int fruitType, int amount)
{
    if (fruitType < 0 || fruitType >= CRATE_COUNT) return false;
    return Crate_RemoveFruitAmount(fruitType, amount);
}
