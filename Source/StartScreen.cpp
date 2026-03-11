#include "StartScreen.h"
#include "Profile.h"
#include "AEEngine.h"
#include "Utilities.h"
#include "GameStateManager.h"
#include <fstream>

enum ButtonID
{
    BUTTON_NEW_GAME = 0,
    BUTTON_CONTINUE,
    BUTTON_SETTINGS,
    BUTTON_EXIT
};

struct Button
{
    AEGfxTexture* normal = nullptr;
    AEGfxTexture* hover = nullptr;

    int ID = 0; // 0: New Game, 1: Continue, 2: Settings, 3: Exit

    float x = 0.0f;
    float y = 0.0f;

    float x_selected = 0.0f;
    float y_selected = 0.0f;

    float x_save = 0.0f;
    float y_save = 0.0f;

    float x_selected_save = 0.0f;
    float y_selected_save = 0.0f;

    bool hovered = false;
};


//Buttons
Button newGameButton;
Button continueButton;
Button profileButton;
Button exitButton;

static bool hasSave = false; //placeholder until we implement profile system
static f32 logoPosX = -520.0;
static f32 logoPosY = 260.0;

float gScaleX = 1600.0f / 1600.0f;  // = 1.0f  (scale relative to base 1600 width)
float gScaleY = 900.0f / 900.0f;   // = 1.0f  (scale relative to base 900 height)

//animation variable
extern bool startScreenActive = true;   // Is the start screen still active?
static bool isExiting = false;          // Has the exit animation started?
static float exitAnimProgress = 0.0f;   // 0.0 -> 1.0 animation progress
static float exitAnimFadeOut = 1.0f;   // 0.0 -> 1.0 fade out progress
static float exitAnimSpeed = 1.3f;      // Speed of slide animation

//Local helper function button click detection
bool IsButtonClicked(Button& btn, float width, float height) {
    if (ClickedOnRect(btn.x, btn.y, width * gScaleX, height * gScaleY))
        return true;
    else return false;
}

bool IsButtonHovered(Button& btn, float width, float height) {
    float x = btn.x;
    float y = btn.y;

    if (hasSave) {
        x = btn.x_save;
        y = btn.y_save;
    }

    return IsMouseOverRect(x, y, width * gScaleX, height * gScaleY);
}

//ButtonID ID (float x, float y) {
//    if (!newGameButton.hovered && IsButtonHovered(newGameButton, 234.0f, 35.0f)) return BUTTON_NEW_GAME;
//    if (!continueButton.hovered && IsButtonHovered(continueButton, 300.0f, 80.0f)) return BUTTON_CONTINUE;
//    if (!settingsButton.hovered && IsButtonHovered(settingsButton, 300.0f, 80.0f)) return BUTTON_SETTINGS;
//    if (!exitButton.hovered && IsButtonHovered(exitButton, 68.0f, 39.0f)) return BUTTON_EXIT;
//}

//Textures
AEGfxTexture* logoTexture = nullptr;
AEGfxTexture* gradientBlur = nullptr;


// Meshes
AEGfxVertexList* pMeshLogo = nullptr;
AEGfxVertexList* pMeshNewGameButton = nullptr;
AEGfxVertexList* pMeshNewGameButton_Selected = nullptr;
AEGfxVertexList* pMeshContinueButton = nullptr;
AEGfxVertexList* pMeshContinueButton_Selected = nullptr;
AEGfxVertexList* pMeshProfileButton = nullptr;
AEGfxVertexList* pMeshProfileButton_Selected = nullptr;
AEGfxVertexList* pMeshExitButton = nullptr;
AEGfxVertexList* pMeshExitButton_Selected = nullptr;

AEGfxVertexList* pMeshGradientBlur = nullptr;


// ------------------------------------------------------------
// Helper
// ------------------------------------------------------------

static bool CheckSaveExists()
{
    std::ifstream file("save.dat");

    if (file.good())
        return true;

    return false;
}

