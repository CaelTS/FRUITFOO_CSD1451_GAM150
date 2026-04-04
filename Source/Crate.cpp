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

    int  fruitCount = 0;     // current stock

    AEGfxVertexList* CrateMesh = NULL;
    AEGfxVertexList* AppleMesh = NULL;

    AEGfxTexture* crate_texture = nullptr;
    f32 crate_x = 0.0f, crate_y = 0.0f;

    AEGfxTexture* apple_1_texture = nullptr;
    AEGfxTexture* apple_2_texture = nullptr;
    AEGfxTexture* apple_3_texture = nullptr;

    f32 apple_x_1 = 0.0f, apple_y_1 = 0.0f;
    f32 apple_x_2 = 0.0f, apple_y_2 = 0.0f;
    f32 apple_x_3 = 0.0f, apple_y_3 = 0.0f;
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
    // [crate] section), force all crates unlocked and persist.
    bool anyUnlocked = false;
    for (int i = 0; i < CRATE_COUNT; i++)
        if (g_crates[i].isUnlocked) { anyUnlocked = true; break; }

    if (!anyUnlocked)
    {
        for (int i = 0; i < CRATE_COUNT; i++)
        {
            g_crates[i].isUnlocked = true;
            Profile_SetCrateUnlocked(i, true);
        }
        std::cout << "Crate migration: all crates force-unlocked\n";
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

    // ---------------------------------------------------------------------------
    // Derive bin world positions from the same stall transform used in
    // MainScreen_Render() and UI_RebuildCrateHitboxesFromStall():
    //   stallX = 330*gScaleX,  stallY = -15*gScaleY
    //   stallW = 702*gScaleX,  stallH = 716*gScaleY
    //
    // UV values from UI_ResetCrateConfigToDefaults:
    //   bin[0] uCenter=0.285, vCenter=0.740  -> Apple  (left bin)
    //   bin[1] uCenter=0.475, vCenter=0.740  -> Pear   (middle bin)
    //   bin[2] uCenter=0.660, vCenter=0.740  -> Banana (right bin)
    //
    // worldX = stallX + (uCenter - 0.5) * stallW
    // worldY = stallY + (0.5 - vCenter) * stallH
    // ---------------------------------------------------------------------------
    const float stallX = 330.0f * gScaleX;
    const float stallY = -15.0f * gScaleY;
    const float stallW = 702.0f * gScaleX;
    const float stallH = 716.0f * gScaleY;

    const float binWorldY = stallY + (0.5f - 0.740f) * stallH;  // shared Y
    const float binWorldX_Apple = stallX + (0.285f - 0.5f) * stallW;
    const float binWorldX_Pear = stallX + (0.475f - 0.5f) * stallW;
    const float binWorldX_Banana = stallX + (0.660f - 0.5f) * stallW;

    //--------------------------------------------------------------------------------------
    // --- Crate 0 (Apple) ---
    CrateData& crate0 = g_crates[0];
    crate0.CrateMesh = CreateMesh();
    crate0.AppleMesh = CreateMesh();

    crate0.apple_1_texture = AEGfxTextureLoad("Assets/Apple_1_First.png");
    crate0.apple_2_texture = AEGfxTextureLoad("Assets/Apple_2_First.png");
    crate0.apple_3_texture = AEGfxTextureLoad("Assets/Apple_3_First.png");
    if (!crate0.apple_1_texture) OutputDebugStringA("ERROR: Failed to load 'Assets/Apple_1_First.png'.\n");
    crate0.crate_texture = AEGfxTextureLoad("Assets/Crate_First.png");

    crate0.crate_x = binWorldX_Apple;
    crate0.crate_y = binWorldY;
    crate0.apple_x_1 = crate0.crate_x;       crate0.apple_y_1 = crate0.crate_y - 20;
    crate0.apple_x_2 = crate0.crate_x + 2;   crate0.apple_y_2 = crate0.crate_y - 9.0f;
    crate0.apple_x_3 = crate0.crate_x + 5.5f; crate0.apple_y_3 = crate0.crate_y - 1;

    printf("Crate 0 (Apple)  pos=(%.1f, %.1f) stock=%d\n", crate0.crate_x, crate0.crate_y, crate0.fruitCount);

    // --- Crate 1 (Pear) ---
    CrateData& crate1 = g_crates[1];
    crate1.CrateMesh = CreateMesh();
    crate1.AppleMesh = CreateMesh();

    // Try dedicated fill textures; fall back to the generic Pear.png
    crate1.apple_1_texture = AEGfxTextureLoad("Assets/Pear_1_Second.png");
    if (!crate1.apple_1_texture) crate1.apple_1_texture = AEGfxTextureLoad("Assets/Pear.png");
    crate1.apple_2_texture = AEGfxTextureLoad("Assets/Pear_2_Second.png");
    if (!crate1.apple_2_texture) crate1.apple_2_texture = AEGfxTextureLoad("Assets/Pear.png");
    crate1.apple_3_texture = AEGfxTextureLoad("Assets/Pear_3_Second.png");
    if (!crate1.apple_3_texture) crate1.apple_3_texture = AEGfxTextureLoad("Assets/Pear.png");
    crate1.crate_texture = AEGfxTextureLoad("Assets/Crate_Second.png");
    if (!crate1.crate_texture) OutputDebugStringA("ERROR: Failed to load crate texture for crate 1.\n");

    crate1.crate_x = binWorldX_Pear;
    crate1.crate_y = binWorldY;
    crate1.apple_x_1 = crate1.crate_x;       crate1.apple_y_1 = crate1.crate_y - 20;
    crate1.apple_x_2 = crate1.crate_x + 2;   crate1.apple_y_2 = crate1.crate_y - 9.0f;
    crate1.apple_x_3 = crate1.crate_x + 5.5f; crate1.apple_y_3 = crate1.crate_y - 1;

    printf("Crate 1 (Pear)   pos=(%.1f, %.1f) stock=%d\n", crate1.crate_x, crate1.crate_y, crate1.fruitCount);

    // --- Crate 2 (Banana) ---
    CrateData& crate2 = g_crates[2];
    crate2.CrateMesh = CreateMesh();
    crate2.AppleMesh = CreateMesh();

    // Try dedicated fill textures; fall back to the generic Banana.png
    crate2.apple_1_texture = AEGfxTextureLoad("Assets/Banana_1_Third.png");
    if (!crate2.apple_1_texture) crate2.apple_1_texture = AEGfxTextureLoad("Assets/Banana.png");
    crate2.apple_2_texture = AEGfxTextureLoad("Assets/Banana_2_Third.png");
    if (!crate2.apple_2_texture) crate2.apple_2_texture = AEGfxTextureLoad("Assets/Banana.png");
    crate2.apple_3_texture = AEGfxTextureLoad("Assets/Banana_3_Third.png");
    if (!crate2.apple_3_texture) crate2.apple_3_texture = AEGfxTextureLoad("Assets/Banana.png");
    crate2.crate_texture = AEGfxTextureLoad("Assets/Crate_Third.png");
    if (!crate2.crate_texture) OutputDebugStringA("ERROR: Failed to load crate texture for crate 2.\n");

    crate2.crate_x = binWorldX_Banana;
    crate2.crate_y = binWorldY;
    crate2.apple_x_1 = crate2.crate_x;       crate2.apple_y_1 = crate2.crate_y - 20;
    crate2.apple_x_2 = crate2.crate_x + 2;   crate2.apple_y_2 = crate2.crate_y - 9.0f;
    crate2.apple_x_3 = crate2.crate_x + 5.5f; crate2.apple_y_3 = crate2.crate_y - 1;

    printf("Crate 2 (Banana) pos=(%.1f, %.1f) stock=%d\n", crate2.crate_x, crate2.crate_y, crate2.fruitCount);
}

