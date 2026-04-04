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

    AEGfxTexture* apple_1_texture = nullptr;
    AEGfxTexture* apple_2_texture = nullptr;
    AEGfxTexture* apple_3_texture = nullptr;

    AEGfxTexture* banana_1_texture = nullptr;
    AEGfxTexture* banana_2_texture = nullptr;
    AEGfxTexture* banana_3_texture = nullptr;

    AEGfxTexture* pear_1_texture = nullptr;
    AEGfxTexture* pear_2_texture = nullptr;
    AEGfxTexture* pear_3_texture = nullptr;

	int apple_scale_x_1 = 0, apple_scale_y_1 = 0;
	int apple_scale_x_2 = 0, apple_scale_y_2 = 0;
	int apple_scale_x_3 = 0, apple_scale_y_3 = 0;

	int banana_scale_x_1 = 0, banana_scale_y_1 = 0;
	int banana_scale_x_2 = 0, banana_scale_y_2 = 0;
	int banana_scale_x_3 = 0, banana_scale_y_3 = 0;

	int pear_scale_x_1 = 0, pear_scale_y_1 = 0;
	int pear_scale_x_2 = 0, pear_scale_y_2 = 0;
	int pear_scale_x_3 = 0, pear_scale_y_3 = 0;

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
    crate0.crate_texture = AEGfxTextureLoad("Assets/Crate_First.png");

    crate0.apple_1_texture = AEGfxTextureLoad("Assets/Apple_1_First.png");
    crate0.apple_2_texture = AEGfxTextureLoad("Assets/Apple_2_First.png");
    crate0.apple_3_texture = AEGfxTextureLoad("Assets/Apple_3_First.png");

	crate0.banana_1_texture = AEGfxTextureLoad("Assets/Banana_1_First.png");
	crate0.banana_2_texture = AEGfxTextureLoad("Assets/Banana_2_First.png");
	crate0.banana_3_texture = AEGfxTextureLoad("Assets/Banana_3_First.png");

	crate0.pear_1_texture = AEGfxTextureLoad("Assets/Pear_1_First.png");
	crate0.pear_2_texture = AEGfxTextureLoad("Assets/Pear_2_First.png");
	crate0.pear_3_texture = AEGfxTextureLoad("Assets/Pear_3_First.png");

    crate0.crate_x = 143.0f;
    crate0.crate_y = -148.0f;

	// Apple positions and scales for different stock levels 
    crate0.apple_x_1 = crate0.crate_x;
    crate0.apple_y_1 = crate0.crate_y - 20;

    crate0.apple_x_2 = crate0.crate_x + 2;
    crate0.apple_y_2 = crate0.crate_y - 9.0f;

    crate0.apple_x_3 = crate0.crate_x + 5.5;
    crate0.apple_y_3 = crate0.crate_y - 1;

	crate0.apple_scale_x_1 = 145;
	crate0.apple_scale_y_1 = 48;

	crate0.apple_scale_x_2 = 150;
	crate0.apple_scale_y_2 = 74;

	crate0.apple_scale_x_3 = 158;
	crate0.apple_scale_y_3 = 94;

	// Banana positions and scales for different stock levels
	crate0.banana_scale_x_1 = 158;
	crate0.banana_scale_y_1 = 64;

	crate0.banana_scale_x_2 = 174;
	crate0.banana_scale_y_2 = 87;

	crate0.banana_scale_x_3 = 175;
	crate0.banana_scale_y_3 = 107;

	// Pear positions and scales for different stock levels
	crate0.pear_scale_x_1 = 135;
	crate0.pear_scale_y_1 = 68;

	crate0.pear_scale_x_2 = 150;
	crate0.pear_scale_y_2 = 89;

	crate0.pear_scale_x_3 = 150;
	crate0.pear_scale_y_3 = 109;

    //==========================================================//

    // Second Crate
    CrateData& crate1 = g_crates[1];

    crate1.CrateMesh = CreateMesh();
    crate1.AppleMesh = CreateMesh();

    //Textures First Crate
    crate1.crate_texture = AEGfxTextureLoad("Assets/Crate_Second.png");

    crate1.apple_1_texture = AEGfxTextureLoad("Assets/Apple_1_Second.png");
    crate1.apple_2_texture = AEGfxTextureLoad("Assets/Apple_2_Second.png");
    crate1.apple_3_texture = AEGfxTextureLoad("Assets/Apple_3_Second.png");

    crate1.banana_1_texture = AEGfxTextureLoad("Assets/Banana_1_Second.png");
    crate1.banana_2_texture = AEGfxTextureLoad("Assets/Banana_2_Second.png");
    crate1.banana_3_texture = AEGfxTextureLoad("Assets/Banana_3_Second.png");

    crate1.pear_1_texture = AEGfxTextureLoad("Assets/Pear_1_Second.png");
    crate1.pear_2_texture = AEGfxTextureLoad("Assets/Pear_2_Second.png");
    crate1.pear_3_texture = AEGfxTextureLoad("Assets/Pear_3_Second.png");

    crate1.crate_x = 258.0f;
    crate1.crate_y = -148.0f;

    // Apple positions and scales for different stock levels 
    crate1.apple_x_1 = crate1.crate_x;
    crate1.apple_y_1 = crate1.crate_y - 20.2;

    crate1.apple_x_2 = crate1.crate_x + 2;
    crate1.apple_y_2 = crate1.crate_y - 9.5f;

    crate1.apple_x_3 = crate1.crate_x ;
    crate1.apple_y_3 = crate1.crate_y + 0.5;

    crate1.apple_scale_x_1 = 133;
    crate1.apple_scale_y_1 = 48;

    crate1.apple_scale_x_2 = 133;
    crate1.apple_scale_y_2 = 75;

    crate1.apple_scale_x_3 = 133;
    crate1.apple_scale_y_3 = 98;

    // Banana positions and scales for different stock levels
    crate1.banana_scale_x_1 = 142;
    crate1.banana_scale_y_1 = 64;

    crate1.banana_scale_x_2 = 142;
    crate1.banana_scale_y_2 = 87;

    crate1.banana_scale_x_3 = 142;
    crate1.banana_scale_y_3 = 107;

    // Pear positions and scales for different stock levels
    crate1.pear_scale_x_1 = 128;
    crate1.pear_scale_y_1 = 59;

    crate1.pear_scale_x_2 = 128;
    crate1.pear_scale_y_2 = 80;

    crate1.pear_scale_x_3 = 128;
    crate1.pear_scale_y_3 = 100;

    //==========================================================//

    // Third Crate
    CrateData& crate2 = g_crates[2];

    crate2.CrateMesh = CreateMesh();
    crate2.AppleMesh = CreateMesh();

    //Textures First Crate
    crate2.crate_texture = AEGfxTextureLoad("Assets/Crate_Third.png");

    crate2.apple_1_texture = AEGfxTextureLoad("Assets/Apple_1_Third.png");
    crate2.apple_2_texture = AEGfxTextureLoad("Assets/Apple_2_Third.png");
    crate2.apple_3_texture = AEGfxTextureLoad("Assets/Apple_3_Third.png");

    crate2.banana_1_texture = AEGfxTextureLoad("Assets/Banana_1_Third.png");
    crate2.banana_3_texture = AEGfxTextureLoad("Assets/Banana_3_Third.png");
    crate2.banana_2_texture = AEGfxTextureLoad("Assets/Banana_2_Third.png");

    crate2.pear_1_texture = AEGfxTextureLoad("Assets/Pear_1_Third.png");
    crate2.pear_2_texture = AEGfxTextureLoad("Assets/Pear_2_Third.png");
    crate2.pear_3_texture = AEGfxTextureLoad("Assets/Pear_3_Third.png");

    crate2.crate_x = 365.0f;
    crate2.crate_y = -148.0f;

    // Apple positions and scales for different stock levels 
    crate2.apple_x_1 = crate2.crate_x + 10.3;
    crate2.apple_y_1 = crate2.crate_y - 21.5;

    crate2.apple_x_2 = crate2.crate_x + 8.2;
    crate2.apple_y_2 = crate2.crate_y - 10.0f;

    crate2.apple_x_3 = crate2.crate_x + 5;
    crate2.apple_y_3 = crate2.crate_y - 1.5;

    crate2.apple_scale_x_1 = 133;
    crate2.apple_scale_y_1 = 48;

    crate2.apple_scale_x_2 = 138;
    crate2.apple_scale_y_2 = 75;

    crate2.apple_scale_x_3 = 146;
    crate2.apple_scale_y_3 = 96;

    // Banana scales for different stock levels
    crate2.banana_scale_x_1 = 135;
    crate2.banana_scale_y_1 = 64;

    crate2.banana_scale_x_2 = 148;
    crate2.banana_scale_y_2 = 87;

    crate2.banana_scale_x_3 = 159;
    crate2.banana_scale_y_3 = 107;

    // Pear scales for different stock levels
    crate2.pear_scale_x_1 = 127;
    crate2.pear_scale_y_1 = 68;

    crate2.pear_scale_x_2 = 136;
    crate2.pear_scale_y_2 = 89;

    crate2.pear_scale_x_3 = 145;
    crate2.pear_scale_y_3 = 109;
    

}