void DrawButton(Button& btn, AEGfxVertexList* mesh, f32 width, f32 height, float offset)
{
    AEMtx33 scale, trans, transform;

    AEGfxTexture* tex = btn.hovered ? btn.hover : btn.normal;

    float x = btn.hovered ? btn.x_selected : btn.x;
    float y = btn.hovered ? btn.y_selected : btn.y;



    if (hasSave)
    {

        x = btn.hovered ? btn.x_selected_save : btn.x_save;
        y = btn.hovered ? btn.y_selected_save : btn.y_save;

    }



    if (!tex) return;

    AEGfxTextureSet(tex, 0, 0);
    AEGfxSetTransparency(1.0f * exitAnimFadeOut);

    AEMtx33Trans(&trans, x - offset, y);
    AEMtx33Scale(&scale, width * gScaleX, height * gScaleY);
    AEMtx33Concat(&transform, &trans, &scale);

    AEGfxSetTransform(transform.m);

    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

AEGfxVertexList* createMesh()
{
    AEGfxMeshStart();
    // bottom-left, bottom-right, top-right
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxVertexAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f);
    AEGfxVertexAdd(0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f);

    // bottom-left, top-right, top-left
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f);
    AEGfxVertexAdd(0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f);
    AEGfxVertexAdd(-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    return AEGfxMeshEnd();
}

bool StartScreen_IsActive()
{
    return startScreenActive; // from your start screen cpp
}

void StartScreen_Init()
{
    // Check if save file exists to determine if "Continue" button should be shown
    // Should be implemented in Profile.cpp, but for now we can just check if the file exists here
    //hasSave = SaveSystem_HasSaveFile();
    hasSave = false;

    logoTexture = AEGfxTextureLoad("Assets/StartScreen_Logo.png");
    gradientBlur = AEGfxTextureLoad("Assets/StartScreen_GradientBlur.png");
    pMeshLogo = createMesh();
    pMeshGradientBlur = createMesh();
    pMeshNewGameButton = createMesh();
    pMeshNewGameButton_Selected = createMesh();
    pMeshContinueButton = createMesh();
    pMeshContinueButton_Selected = createMesh();
    pMeshProfileButton = createMesh();
    pMeshProfileButton_Selected = createMesh();
    pMeshExitButton = createMesh();
    pMeshExitButton_Selected = createMesh();

    // Initialize "Exit" button (always shown)
    exitButton.normal = AEGfxTextureLoad("Assets/StartScreen_Exit.png");
    exitButton.hover = AEGfxTextureLoad("Assets/StartScreen_Exit_Selected.png");
    exitButton.x = logoPosX - 102.0f;
    exitButton.y = 45.0f;
    exitButton.x_selected = exitButton.x; // slide left on hover
    exitButton.y_selected = exitButton.y;
    exitButton.x_save = exitButton.x; // no slide when save exists
    exitButton.y_save = exitButton.y - (100.0f - 45.0f);
    exitButton.x_selected_save = exitButton.x - 1; // no slide when save exists
    exitButton.y_selected_save = exitButton.y_save;

    // Initialize "Continue" button 
    continueButton.normal = AEGfxTextureLoad("Assets/StartScreen_Continue.png");
    continueButton.hover = AEGfxTextureLoad("Assets/StartScreen_Continue_Selected.png");
    continueButton.x = logoPosX - 50.0f;
    continueButton.y = 100.0f;
    continueButton.x_selected = continueButton.x; // slide left on hover
    continueButton.y_selected = continueButton.y;
    continueButton.x_save = continueButton.x; // no slide when save exists
    continueButton.y_save = continueButton.y;
    continueButton.x_selected_save = continueButton.x; // no slide when save exists
    continueButton.y_selected_save = continueButton.y;

    // Initialize "Profile" button
    profileButton.normal = AEGfxTextureLoad("Assets/StartScreen_Profile.png");
    profileButton.hover = AEGfxTextureLoad("Assets/StartScreen_Profile_Selected.png");
    profileButton.x = logoPosX - 72.0f;
    profileButton.y = 45.0f;
    profileButton.x_selected = profileButton.x; // slide left on hover
    profileButton.y_selected = profileButton.y;
    profileButton.x_save = profileButton.x; // no slide when save exists    
    profileButton.y_save = profileButton.y;
    profileButton.x_selected_save = profileButton.x; // no slide when save exists   
    profileButton.y_selected_save = profileButton.y;

    // Initialize "New Game" button
    newGameButton.normal = AEGfxTextureLoad("Assets/StartScreen_NewGameButton.png");
    newGameButton.hover = AEGfxTextureLoad("Assets/StartScreen_NewGameButton_Selected.png");
    newGameButton.x = logoPosX - 32.0f;
    newGameButton.y = 100.0f;
    newGameButton.x_selected = newGameButton.x; // slide left on hover
    newGameButton.y_selected = newGameButton.y;


}