void Crate_Update(float dt)
{
    (void)dt;
}

void Crate_Draw()
{
    AEMtx33 scale, trans, rotation, transform, rotscale;

    for (int i = 0; i < CRATE_COUNT; ++i)
    {
        CrateData& crate = g_crates[i];

        if (!crate.isUnlocked)    continue;
        if (!crate.crate_texture) continue;
        if (!crate.CrateMesh)     continue;

        // --- Draw the crate box ---
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f); // clear additive tint — prevents white bg bleed
        AEGfxSetTransparency(1.0f);
        AEGfxTextureSet(crate.crate_texture, 0, 0);

        AEMtx33Scale(&scale, 186 * gScaleX, 99 * gScaleY);
        AEMtx33Rot(&rotation, 0);
        AEMtx33Trans(&trans, crate.crate_x, crate.crate_y);
        AEMtx33Concat(&rotscale, &rotation, &scale);
        AEMtx33Concat(&transform, &trans, &rotscale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(crate.CrateMesh, AE_GFX_MDM_TRIANGLES);

        // --- Draw fruit fill based on stock level ---
        float stockPercent = crate.fruitCount / float(g_maxCrateStock);

        if (stockPercent > 0.0f)
        {
            AEGfxTexture* fruitTex = nullptr;
            float fw = 0.0f, fh = 0.0f, fx = 0.0f, fy = 0.0f;

            if (stockPercent <= 0.3f)
            {
                fruitTex = crate.apple_1_texture;
                fw = 145 * gScaleX; fh = 48 * gScaleY;
                fx = crate.apple_x_1; fy = crate.apple_y_1;
            }
            else if (stockPercent <= 0.7f)
            {
                fruitTex = crate.apple_2_texture;
                fw = 150 * gScaleX; fh = 74 * gScaleY;
                fx = crate.apple_x_2; fy = crate.apple_y_2;
            }
            else
            {
                fruitTex = crate.apple_3_texture;
                fw = 158 * gScaleX; fh = 94 * gScaleY;
                fx = crate.apple_x_3; fy = crate.apple_y_3;
            }

            if (fruitTex)
            {
                // Alpha blend so transparent PNGs show no white box
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
                AEGfxSetTransparency(1.0f);
                AEGfxTextureSet(fruitTex, 0, 0);

                AEMtx33Scale(&scale, fw, fh);
                AEMtx33Trans(&trans, fx, fy);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(crate.AppleMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }
}


void Crate_Free()
{
    std::cout << "Crate_Free\n";

    for (int i = 0; i < CRATE_COUNT; ++i)
    {
        CrateData& c = g_crates[i];

        if (c.CrateMesh) { AEGfxMeshFree(c.CrateMesh);         c.CrateMesh = nullptr; }
        if (c.AppleMesh) { AEGfxMeshFree(c.AppleMesh);         c.AppleMesh = nullptr; }
        if (c.crate_texture) { AEGfxTextureUnload(c.crate_texture); c.crate_texture = nullptr; }

        // Guard against double-free when fallback makes all 3 slots share one pointer
        if (c.apple_1_texture)
        {
            AEGfxTextureUnload(c.apple_1_texture);
            if (c.apple_2_texture == c.apple_1_texture) c.apple_2_texture = nullptr;
            if (c.apple_3_texture == c.apple_1_texture) c.apple_3_texture = nullptr;
            c.apple_1_texture = nullptr;
        }
        if (c.apple_2_texture)
        {
            AEGfxTextureUnload(c.apple_2_texture);
            if (c.apple_3_texture == c.apple_2_texture) c.apple_3_texture = nullptr;
            c.apple_2_texture = nullptr;
        }
        if (c.apple_3_texture)
        {
            AEGfxTextureUnload(c.apple_3_texture);
            c.apple_3_texture = nullptr;
        }
    }
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
    Profile_SetCrateUnlocked(crateIndex, unlocked);
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
    if (maxStock < 0) return;
    g_maxCrateStock = maxStock;

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