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

float gScaleX;
float gScaleY;

//animation variable
bool startScreenActive = true;   // Is the start screen still active?
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

static bool IsButtonHovered(Button& btn, float width, float height) {
    float x = btn.x;
    float y = btn.y;

    if (hasSave) {
        x = btn.x_save;
        y = btn.y_save;
    }

    return IsMouseOverRect(x, y, width * gScaleX, height * gScaleY);
}

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

static void DrawButton(Button& btn, AEGfxVertexList* mesh, f32 width, f32 height, float offset)
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

static AEGfxVertexList* createMesh()
{
    AEGfxMeshStart();
    // Use AEGfxTriAdd for consistency with other code in the project
    // First triangle: bottom-left, bottom-right, top-left
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f
    );
    // Second triangle: bottom-right, top-right, top-left
    AEGfxTriAdd(
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f
    );
    return AEGfxMeshEnd();
}

bool StartScreen_IsActive()
{
    return startScreenActive; // from your start screen cpp
}

void StartScreen_Load()
{
    // Load all textures and create all meshes - guarded so they only load once.
    // Called by GSM_Load so assets exist before the very first draw call.
    if (!logoTexture)
        logoTexture = AEGfxTextureLoad("Assets/StartScreen_Logo.png");
    if (!logoTexture)
        OutputDebugStringA("ERROR: Failed to load 'Assets/StartScreen_Logo.png'\n");

    if (!gradientBlur)
        gradientBlur = AEGfxTextureLoad("Assets/StartScreen_GradientBlur.png");
    if (!gradientBlur)
        OutputDebugStringA("ERROR: Failed to load 'Assets/StartScreen_GradientBlur.png'\n");

    if (!exitButton.normal)
        exitButton.normal = AEGfxTextureLoad("Assets/StartScreen_Exit.png");
    if (!exitButton.hover)
        exitButton.hover = AEGfxTextureLoad("Assets/StartScreen_Exit_Selected.png");
    if (!continueButton.normal)
        continueButton.normal = AEGfxTextureLoad("Assets/StartScreen_Continue.png");
    if (!continueButton.hover)
        continueButton.hover = AEGfxTextureLoad("Assets/StartScreen_Continue_Selected.png");
    if (!profileButton.normal)
        profileButton.normal = AEGfxTextureLoad("Assets/StartScreen_Profile.png");
    if (!profileButton.hover)
        profileButton.hover = AEGfxTextureLoad("Assets/StartScreen_Profile_Selected.png");
    if (!newGameButton.normal)
        newGameButton.normal = AEGfxTextureLoad("Assets/StartScreen_NewGameButton.png");
    if (!newGameButton.hover)
        newGameButton.hover = AEGfxTextureLoad("Assets/StartScreen_NewGameButton_Selected.png");

    if (!pMeshLogo)                   pMeshLogo = createMesh();
    if (!pMeshGradientBlur)           pMeshGradientBlur = createMesh();
    if (!pMeshNewGameButton)          pMeshNewGameButton = createMesh();
    if (!pMeshNewGameButton_Selected) pMeshNewGameButton_Selected = createMesh();
    if (!pMeshContinueButton)         pMeshContinueButton = createMesh();
    if (!pMeshContinueButton_Selected)pMeshContinueButton_Selected = createMesh();
    if (!pMeshProfileButton)          pMeshProfileButton = createMesh();
    if (!pMeshProfileButton_Selected) pMeshProfileButton_Selected = createMesh();
    if (!pMeshExitButton)             pMeshExitButton = createMesh();
    if (!pMeshExitButton_Selected)    pMeshExitButton_Selected = createMesh();
}

void StartScreen_Init()
{
    // Reset animation and button state so the screen plays fresh on each visit.
    startScreenActive = true;
    isExiting = false;
    exitAnimProgress = 0.0f;
    exitAnimFadeOut = 1.0f;

    hasSave = false; // TODO: replace with actual save detection

    // Initialize "Exit" button positions (always shown)
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
    if (!continueButton.normal) {
        OutputDebugStringA("ERROR: Failed to load 'Assets/StartScreen_Continue.png'\n");
    }
    continueButton.hover = AEGfxTextureLoad("Assets/StartScreen_Continue_Selected.png");
    if (!continueButton.hover) {
        OutputDebugStringA("ERROR: Failed to load 'Assets/StartScreen_Continue_Selected.png'\n");
    }
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
    if (!profileButton.normal) {
        OutputDebugStringA("ERROR: Failed to load 'Assets/StartScreen_Profile.png'\n");
    }
    profileButton.hover = AEGfxTextureLoad("Assets/StartScreen_Profile_Selected.png");
    if (!profileButton.hover) {
        OutputDebugStringA("ERROR: Failed to load 'Assets/StartScreen_Profile_Selected.png'\n");
    }
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
    if (!newGameButton.normal) {
        OutputDebugStringA("ERROR: Failed to load 'Assets/StartScreen_NewGameButton.png'\n");
    }
    newGameButton.hover = AEGfxTextureLoad("Assets/StartScreen_NewGameButton_Selected.png");
    if (!newGameButton.hover) {
        OutputDebugStringA("ERROR: Failed to load 'Assets/StartScreen_NewGameButton_Selected.png'\n");
    }
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
                // Continue game - go to Main Screen
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
                // New Game - go to Main Screen
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

    if (!startScreenActive) return; // don't draw after animation finished

    float slideOffset = exitAnimProgress * 3000.0f; // pixels to move left
    float fadeOut = exitAnimFadeOut; // fade out from 1 to 0

    // Transformation Matrices
    AEMtx33 scale, trans, transform;

    //Draw background gradient blur
    if (pMeshGradientBlur && gradientBlur)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);  // Reset color multiply
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.9f * fadeOut);
        AEGfxTextureSet(gradientBlur, 0, 0);

        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Scale(&scale, 1920.0f * gScaleX, 1080.0f * gScaleY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(pMeshGradientBlur, AE_GFX_MDM_TRIANGLES);
    }

    // Draw logo
    if (pMeshLogo && logoTexture)
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