void StartScreen_Update(float dt)
{

    int mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    // Mouse is outside the window, reset hover states
    newGameButton.hovered = false;
    continueButton.hovered = false;
    profileButton.hovered = false;
    exitButton.hovered = false;

    // If mouse is outside window, keep all false
    if (mouseX >= 0 && mouseX <= 1600 && mouseY >= 0 && mouseY <= 900)
    {
    }

    if (AEInputCheckTriggered(AEVK_RSHIFT)) {
        hasSave = !hasSave; // toggle save file existence for testing
    }

    // --- Input handling ---
    if (!isExiting)
    {
        if (hasSave)
        {
            if (IsButtonClicked(profileButton, 137.0f, 40.0f))
            {
                // Go to profile screen
                nextState = GS_NEXT_SCREEN;
            }
            else if (IsButtonClicked(continueButton, 190.0f, 41.0f))
            {
                // Continue game (for now just go to farm screen)
                isExiting = true;
            }

            // When a save exists, only check the save-related buttons
            if (IsButtonHovered(continueButton, 190.0f, 41.0f))
                continueButton.hovered = true;
            else if (IsButtonHovered(profileButton, 137.0f, 40.0f))
                profileButton.hovered = true;
            else if (IsButtonHovered(exitButton, 68.0f, 39.0f))
                exitButton.hovered = true;

        }
        else
        {
            if (IsButtonClicked(newGameButton, 234.0f, 35.0f)) {
                isExiting = true;
            }
            // No save: check NewGame + Exit
            if (IsButtonHovered(newGameButton, 234.0f, 35.0f))
                newGameButton.hovered = true;
            else if (IsButtonHovered(exitButton, 68.0f, 39.0f))
                exitButton.hovered = true;

        }

        if (IsButtonClicked(exitButton, 68.0f, 39.0f))
        {
            nextState = GS_EXIT;
        }

        // For now, just simulate button click with keyboard for testing
        if (AEInputCheckTriggered(AEVK_RETURN)) // press Enter to start
        {
            isExiting = true;
        }

    }
    else
    {
        // Animate exit
        exitAnimProgress += (dt * exitAnimSpeed) * 0.1f;
        exitAnimFadeOut -= (dt * exitAnimSpeed);
        if (exitAnimProgress >= 1.0f)
        {
            exitAnimProgress = 1.0f;
            startScreenActive = false; // animation complete
        }
    }
}

void StartScreen_Draw()
{

    if (!startScreenActive) return; // don�t draw after animation finished

    float slideOffset = exitAnimProgress * 3000.0f; // pixels to move left
    float fadeOut = exitAnimFadeOut; // fade out from 1 to 0

    // Transformation Matrices
    AEMtx33 scale, trans, transform;

    //Draw background gradient blur
    if (pMeshGradientBlur)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 0.5f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.9f * fadeOut);
        AEGfxTextureSet(gradientBlur, 0, 0);

        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Scale(&scale, 1920 * gScaleX, 1080.0f * gScaleY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMeshGradientBlur, AE_GFX_MDM_TRIANGLES);


    }

    // Draw logo
    if (pMeshLogo)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.1f, 0.1f, 0.1f, 0.1f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f * fadeOut);
        AEGfxTextureSet(logoTexture, 0, 0);

        AEMtx33Trans(&trans, logoPosX - slideOffset, logoPosY);
        AEMtx33Scale(&scale, 331.0f * gScaleX, 228.0f * gScaleY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMeshLogo, AE_GFX_MDM_TRIANGLES);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    }

    //Exit Button
    if (pMeshExitButton) {

        if (!exitButton.hovered)
        {
            DrawButton(exitButton, pMeshExitButton, 68.0f, 39.0f, slideOffset);
        }
        else if (exitButton.hovered)
        {
            DrawButton(exitButton, pMeshExitButton_Selected, 89.0f, 60.0f, slideOffset);
        }
    }



    // Not new user, has save file
    if (hasSave)
    {
        // Draw Continue button
        if (!continueButton.hovered)
            DrawButton(continueButton, pMeshContinueButton, 190.0f, 41.0f, slideOffset);
        else
            DrawButton(continueButton, pMeshContinueButton_Selected, 211.0f, 61.0f, slideOffset);

        // Draw Profile button
        if (!profileButton.hovered)
            DrawButton(profileButton, pMeshProfileButton, 137.0f, 40.0f, slideOffset);
        else
            DrawButton(profileButton, pMeshProfileButton_Selected, 159.0f, 63.0f, slideOffset);
    }

    else
    {

        if (!newGameButton.hovered)
            DrawButton(newGameButton, pMeshNewGameButton, 234.0f, 35.0f, slideOffset);
        else
            DrawButton(newGameButton, pMeshNewGameButton_Selected, 256.0f, 58.0f, slideOffset);
    }

    // Draw New Game button
    // Draw Settings/Profile button
    // Draw Exit button
}