void Crate_Update()
{
    // Nothing to update for now, but we could add animations or effects here later.
    for (int i = 0; i < CRATE_COUNT; ++i)
    {
        g_crates[i].fruitType = FruitType(Crate_GetFruitType(i));
    }
    // Update How many apples are in the crate for testing

}

bool hello = false; // testing variable for mouse click


void Crate_Draw()
{

    CrateData& crate0 = g_crates[0];
    
    if (crate0.isUnlocked) {
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

        // Pick textures and scale values based on the current fruit type (safe, explicit)
        AEGfxTexture* fruitTexture_1 = nullptr;
        AEGfxTexture* fruitTexture_2 = nullptr;
        AEGfxTexture* fruitTexture_3 = nullptr;

        int scalex_1 = 0, scaley_1 = 0;
        int scalex_2 = 0, scaley_2 = 0;
        int scalex_3 = 0, scaley_3 = 0;

        float x_1 = 0, y_1 = 0;
        float x_2 = 0, y_2 = 0;
        float x_3 = 0, y_3 = 0;

        switch (crate0.fruitType) {
        case APPLE:
            fruitTexture_1 = crate0.apple_1_texture;
            fruitTexture_2 = crate0.apple_2_texture;
            fruitTexture_3 = crate0.apple_3_texture;

            scalex_1 = crate0.apple_scale_x_1; scaley_1 = crate0.apple_scale_y_1;
            scalex_2 = crate0.apple_scale_x_2; scaley_2 = crate0.apple_scale_y_2;
            scalex_3 = crate0.apple_scale_x_3; scaley_3 = crate0.apple_scale_y_3;

            x_1 = crate0.apple_x_1; y_1 = crate0.apple_y_1;
            x_2 = crate0.apple_x_2; y_2 = crate0.apple_y_2;
            x_3 = crate0.apple_x_3; y_3 = crate0.apple_y_3;

            break;
        case BANANA:
            fruitTexture_1 = crate0.banana_1_texture;
            fruitTexture_2 = crate0.banana_2_texture;
            fruitTexture_3 = crate0.banana_3_texture;

            scalex_1 = crate0.banana_scale_x_1; scaley_1 = crate0.banana_scale_y_1;
            scalex_2 = crate0.banana_scale_x_2; scaley_2 = crate0.banana_scale_y_2;
            scalex_3 = crate0.banana_scale_x_3; scaley_3 = crate0.banana_scale_y_3;

			x_1 = crate0.apple_x_1 - 9; y_1 = crate0.apple_y_1 + 4;
			x_2 = crate0.apple_x_2 - 4; y_2 = crate0.apple_y_2 + 2.5;
			x_3 = crate0.apple_x_3 - 7; y_3 = crate0.apple_y_3 + 2.9;

            break;
        case PEAR:
            fruitTexture_1 = crate0.pear_1_texture;
            fruitTexture_2 = crate0.pear_2_texture;
            fruitTexture_3 = crate0.pear_3_texture;
            scalex_1 = crate0.pear_scale_x_1; scaley_1 = crate0.pear_scale_y_1;
            scalex_2 = crate0.pear_scale_x_2; scaley_2 = crate0.pear_scale_y_2;
            scalex_3 = crate0.pear_scale_x_3; scaley_3 = crate0.pear_scale_y_3;

            x_1 = crate0.apple_x_1 - 5; y_1 = crate0.apple_y_1 + 8.5;
            x_2 = crate0.apple_x_2; y_2 = crate0.apple_y_2 + 5;
			x_3 = crate0.apple_x_3 - 5; y_3 = crate0.apple_y_3 + 5;
            break;
        default:
            // leave textures null
            break;
        }

        // Render fruits based on stock percentage (0-100% mapped to 3 textures)
        if (stockPercent > 0.0f) {
            // If stock is 30% or less
            if (stockPercent <= 0.3f) {
                AEGfxTextureSet(fruitTexture_1, 0, 0);  // Set the texture
                AEMtx33Scale(&scale, scalex_1 * gScaleX, scaley_1 * gScaleY);
                AEMtx33Trans(&trans, x_1, y_1);  // Apply position transformation
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(crate0.AppleMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
            }

            // If stock is between 30% and 70%
            if (stockPercent > 0.3f && stockPercent <= 0.7f) {
                AEGfxTextureSet(fruitTexture_2, 0, 0);  // Set the texture
                AEMtx33Scale(&scale, scalex_2 * gScaleX, scaley_2 * gScaleY);
                AEMtx33Trans(&trans, x_2, y_2);  // Apply position transformation
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(crate0.AppleMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
            }

            // If stock is above 70%
            if (stockPercent > 0.7f) {
                AEGfxTextureSet(fruitTexture_3, 0, 0);  // Set the texture
                AEMtx33Scale(&scale, scalex_3 * gScaleX, scaley_3 * gScaleY);
                AEMtx33Trans(&trans, x_3, y_3);  // Apply position transformation
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(crate0.AppleMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
            }




        }

    }

    CrateData& crate2 = g_crates[2];

    if (crate2.isUnlocked) {
        // Render Crate
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);  // Use texture rendering mode
        AEGfxTextureSet(crate2.crate_texture, 0, 0);  // Set the texture

        AEMtx33 scale, trans, rotation, transform, rotscale;

        AEMtx33Scale(&scale, 181 * gScaleX, 100 * gScaleY);

        AEMtx33Rot(&rotation, 0);  // Create rotation matrix 

        AEMtx33Trans(&trans, crate2.crate_x, crate2.crate_y);  // Apply position transformation

        AEMtx33Concat(&rotscale, &rotation, &scale);
        AEMtx33Concat(&transform, &trans, &rotscale);

        AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple

        AEGfxMeshDraw(crate2.CrateMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad

        // Render Apples based on stock count
        float stockPercent = crate2.fruitCount / float(g_maxCrateStock); // <<--- make global later when we have more crates

        // Pick textures and scale values based on the current fruit type (safe, explicit)
        AEGfxTexture* fruitTexture_1 = nullptr;
        AEGfxTexture* fruitTexture_2 = nullptr;
        AEGfxTexture* fruitTexture_3 = nullptr;

        int scalex_1 = 0, scaley_1 = 0;
        int scalex_2 = 0, scaley_2 = 0;
        int scalex_3 = 0, scaley_3 = 0;

        float x_1 = 0, y_1 = 0;
        float x_2 = 0, y_2 = 0;
        float x_3 = 0, y_3 = 0;

      /*  if (AEInputCheckReleased(AEVK_LBUTTON)) {
            hello = !hello;
            crate2.fruitType = APPLE;
            printf("Mouse click detected! Toggling fruit display. Current fruit type: %d\n", crate2.fruitType);
        }*/

        switch (crate2.fruitType) {
        case APPLE:
            fruitTexture_1 = crate2.apple_1_texture;
            fruitTexture_2 = crate2.apple_2_texture;
            fruitTexture_3 = crate2.apple_3_texture;

            scalex_1 = crate2.apple_scale_x_1; scaley_1 = crate2.apple_scale_y_1;
            scalex_2 = crate2.apple_scale_x_2; scaley_2 = crate2.apple_scale_y_2;
            scalex_3 = crate2.apple_scale_x_3; scaley_3 = crate2.apple_scale_y_3;

            x_1 = crate2.apple_x_1; y_1 = crate2.apple_y_1;
            x_2 = crate2.apple_x_2; y_2 = crate2.apple_y_2;
            x_3 = crate2.apple_x_3; y_3 = crate2.apple_y_3;

            break;
        case BANANA:
            fruitTexture_1 = crate2.banana_1_texture;
            fruitTexture_2 = crate2.banana_2_texture;
            fruitTexture_3 = crate2.banana_3_texture;

            scalex_1 = crate2.banana_scale_x_1; scaley_1 = crate2.banana_scale_y_1;
            scalex_2 = crate2.banana_scale_x_2; scaley_2 = crate2.banana_scale_y_2;
            scalex_3 = crate2.banana_scale_x_3; scaley_3 = crate2.banana_scale_y_3;

            x_1 = crate2.apple_x_1 + 1.5; y_1 = crate2.apple_y_1 + 4;
            x_2 = crate2.apple_x_2 - 1.5; y_2 = crate2.apple_y_2 + 2;
            x_3 = crate2.apple_x_3 - 3; y_3 = crate2.apple_y_3 + 1.8;

            break;
        case PEAR:
            fruitTexture_1 = crate2.pear_1_texture;
            fruitTexture_2 = crate2.pear_2_texture;
            fruitTexture_3 = crate2.pear_3_texture;
            scalex_1 = crate2.pear_scale_x_1; scaley_1 = crate2.pear_scale_y_1;
            scalex_2 = crate2.pear_scale_x_2; scaley_2 = crate2.pear_scale_y_2;
            scalex_3 = crate2.pear_scale_x_3; scaley_3 = crate2.pear_scale_y_3;

            x_1 = crate2.apple_x_1; y_1 = crate2.apple_y_1 + 8.2 ;
            x_2 = crate2.apple_x_2 - 1.6; y_2 = crate2.apple_y_2 + 5.2 ;
            x_3 = crate2.apple_x_3 - 2; y_3 = crate2.apple_y_3 + 5;
            break;
        default:
            // leave textures null
            break;
        }

        // Render fruits based on stock percentage (0-100% mapped to 3 textures)
        if (stockPercent > 0.0f) {
            // If stock is 30% or less
            if (stockPercent <= 0.3f && stockPercent > 0.0f) {
                AEGfxTextureSet(fruitTexture_1, 0, 0);  // Set the texture
                AEMtx33Scale(&scale, scalex_1 * gScaleX, scaley_1 * gScaleY);
                AEMtx33Trans(&trans, x_1, y_1);  // Apply position transformation
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(crate2.AppleMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
            }

            // If stock is between 30% and 70%
            if (stockPercent > 0.3f && stockPercent <= 0.7f) {
                AEGfxTextureSet(fruitTexture_2, 0, 0);  // Set the texture
                AEMtx33Scale(&scale, scalex_2 * gScaleX, scaley_2 * gScaleY);
                AEMtx33Trans(&trans, x_2, y_2);  // Apply position transformation
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(crate2.AppleMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
            }

            // If stock is above 70%
            if (stockPercent > 0.7f) {
                AEGfxTextureSet(fruitTexture_3, 0, 0);  // Set the texture
                AEMtx33Scale(&scale, scalex_3 * gScaleX, scaley_3 * gScaleY);
                AEMtx33Trans(&trans, x_3, y_3);  // Apply position transformation
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(crate2.AppleMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
            }




        }

    }

    CrateData& crate1 = g_crates[1];

    if (crate1.isUnlocked) {
        // Render Crate
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);  // Use texture rendering mode
        AEGfxTextureSet(crate1.crate_texture, 0, 0);  // Set the texture

        AEMtx33 scale, trans, rotation, transform, rotscale;

        AEMtx33Scale(&scale, 150 * gScaleX, 101 * gScaleY);

        AEMtx33Rot(&rotation, 0);  // Create rotation matrix 

        AEMtx33Trans(&trans, crate1.crate_x, crate1.crate_y);  // Apply position transformation

        AEMtx33Concat(&rotscale, &rotation, &scale);
        AEMtx33Concat(&transform, &trans, &rotscale);

        AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple

        AEGfxMeshDraw(crate1.CrateMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad

        // Render Apples based on stock count
        float stockPercent = crate1.fruitCount / float(g_maxCrateStock); // <<--- make global later when we have more crates

        // Pick textures and scale values based on the current fruit type (safe, explicit)
        AEGfxTexture* fruitTexture_1 = nullptr;
        AEGfxTexture* fruitTexture_2 = nullptr;
        AEGfxTexture* fruitTexture_3 = nullptr;

        int scalex_1 = 0, scaley_1 = 0;
        int scalex_2 = 0, scaley_2 = 0;
        int scalex_3 = 0, scaley_3 = 0;

        float x_1 = 0, y_1 = 0;
        float x_2 = 0, y_2 = 0;
        float x_3 = 0, y_3 = 0;

        switch (crate1.fruitType) {
        case APPLE:
            fruitTexture_1 = crate1.apple_1_texture;
            fruitTexture_2 = crate1.apple_2_texture;
            fruitTexture_3 = crate1.apple_3_texture;

            scalex_1 = crate1.apple_scale_x_1; scaley_1 = crate1.apple_scale_y_1;
            scalex_2 = crate1.apple_scale_x_2; scaley_2 = crate1.apple_scale_y_2;
            scalex_3 = crate1.apple_scale_x_3; scaley_3 = crate1.apple_scale_y_3;

            x_1 = crate1.apple_x_1; y_1 = crate1.apple_y_1;
            x_2 = crate1.apple_x_2; y_2 = crate1.apple_y_2;
            x_3 = crate1.apple_x_3; y_3 = crate1.apple_y_3;

            break;
        case BANANA:
            fruitTexture_1 = crate1.banana_1_texture;
            fruitTexture_2 = crate1.banana_2_texture;
            fruitTexture_3 = crate1.banana_3_texture;

            scalex_1 = crate1.banana_scale_x_1; scaley_1 = crate1.banana_scale_y_1;
            scalex_2 = crate1.banana_scale_x_2; scaley_2 = crate1.banana_scale_y_2;
            scalex_3 = crate1.banana_scale_x_3; scaley_3 = crate1.banana_scale_y_3;

            x_1 = crate1.apple_x_1 - 2; y_1 = crate1.apple_y_1 + 3.1;
            x_2 = crate1.apple_x_2 - 4; y_2 = crate1.apple_y_2 + 2.0;
            x_3 = crate1.apple_x_3 - 2; y_3 = crate1.apple_y_3 + 0.4;

            break;
        case PEAR:
            fruitTexture_1 = crate1.pear_1_texture;
            fruitTexture_2 = crate1.pear_2_texture;
            fruitTexture_3 = crate1.pear_3_texture;
            scalex_1 = crate1.pear_scale_x_1; scaley_1 = crate1.pear_scale_y_1;
            scalex_2 = crate1.pear_scale_x_2; scaley_2 = crate1.pear_scale_y_2;
            scalex_3 = crate1.pear_scale_x_3; scaley_3 = crate1.pear_scale_y_3;

            x_1 = crate1.apple_x_1 ; y_1 = crate1.apple_y_1 + 11.9;
            x_2 = crate1.apple_x_2 - 2; y_2 = crate1.apple_y_2 + 9.9;
            x_3 = crate1.apple_x_3 ; y_3 = crate1.apple_y_3 + 8.2;
            break;
        default:
            // leave textures null
            break;
        }

        // Render fruits based on stock percentage (0-100% mapped to 3 textures)
        if (stockPercent > 0.0f ){
            // If stock is 30% or less
            if (stockPercent <= 0.3f && stockPercent > 0.0f ) {
                AEGfxTextureSet(fruitTexture_1, 0, 0);  // Set the texture
                AEMtx33Scale(&scale, scalex_1 * gScaleX, scaley_1 * gScaleY);
                AEMtx33Trans(&trans, x_1, y_1);  // Apply position transformation
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(crate1.AppleMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
            }

            // If stock is between 30% and 70%
            if ( stockPercent > 0.3f && stockPercent <= 0.7f ) {
                AEGfxTextureSet(fruitTexture_2, 0, 0);  // Set the texture
                AEMtx33Scale(&scale, scalex_2 * gScaleX, scaley_2 * gScaleY);
                AEMtx33Trans(&trans, x_2, y_2);  // Apply position transformation
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(crate1.AppleMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
            }

            // If stock is above 70%
            if (stockPercent > 0.7f ) {
                AEGfxTextureSet(fruitTexture_3, 0, 0);  // Set the texture
                AEMtx33Scale(&scale, scalex_3 * gScaleX, scaley_3 * gScaleY);
                AEMtx33Trans(&trans, x_3, y_3);  // Apply position transformation
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(crate1.AppleMesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
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
