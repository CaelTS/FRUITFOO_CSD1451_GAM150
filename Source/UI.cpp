#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS

#include "UI.h"
#include "Farm.h"
#include "Economy.h"
#include "AEEngine.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include "Inventory.h"
#include "Crate.h"
#include "Upgrades.h"
#include "Main.h"
#include "UIAudio.h"
#include "GameStateManager.h"
#include "StartScreen.h"
#include "Utilities.h"


extern AEGfxVertexList* g_pMeshFullScreen;
extern s8 fontId;

static bool menuOpen = false;
static bool popupOpen = false;
static bool seedsPopupOpen = false;
static bool cratePopupOpen = false;

static int activePopupIndex = -1;
static int selectedSeed = -1;
static int hoveredSeed = -1;   // purely for highlight
static int infoSeed = -1;         // which seed info panel is showing
static int hoveredPlotIndex = -1;
static int activePlotIndex = -1;

float UI_GetPlotCenterX();
float UI_GetPlotCenterY();

std::vector<PlotSlot> plotSlots;
static AEGfxTexture* plotSlotTexture = nullptr;

static AEGfxTexture* menuTexture = nullptr;
static AEGfxTexture* seedsTexture = nullptr;

static AEGfxTexture* inventoryIcon = nullptr;
static AEGfxTexture* inventoryBG = nullptr;
static AEGfxTexture* invFruit = nullptr;
static AEGfxTexture* invFruitHighlight = nullptr;
static AEGfxTexture* invSeed = nullptr;
static AEGfxTexture* invSeedHighlight = nullptr;
static AEGfxTexture* invSliderKnob = nullptr;
static AEGfxTexture* invSliderFill = nullptr;
static AEGfxTexture* appleIcon = nullptr;
static AEGfxTexture* pearIcon = nullptr;
static AEGfxTexture* bananaIcon = nullptr;
static AEGfxTexture* trashIcon = nullptr;
static AEGfxTexture* confirmBG = nullptr;
static AEGfxTexture* confirmYes = nullptr;
static AEGfxTexture* confirmNo = nullptr;

static AEGfxTexture* collectionIcon = nullptr;
static AEGfxTexture* collectionBG = nullptr;

static AEGfxTexture* settingsIcon = nullptr;
static AEGfxTexture* settingsBG = nullptr;
static AEGfxTexture* settingsOn = nullptr;
static AEGfxTexture* settingsOff = nullptr;

static AEGfxTexture* appleSeedIcon = nullptr;
static AEGfxTexture* pearSeedIcon = nullptr;
static AEGfxTexture* bananaSeedIcon = nullptr;
static AEGfxTexture* appleSeedInfo = nullptr;
static AEGfxTexture* leftArrowTexture = nullptr;
static AEGfxTexture* rightArrowTexture = nullptr;
static AEGfxTexture* plotSlotPearTexture = nullptr;
static AEGfxTexture* plotSlotBananaTexture = nullptr;


enum ButtonType
{
    BUTTON_INVENTORY,
    BUTTON_COLLECTION,
    BUTTON_SETTINGS
};


struct MenuButton
{
    float x, y;
    float width, height;
    bool isHovered;
    ButtonType type;
};

struct SeedData
{
    int cost;
    float growTime;
    int waterNeeded;
    const char* name;
};

static SeedData seedDatabase[SEED_COUNT] =
{
    { 10, 5.0f, 1, "Apple Seed" },
    { 15, 7.0f, 2, "Pear Seed" },   // <-- NEW
    { 20, 9.0f, 3, "Banana Seed" }  // optional
};

// The current seed page shown in the panel
static int currentSeedIndex = 0;

static std::vector<MenuButton> menuButtons;
static MenuButton plotPlusButton;

// Crate bins
extern std::vector<FruitBasket> gFruitBaskets;

static CrateLayoutConfig gCrateCfg{};
static bool gCrateCfgInitialized = false;


// ------------------------
// Upgrades
// ------------------------
//struct Upgrade { std::string name; int cost; bool purchased; };
//static std::vector<Upgrade> upgrades;
static int upgradesStartIndex = 0;
static const int MAX_VISIBLE_UPGRADES = 3;
static const float UPGRADES_PANEL_W = 450.0f;
static const float UPGRADES_PANEL_H = 280.0f;
static const float UPGRADES_PANEL_X = -530.0f;
static const float UPGRADES_PANEL_Y = -160.0f;

float ScaleX = 1600.0f / 1920.0f;
float ScaleY = 900.0f / 1080.0f;

//static const int MAX_VISIBLE_UPGRADES = 3;
static const float UP_BOX_X = -480.0f;   // center X of the Upgrades box
static const float UP_BOX_Y = -180.0f;   // center Y of the Upgrades box 
static const float UP_BOX_W = 450.0f;    // width  of the Upgrades box interior
static const float UP_BOX_H = 280.0f;    // height of the Upgrades box interior

// List layout inside upgrade box 
static const float UP_LIST_TOP_OFFSET = +80.0f; // distance from center Y to first row center (positive is up)
static const float UP_ROW_SPACING = 70.0f;  // vertical distance between row centers
static const float UP_ROW_W_MARGIN = 20.0f;  // left/right padding from box edge
static const float UP_ROW_H = 70.0f;  // row height (for hover highlight)

// Upgrades highlight tuning

static const float UP_HOVER_INSET_L = 15.0f;  // px inset from left edge of the row
static const float UP_HOVER_INSET_R = 12.0f;  // px inset from right edge of the row 
static const float UP_HOVER_INSET_TB = 8.0f;   // px inset from top/bottom (bigger = thinner bar)
static const float UP_HOVER_Y_NUDGE = 15.0f;  // px vertical nudge (negative = slightly lower)
static const float UP_HOVER_X_OFFSET = -50.0f;  // px horizontal nudge without moving text (negative = left)


// -------------------------
// Inventory panel
// -------------------------

// =========================
// Inventory header icons 
// =========================
enum InvTab { TAB_FRUITS = 0, TAB_SEEDS = 1 };
static InvTab gActiveInvTab = TAB_FRUITS;   // default tab

static const float INV_PANEL_W = 520.0f;
static const float INV_PANEL_H = 680.0f;

// ---- POSITION CONTROLS  ----
static const float INV_TAB_LEFT_PAD = 48.0f;   // px from panel LEFT edge to left icon's LEFT edge
static const float INV_TAB_TOP_PAD = 120.0f;  // px from panel TOP edge down to icon's TOP edge
static const float INV_TAB_SPACING_X = 96.0f;   // horizontal gap between Fruit and Seed

// Icon draw size 
static const float INV_FRUIT_W = 64.0f;
static const float INV_FRUIT_H = 64.0f;
static const float INV_FRUIT_HL_W = 72.0f;
static const float INV_FRUIT_HL_H = 72.0f;

static const float INV_SEED_W = 64.0f;
static const float INV_SEED_H = 64.0f;
static const float INV_SEED_HL_W = 72.0f;
static const float INV_SEED_HL_H = 72.0f;

// ---------- Slider label & value positions ----------
static const float INV_SLD_LABEL_LEFT_XPAD = -10.0f;  // + = move min label right
static const float INV_SLD_LABEL_RIGHT_XPAD = 6.0f;  // + = move max label left
static const float INV_SLD_LABEL_Y_OFFSET = -20.0f;  //-+ = move labels DOWN 

static const float INV_SLD_VALUE_Y_OFFSET = 30.0f;  // distance ABOVE the knob
static const float INV_SLD_VALUE_SCALE = 0.5f;   // size of the number above the knob

// ----------- Slider track position & size ---------
static const float INV_SLD_LEFT_PAD = 110.0f;
static const float INV_SLD_TOP_PAD = 590.0f;
static const float INV_SLD_TRACK_W = 230.0f;
static const float INV_SLD_FILL_H = 14.0f;

// Knob size
static const float INV_SLD_KNOB_W = 35.0f;
static const float INV_SLD_KNOB_H = 48.0f;

// Slider state (you already declared similar variables; if so, reuse)
static int  gInvSliderValue = 1;     // selection
static bool gInvSliderDragging = false; // dragging flag

// ---- Trough & bubble alignment knobs ----
static const float INV_SLD_FILL_Y_OFFSET = -10.0f;  // - moves fill DOWN, + moves UP  (start -4)
static const float INV_SLD_TRACK_CAP_INSET = -60.0f;  // px eaten from both ends to sit inside rounded caps (start 10)
static const float INV_SLD_FILL_THICKNESS = 10.0f;  // visual thickness of stripe (start 10)

// Value bubble (number) above the trough, centered on the knob's X
static const float INV_SLD_KNOB_ANCHOR_Y = 10.0f;
static const float INV_SLD_TEXT_FROM_PNG_CENTER_Y = 5.0f;
static const float INV_SLD_TEXT_X_OFFSET = -5.0f; // negative = left, positive = right

// ================= Inventory item selection =================
enum InvItem
{
    INV_ITEM_NONE = -1,
    INV_ITEM_APPLE = 0,
    INV_ITEM_PEAR = 1,
    INV_ITEM_BANANA = 2,
    INV_ITEM_APPLE_SEED = 3,
    INV_ITEM_PEAR_SEED = 4,
    INV_ITEM_BANANA_SEED = 5
};

static int gSelectedInvItem = INV_ITEM_NONE;
static bool gInvConfirmOpen = false;
static bool gHoverYes = false;
static bool gHoverNo = false;
static bool gHoverApple = false;
static bool gHoverTrash = false;

// Inventory item (apple / seed)
static const float INV_ITEM_X = -180.0f;
static const float INV_ITEM_Y = 90.0f;
static const float INV_ITEM_SIZE = 90.0f;


// Trash button
static const float INV_TRASH_X = 180.0f;
static const float INV_TRASH_Y = -250.0f;
static const float INV_TRASH_SIZE = 48.0f;

//======================= Crate Panel =======================
// Struct Button
struct CButton
{
    AEGfxVertexList* mesh;

    AEGfxTexture* availableTex;
    AEGfxTexture* unselectedTex;
    AEGfxTexture* selectedTex;

    float availableScaleX, availableScaleY;
    float unselectedScaleX, unselectedScaleY;
    float selectedScaleX, selectedScaleY;

    float x, y;

    bool isHovered;

};

struct RButton
{
    AEGfxVertexList* mesh;
    AEGfxTexture* normalTex;
    AEGfxTexture* altTex;
    float normalScaleX, normalScaleY;
    float altScaleX, altScaleY;
    float x, y;
    bool isHovered;
};


//Mesh
AEGfxVertexList* pMeshCratePanelBG = NULL;
AEGfxVertexList* pMeshCrateCost = NULL;
AEGfxVertexList* pMeshCrateInfo = NULL;

AEGfxVertexList* pMeshCrateSliderCircle = NULL;
AEGfxVertexList* pMeshCrateSliderFILL = NULL;

//Textures
AEGfxTexture* pCratePanelBGTexture = nullptr;
AEGfxTexture* pCrateCostTexture = nullptr;
AEGfxTexture* pCrateInfoATexture = nullptr;

AEGfxTexture* pCrateSliderCircleTexture = nullptr;
AEGfxTexture* pCrateSliderFillTexture = nullptr;

//Slider
static int InvAppleCount = 0; //to read current apples in inventory (for slider max)
static int crateAppleCount = 0; //to read current apples in crate (for slider max)

// Slider / selection
int sliderValue = 0;       // how many apples selected
int minValue = 0;          // always 0
int maxValue = InvAppleCount; // max is apples in inventory

static int CrateSliderValue = 0; //current slider value 
static bool CrateSliderDragging = false;

//Helper to create mesh
static AEGfxVertexList* CreateMesh()
{
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);

    return AEGfxMeshEnd();
}

//Helper function to create crate panel buttons
static CButton CreateCrateButton(AEGfxTexture* availableTex = nullptr, AEGfxTexture* unselectedTex = nullptr, AEGfxTexture* selectedTex = nullptr,
    float availableScaleX = 0, float availableScaleY = 0, float unselectedScaleX = 0, float unselectedScaleY = 0, float selectedScaleX = 0, float selectedScaleY = 0,
    float x = 0, float y = 0)
{
    CButton button{};
    button.mesh = CreateMesh();
    button.availableTex = availableTex;
    button.unselectedTex = unselectedTex;
    button.selectedTex = selectedTex;
    button.availableScaleX = availableScaleX;
    button.availableScaleY = availableScaleY;
    button.unselectedScaleX = unselectedScaleX;
    button.unselectedScaleY = unselectedScaleY;
    button.selectedScaleX = selectedScaleX;
    button.selectedScaleY = selectedScaleY;
    button.x = x;
    button.y = y;
    button.isHovered = false;
    return button;
}

static RButton CreateCrateRButton(AEGfxTexture* normalTex = nullptr, AEGfxTexture* altTex = nullptr,
    float normalScaleX = 0, float normalScaleY = 0, float altScaleX = 0, float altScaleY = 0,
    float x = 0, float y = 0)
{
    RButton button{};
    button.mesh = CreateMesh();
    button.normalTex = normalTex;
    button.altTex = altTex;
    button.normalScaleX = normalScaleX;
    button.normalScaleY = normalScaleY;
    button.altScaleX = altScaleX;
    button.altScaleY = altScaleY;
    button.x = x;
    button.y = y;
    button.isHovered = false;
    return button;
}

static AEGfxTexture* TextureLoad(const char* address) {
    return AEGfxTextureLoad(address);
}

CButton InvAppleButton;
CButton CrateAppleButton;
RButton Store_Crate;

static bool isButtonHovered(float btnX, float btnY, float btnW, float btnH)
{
    return IsMouseOverRect(btnX, btnY, btnW, btnH);
}

static bool isButtonClicked(float btnX, float btnY, float btnW, float btnH)
{
    return ClickedOnRect(btnX, btnY, btnW, btnH);
}

// --- Safe clamp ---
static inline float clampf(float v, float lo, float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

// Convert mouse to world coordinates (window: 1600x900)
static inline void GetWorldMouse(float& worldX, float& worldY)
{
    int mx, my;
    AEInputGetCursorPosition(&mx, &my);

    const float halfW = 1600.0f * 0.5f;
    const float halfH = 900.0f * 0.5f;

    worldX = static_cast<float>(mx) - halfW;
    worldY = halfH - static_cast<float>(my);
}


// Selected crate index (last clicked), -1 = none
static int gSelectedBasketIndex = -1;

// Return index of basket under mouse or -1 if none.
// Uses the same AABB test as IsMouseOverBasket to keep behaviour consistent.
static int GetFruitBasketIndexUnderMouse()
{
    // Use the same conversion helper
    int mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);
    float worldX = static_cast<float>(mouseX) - 800.0f; // Convert to world coordinates
    float worldY = 450.0f - static_cast<float>(mouseY); // Invert Y axis and convert

    // gFruitBaskets is declared extern at top of this file
    for (int i = 0; i < (int)gFruitBaskets.size(); ++i)
    {
        const FruitBasket& b = gFruitBaskets[i];
        if (worldX >= b.x - b.width * 0.5f &&
            worldX <= b.x + b.width * 0.5f &&
            worldY >= b.y - b.height * 0.5f &&
            worldY <= b.y + b.height * 0.5f)
        {

            return static_cast<int>(i);
        }
    }


    return -1;
}


// -------------------------
// Settings panel state
// -------------------------
bool gSoundEnabled = true;
bool gMusicEnabled = true;

static bool gHoverSoundToggle = false;
static bool gHoverMusicToggle = false;
//static bool gHoverSettingsExit = false; removed unused variable
//static bool gHoverSettingsClose = false; removed unused variable


void MainBGM_SetEnabled(bool enabled);


// -------------------------
// Settings layout
// -------------------------
static const float SET_PANEL_W = 962.0f;
static const float SET_PANEL_H = 609.0f;
static const float SET_PANEL_X = 200.0f;
static const float SET_PANEL_Y = 0.0f;

static const float SET_SOUND_Y = 70.0f;
static const float SET_MUSIC_Y = 15.0f;

static const float SET_TOGGLE_X = 130.0f;
static const float SET_TOGGLE_W = 235.0f;
static const float SET_TOGGLE_H = 57.0f;

static const float SET_CLOSE_X = 230.0f;
static const float SET_CLOSE_Y = 150.0f;
static const float SET_CLOSE_SIZE = 28.0f;

void UI_Init()
{
    menuTexture = AEGfxTextureLoad("Assets/MenuMockup.PNG");
    seedsTexture = AEGfxTextureLoad("Assets/SeedsPanel.png");

    inventoryIcon = AEGfxTextureLoad("Assets/Inventory.png");
    inventoryBG = AEGfxTextureLoad("Assets/InventoryPanelBG.png");
    invFruit = AEGfxTextureLoad("Assets/InvFruit.png");
    invFruitHighlight = AEGfxTextureLoad("Assets/InvFruitHighlight.png");
    invSeed = AEGfxTextureLoad("Assets/InvSeed.png");
    invSeedHighlight = AEGfxTextureLoad("Assets/InvSeedHighlight.png");
    invSliderKnob = AEGfxTextureLoad("Assets/InvSliderKnob.png");
    invSliderFill = AEGfxTextureLoad("Assets/InvSliderFill.png");
    appleIcon = AEGfxTextureLoad("Assets/Fruit_Apple.png");
    pearIcon = AEGfxTextureLoad("Assets/PlotPear.png");
    bananaIcon = AEGfxTextureLoad("Assets/PlotBanana.png");
    trashIcon = AEGfxTextureLoad("Assets/Trash.png");

    confirmBG = AEGfxTextureLoad("Assets/ConfirmBG.png");
    confirmYes = AEGfxTextureLoad("Assets/Yes.png");
    confirmNo = AEGfxTextureLoad("Assets/No.png");

    collectionIcon = AEGfxTextureLoad("Assets/Collection.png");
    collectionBG = AEGfxTextureLoad("Assets/collectionBG.png");

    settingsIcon = AEGfxTextureLoad("Assets/Settings.png");
    settingsBG = AEGfxTextureLoad("Assets/Settings_BG.png");
    settingsOn = AEGfxTextureLoad("Assets/Settings_ON.png");
    settingsOff = AEGfxTextureLoad("Assets/Settings_OFF.png");


    appleSeedIcon = AEGfxTextureLoad("Assets/AppleSeed.png");
    pearSeedIcon = AEGfxTextureLoad("Assets/PearSeed.png");
    bananaSeedIcon = AEGfxTextureLoad("Assets/BananaSeed.png");
    plotSlotPearTexture = AEGfxTextureLoad("Assets/PlotPear.png");
    plotSlotBananaTexture = AEGfxTextureLoad("Assets/PlotBanana.png");
    appleSeedInfo = AEGfxTextureLoad("Assets/AppleSeedInfo.png");
    plotSlotTexture = AEGfxTextureLoad("Assets/Plot1.png");
    leftArrowTexture = AEGfxTextureLoad("Assets/ArrowLeft.png");
    rightArrowTexture = AEGfxTextureLoad("Assets/ArrowRight.png");

    //=========================== Crate Panel ==================================

    pMeshCratePanelBG = CreateMesh();
    pCratePanelBGTexture = AEGfxTextureLoad("Assets/Crate_1_UI_BG.png");

    pMeshCrateCost = CreateMesh();
    pCrateCostTexture = AEGfxTextureLoad("Assets/Crate_1_UI_Cost_Apple.png");

    pMeshCrateInfo = CreateMesh();
    pCrateInfoATexture = AEGfxTextureLoad("Assets/Crate_1_UI_Info_Apple.png");

    pMeshCrateSliderCircle = CreateMesh();
    pCrateSliderCircleTexture = AEGfxTextureLoad("Assets/Crate_1_UI_SliderCircle.png");

    pMeshCrateSliderFILL = CreateMesh();
    pCrateSliderFillTexture = AEGfxTextureLoad("Assets/Crate_1_UI_Slider_Fill.png");

    InvAppleButton = CreateCrateButton(TextureLoad("Assets/Crate_1_UI_Available.png"),
        TextureLoad("Assets/Crate_1_UI_Unselected_Apple.png"),
        TextureLoad("Assets/Crate_1_UI_Selected_Apple.png"),
        122, 122, 141, 141, 158, 145,
        -140, -40);

    CrateAppleButton = CreateCrateButton(TextureLoad("Assets/Crate_1_UI_Available.png"),
        TextureLoad("Assets/Crate_1_UI_Unselected_Apple.png"),
        TextureLoad("Assets/Crate_1_UI_Selected_Apple.png"),
        122, 122, 141, 141, 158, 145,
        -150, 180);

    Store_Crate = CreateCrateRButton(TextureLoad("Assets/Crate_1_UI_Icon_Crate.png"),
        TextureLoad("Assets/Crate_1_UI_Icon_Inventory.png"),
        57, 54, 67, 71,
        180, -270);

    //=========================================================================


    // --- Menu Buttons ---
    menuButtons.clear();
    float menuCenterX = -530.0f;
    float buttonSize = 100.0f;
    float buttonSpacing = 140.0f;
    float buttonY = -360.0f;

    ButtonType buttonOrder[] = { BUTTON_INVENTORY, BUTTON_COLLECTION, BUTTON_SETTINGS };
    for (int i = 0; i < 3; ++i)
    {
        menuButtons.push_back({ menuCenterX + (i - 1) * buttonSpacing, buttonY, buttonSize, buttonSize, false, buttonOrder[i] });
    }


    //plot setup
    plotPlusButton.x = -630.0f;
    plotPlusButton.y = 150.0f;
    plotPlusButton.width = 120.0f;
    plotPlusButton.height = 120.0f;

    float slotSize = 120.0f;
    float spacing = 150.0f;

    int cols = 2;
    int rows = 2;

    // Center of the Plot panel
    float panelCenterX = -510.0f;   // tweak slightly if needed
    float panelCenterY = 150.0f;    // tweak slightly if needed
    float totalWidth = (cols - 1) * spacing + slotSize;
    float totalHeight = (rows - 1) * spacing + slotSize;

    float startX = panelCenterX - totalWidth * 0.5f + slotSize * 0.5f;
    float startY = panelCenterY + totalHeight * 0.5f - slotSize * 0.5f;

    plotSlots.clear();

    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < cols; col++)
        {
            PlotSlot slot = {};
            slot.x = startX + col * spacing;
            slot.y = startY - row * spacing;
            slot.width = slotSize;
            slot.height = slotSize;

            plotSlots.push_back(slot);
        }
    }

    // --- Upgrades ---
    /*upgrades = { {"Speed Boost", 100, false}, {"Crate Storage", 150, false}, {"Faster Growth", 200, false}, {"Quality Fruits", 300, false}, {"Stall Revamp", 500, false} };*/
    (void)Upgrades_GetList(); // pre-cache upgrade list

}

//======================== crate panel state ===================================//
int crateID = -1; //default
bool empty = false;
bool once = true;
bool isDragging = false;

float sliderX = -200.0f;
float sliderY = -265.0f;
float maxSlider = 132.0f, minSlider = -200.0f;

enum location { INVENTORY, CRATE, NIL };
location selectedLocation = NIL; // where the fruit is coming from (inventory) or going to (crate)

FruitType selectedFruit = NILL; // future proofing for more fruit types, but for now just apple
FruitType selectedInventoryFruitType = APPLE;


//how do i do fruit selected based on the slider value and apple count in inventory? also need to update slider max based on inventory changes (after adding/removing fruit from crate)
static int GetSliderFruitCount(location location)
{
    (void)location;
    if (location == NIL) return 0; // no location, no fruit

    float range = maxSlider - minSlider;
    int maxFruitCount = 0;

    if (location == INVENTORY) {
        if (selectedInventoryFruitType == APPLE) {
            maxFruitCount = GetAppleCount();
        }
        if (selectedInventoryFruitType == PEAR) {
          //  maxFruitCount = GetPearCount(); // Implement this function in Inventory when you add pears
        }
        if (selectedInventoryFruitType == BANANA) {
           // maxFruitCount = GetBananaCount(); // Implement this function in Inventory when you add bananas
        }
    }
    else if (location == CRATE) {
        maxFruitCount = Crate_GetFruitCount(crateID);
    }

    // Map sliderX to fruit count
    int fruitCount = static_cast<int>(((sliderX - minSlider) / range) * maxFruitCount);
    return fruitCount;
}

void UI_Input()
{
    if (AEInputCheckTriggered(AEVK_M))
        menuOpen = !menuOpen;

    if (menuOpen)
        UI_UpdateButtons();

    if (AEInputCheckTriggered(AEVK_P)) {
        float worldX, worldY;
        int mouseX, mouseY;
        AEInputGetCursorPosition(&mouseX, &mouseY);
        worldX = static_cast<float>(mouseX) - 800.0f; // Convert to world coordinates
        worldY = 450.0f - static_cast<float>(mouseY); // Invert Y axis and convert
        printf("Mouse world coordinates: (%.2f, %.2f)\n", worldX, worldY);
    }



    if (AEInputCheckTriggered(AEVK_C)) {
        Crate_AddFruit(0, 1);
        Crate_SetFruitType(0, 0); // Set to apple type for testing
        printf("Added apple to crate %d, now has %d apples:fruitID %d\n", 0, Crate_GetFruitCount(0), Crate_GetFruitType(0));
    }

    //================= Crate panel input ===================
    /*float crate0x = 143.0f, crate0y = -148.0f, crateW = 186 * gScaleX, crateH = 99 * gScaleY;*/
    if (cratePopupOpen == false && /*IsMouseOverRect(crate0x, crate0y, crateW, crateH)*/ GetFruitBasketIndexUnderMouse() >= 0 && AEInputCheckTriggered(AEVK_LBUTTON))
    {
        // toggle UI
        cratePopupOpen = true;
        crateID = GetFruitBasketIndexUnderMouse();
        printf("Clicked on basket %d,popup now %s\n", crateID, cratePopupOpen ? "OPEN" : "CLOSED");

        // Example: get fruit type id
        // int fruitId = gFruitBaskets[gSelectedBasketIndex].fruitType;
        // you can use fruitId to open crate UI for that fruit
    }

    GetFruitBasketIndexUnderMouse();
    (void)GetFruitBasketIndexUnderMouse();

    if (cratePopupOpen)
    {
        // Example: close crate UI if clicking outside
        float panelX = 0, panelY = 0, panelW = 617 * gScaleX, panelH = 871 * gScaleY;
        if (!IsMouseOverRect(panelX, panelY, panelW, panelH) && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            cratePopupOpen = false;
            printf("Clicked outside crate panel, closing it\n");
        }
    }

    if (cratePopupOpen)
    {
        // Handle crate UI interactions here, e.g.:
        // - Check if clicking on "Add to Crate" button
        // - Check if dragging slider
        // - etc.

        int typeInCrate = Crate_GetFruitType(crateID);
        int countInCrate = Crate_GetFruitCount(crateID);
        empty = countInCrate < 0;

        if (empty == false && once == true) {

            printf("Fruit in crate %d: %d, Count: %d\n", crateID, typeInCrate, countInCrate);
            once = false;

        }

        if (empty) {
            printf("Crate %d is empty!\n", crateID);
        }

        // Improved dragging: world-space, grab-offset, snap-to-track, clamp
        static float dragOffsetX = 0.0f;

        // knob visual size (match draw size)
        const float knobW = 43.0f;
        const float knobH = 65.0f;
        const float knobHalfW = knobW * 0.5f;
        const float knobHalfH = knobH * 0.5f;

        // Get mouse in world coords
        float worldMX, worldMY;
        GetWorldMouse(worldMX, worldMY);

        // Trigger: start drag when clicking knob, or snap+start if clicking the track
        if (AEInputCheckTriggered(AEVK_LBUTTON)) {

            if (isButtonClicked(InvAppleButton.x, InvAppleButton.y, InvAppleButton.availableScaleX, InvAppleButton.availableScaleY)) {
                selectedInventoryFruitType = APPLE;
                selectedLocation = INVENTORY;
                printf("Selected fruit: APPLE\n");
            }

            if (isButtonClicked(CrateAppleButton.x, CrateAppleButton.y, CrateAppleButton.availableScaleX, CrateAppleButton.availableScaleY)) {
                selectedFruit = APPLE;
                selectedLocation = CRATE;
                printf("Selected fruit: APPLE\n");
            }

            // Hit test knob
            const bool overKnob =
                worldMX >= sliderX - knobHalfW && worldMX <= sliderX + knobHalfW &&
                worldMY >= sliderY - knobHalfH && worldMY <= sliderY + knobHalfH;
            const bool sliderArea = isMouseOver4Corners(-220.00, -218.00, 219.00, -323.00);

            if (!sliderArea) {
                // Clicked outside slider area, ignore

            }
            else {
                if (overKnob)
                {
                    isDragging = true;
                    dragOffsetX = worldMX - sliderX; // preserve where on knob user grabbed
                }
                else
                {
                    // generous track hit area (vertical tolerance)
                    const float trackTolerance = 20.0f;
                    const bool overTrack =
                        worldMX >= minSlider && worldMX <= maxSlider &&
                        worldMY >= sliderY - trackTolerance && worldMY <= sliderY + trackTolerance;

                    if (overTrack)
                    {
                        // snap knob to click location and begin dragging
                        sliderX = clampf(worldMX, minSlider, maxSlider);
                        isDragging = true;
                        dragOffsetX = 0.0f;
                    }
                }
            }

        }

        // During drag, move knob with mouse, applying initial grab offset and clamping to track
        if (isDragging && AEInputCheckCurr(AEVK_LBUTTON)) {
            float targetX = worldMX - dragOffsetX;
            sliderX = clampf(targetX, minSlider, maxSlider);
        }

        // End drag on mouse release
        if (AEInputCheckReleased(AEVK_LBUTTON)) {
            const bool overInvBtn = isButtonHovered(InvAppleButton.x, InvAppleButton.y, InvAppleButton.availableScaleX, InvAppleButton.availableScaleY);
            const bool overCrateBtn = isButtonHovered(CrateAppleButton.x, CrateAppleButton.y, CrateAppleButton.availableScaleX, CrateAppleButton.availableScaleY);

            if (!overInvBtn && !overCrateBtn && !isMouseOver4Corners(-220.00, -218.00, 219.00, -323.00))
            {
                selectedLocation = NIL;
                sliderX = minSlider; // explicit deselect -> reset
            }
            else if (selectedLocation == NIL && isMouseOver4Corners(-220.00, -218.00, 219.00, -323.00))
            {
                // released over track but not buttons, keep selection but don't reset slider
                sliderX = minSlider; // reset slider but keep location so user can see where they dropped on track
            }
            else
            {
                // released over a button, keep selection and slider as is
            }

            isDragging = false;
        }

        sliderValue = GetSliderFruitCount(selectedLocation);


        if (isButtonClicked(Store_Crate.x, Store_Crate.y, Store_Crate.normalScaleX, Store_Crate.normalScaleY)) {

            // Store / confirm button: toggle dialog on click 
            if (isButtonClicked(Store_Crate.x, Store_Crate.y, Store_Crate.normalScaleX, Store_Crate.normalScaleY)) {
                gInvConfirmOpen = !gInvConfirmOpen;
                printf("Clicked store crate button, %s confirm dialog\n", gInvConfirmOpen ? "opening" : "closing");
            }

        }

        // Use the world mouse already read into worldMX/worldMY above
        auto Btn = [&](float x, float y) -> bool {
            return worldMX >= x - 40 && worldMX <= x + 40 &&
                worldMY >= y - 20 && worldMY <= y + 20;
            };

        if (gInvConfirmOpen) {

            // ================= Confirm buttons hover =================

            const float btnW = 80.0f * gScaleX;
            const float btnH = 40.0f * gScaleY;

            const float yesX = -60.0f;
            const float yesY = -40.0f;

            const float noX = 60.0f;
            const float noY = -40.0f;

            // use the world mouse coordinates read earlier (worldMX/worldMY) not worldX/worldY
            gHoverYes =
                worldMX >= yesX - btnW * 0.5f &&
                worldMX <= yesX + btnW * 0.5f &&
                worldMY >= yesY - btnH * 0.5f &&
                worldMY <= yesY + btnH * 0.5f;

            gHoverNo =
                worldMX >= noX - btnW * 0.5f &&
                worldMX <= noX + btnW * 0.5f &&
                worldMY >= noY - btnH * 0.5f &&
                worldMY <= noY + btnH * 0.5f;

            if (AEInputCheckTriggered(AEVK_LBUTTON)) {
                // For testing, just print action. Replace with actual logic to move fruit.
                if (Btn(-60, -40)) {

                    if (selectedLocation == INVENTORY && selectedInventoryFruitType == APPLE) {
                        // Move from inventory to crate
                        int availableToMove = GetAppleCount();
                        int toMove = std::min(sliderValue, availableToMove);
                        if (toMove > 0) {
                            Inventory_RemoveFruit(static_cast<u8>(toMove));
                            Crate_AddFruit(crateID, toMove);
                            Crate_SetFruitType(crateID, APPLE);
                            printf("Moved %d apples from inventory to crate %d\n", toMove, crateID);
                        }
                    }
                    else if (selectedLocation == CRATE && selectedFruit == APPLE) {
                        // Move from crate back to inventory
                        int availableToMove = Crate_GetFruitCount(crateID);
                        int toMove = std::min(sliderValue, availableToMove);
                        if (toMove > 0) {
                            Crate_RemoveFruitAmount(crateID, toMove);
                            Inventory_AddFruit(static_cast<u8>(toMove), APPLE);
                            printf("Moved %d apples from crate %d back to inventory\n", toMove, crateID);
                        }
                    }

                    else if (selectedLocation == INVENTORY && selectedInventoryFruitType == PEAR) {
                        int availableToMove = GetPearCount();
                        int toMove = std::min(sliderValue, availableToMove);
                        if (toMove > 0) {
                            Inventory_RemoveFruitTyped(static_cast<u8>(toMove), 1); // 1 = pear
                            Crate_AddFruit(crateID, toMove);
                            Crate_SetFruitType(crateID, PEAR);
                        }
                    }
                    else if (selectedLocation == CRATE && selectedFruit == PEAR) {
                        int availableToMove = Crate_GetFruitCount(crateID);
                        int toMove = std::min(sliderValue, availableToMove);
                        if (toMove > 0) {
                            Crate_RemoveFruitAmount(crateID, toMove);
                            Inventory_AddFruit(static_cast<u8>(toMove), 1); // 1 = pear
                        }
                    }
                    else if (selectedLocation == INVENTORY && selectedInventoryFruitType == BANANA) {
                        int availableToMove = GetBananaCount();
                        int toMove = std::min(sliderValue, availableToMove);
                        if (toMove > 0) {
                            Inventory_RemoveFruitTyped(static_cast<u8>(toMove), 2); // 2 = banana
                            Crate_AddFruit(crateID, toMove);
                            Crate_SetFruitType(crateID, BANANA);
                        }
                    }
                    else if (selectedLocation == CRATE && selectedFruit == BANANA) {
                        int availableToMove = Crate_GetFruitCount(crateID);
                        int toMove = std::min(sliderValue, availableToMove);
                        if (toMove > 0) {
                            Crate_RemoveFruitAmount(crateID, toMove);
                            Inventory_AddFruit(static_cast<u8>(toMove), 2); // 2 = banana
                        }
                    }

                    // close dialog after action
                    gInvConfirmOpen = false;
                }


                // NO
                else if (Btn(60, -40))
                {
                    gInvConfirmOpen = false;
                }

            }

        }

    }
}

void UI_UpdateButtons()
{
    int mx, my;
    AEInputGetCursorPosition(&mx, &my);

    float worldX = static_cast<float>(mx) - 800.0f;
    float worldY = 450.0f - static_cast<float>(my);

    bool clickThisFrame = AEInputCheckTriggered(AEVK_LBUTTON);
    bool clickConsumed = false;
    (void)clickConsumed; //unused variable

    // -------------------------------------------------
    // MENU BUTTONS
    // -------------------------------------------------
    for (auto& button : menuButtons)
    {
        button.isHovered =
            worldX >= button.x - button.width * 0.5f &&
            worldX <= button.x + button.width * 0.5f &&
            worldY >= button.y - button.height * 0.5f &&
            worldY <= button.y + button.height * 0.5f;

        if (button.isHovered && clickThisFrame)
        {
            clickConsumed = true;

            if (popupOpen && activePopupIndex == button.type)
            {
                popupOpen = false;
                activePopupIndex = -1;
            }
            else
            {
                popupOpen = true;
                activePopupIndex = button.type;
            }
        }

    }

    // -------------------------------------------------
    // PLOT "+" BUTTON (TOGGLE PANEL)
    // -------------------------------------------------
    plotPlusButton.isHovered =
        worldX >= plotPlusButton.x - plotPlusButton.width * 0.5f &&
        worldX <= plotPlusButton.x + plotPlusButton.width * 0.5f &&
        worldY >= plotPlusButton.y - plotPlusButton.height * 0.5f &&
        worldY <= plotPlusButton.y + plotPlusButton.height * 0.5f;

    if (plotPlusButton.isHovered && AEInputCheckTriggered(AEVK_LBUTTON))
    {
        seedsPopupOpen = !seedsPopupOpen;

        // If closing the panel, reset selection
        if (!seedsPopupOpen)
            selectedSeed = -1;
    }
    // -------------------------------------------------
    // SEED SELECTION (ONLY IF PANEL OPEN)
    // -------------------------------------------------
    if (seedsPopupOpen)
    {
        float panelX = -70.0f;
        float panelY = 20.0f;
        float seedY = panelY + 120.0f;
        float seedW = 100.0f;
        float seedH = 100.0f;

        // Close when clicking outside
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            bool clickInside =
                worldX >= panelX - 200.0f &&
                worldX <= panelX + 200.0f &&
                worldY >= panelY - 275.0f &&
                worldY <= panelY + 275.0f;

            if (!clickInside)
            {
                seedsPopupOpen = false;
                selectedSeed = -1;
            }
        }

        // ---- ARROW HIT DETECTION ----
        float arrowOffsetX = 150.0f;
        float arrowY = panelY + 150.0f;
        float arrowSize = 60.0f;   // generous hitbox

        bool overLeft =
            worldX >= (panelX - arrowOffsetX - arrowSize) &&
            worldX <= (panelX - arrowOffsetX + arrowSize) &&
            worldY >= (arrowY - arrowSize) &&
            worldY <= (arrowY + arrowSize);

        bool overRight =
            worldX >= (panelX + arrowOffsetX - arrowSize) &&
            worldX <= (panelX + arrowOffsetX + arrowSize) &&
            worldY >= (arrowY - arrowSize) &&
            worldY <= (arrowY + arrowSize);

        // Arrow clicks have highest priority — handle and bail early
        if (clickThisFrame)
        {
            if (overLeft)
            {
                currentSeedIndex--;
                if (currentSeedIndex < 0)
                    currentSeedIndex = SEED_COUNT - 1;

                clickConsumed = true;
                return;
            }

            if (overRight)
            {
                currentSeedIndex++;
                if (currentSeedIndex >= SEED_COUNT)
                    currentSeedIndex = 0;

                clickConsumed = true;
                return;
            }
        }


        bool overSeed =
            worldX >= panelX - seedW * 0.5f &&
            worldX <= panelX + seedW * 0.5f &&
            worldY >= seedY - seedH * 0.5f &&
            worldY <= seedY + seedH * 0.5f;

        // Hover detection
        if (overSeed)
            hoveredSeed = currentSeedIndex;
        else
            hoveredSeed = -1;
        if (overSeed && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            selectedSeed = currentSeedIndex;

            if (activePlotIndex != -1)
            {
                int plotToPlant = activePlotIndex;

                Farm_PlantSeed(plotToPlant, currentSeedIndex);
                u8 seedIndexSafe = 0;

                if (currentSeedIndex >= 0 && currentSeedIndex <= 255)
                    seedIndexSafe = static_cast<u8>(currentSeedIndex);

                Inventory_RemoveSeed(static_cast<u8>(1), seedIndexSafe);


                std::cout << "Planted seed type: " << currentSeedIndex
                    << " on plot: " << plotToPlant << "\n";

                seedsPopupOpen = false;
                activePlotIndex = -1;
                //selectedSeed = 1;
            }
        }
    }

    // -------------------------------------------------
    // PLOT SLOT HOVER
    // -------------------------------------------------

    hoveredPlotIndex = -1;

    for (int i = 0; i < (int)plotSlots.size(); i++)
    {
        PlotSlot& slot = plotSlots[i];

        bool isOver =
            worldX >= slot.x - slot.width * 0.5f &&
            worldX <= slot.x + slot.width * 0.5f &&
            worldY >= slot.y - slot.height * 0.5f &&
            worldY <= slot.y + slot.height * 0.5f;

        if (isOver)
        {
            hoveredPlotIndex = static_cast<int>(i);

            if (Farm_IsPlotLocked(static_cast<int>(i)))
                break;

            if (AEInputCheckTriggered(AEVK_LBUTTON))
            {
                if (seedsPopupOpen && activePlotIndex == i)
                {
                    seedsPopupOpen = false;
                    selectedSeed = -1;
                    activePlotIndex = -1;
                }
                else
                {
                    seedsPopupOpen = true;
                    activePlotIndex = static_cast<int>(i);
                    selectedSeed = currentSeedIndex;
                }
            }

            break;  // stop checking other slots
        }
    }

    // DELETE SEED BUTTON (CLICK LOGIC ONLY)
    for (int i = 0; i < (int)plotSlots.size(); i++)
    {
        if (!Farm_IsPlotPlanted(static_cast<int>(i)))
            continue;

        float xSize = 25.0f;

        float offsetX = -45.0f;   // SAME as Farm_Render
        float offsetY = 45.0f;

        float xPos = plotSlots[i].x + offsetX;
        float yPos = plotSlots[i].y + offsetY;

        bool overDelete =
            worldX >= xPos - xSize * 0.5f &&
            worldX <= xPos + xSize * 0.5f &&
            worldY >= yPos - xSize * 0.5f &&
            worldY <= yPos + xSize * 0.5f;

        if (overDelete && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            Farm_ClearPlot(static_cast<int>(i));
            break;
        }
    }
    float upgradesPanelW = UPGRADES_PANEL_W;

    float panelX = UPGRADES_PANEL_X;
    float panelY = UPGRADES_PANEL_Y;

    float spacingUp = 70.0f;
    float startYUp = panelY + 60.0f;

    auto& upgrades = Upgrades_GetList();

    int shownUp = 0;
    for (int i = upgradesStartIndex; i < (int)upgrades.size() && shownUp < MAX_VISIBLE_UPGRADES; ++i)
    {
        auto& u = upgrades[i];
        if (u.purchased) continue;  // skip purchased upgrades

        float y = startYUp - shownUp * spacingUp;

        float boxW = upgradesPanelW - 40.0f;
        float boxH = 50.0f;

        bool over =
            worldX >= panelX - boxW * 0.5f &&
            worldX <= panelX + boxW * 0.5f &&
            worldY >= y - boxH * 0.5f &&
            worldY <= y + boxH * 0.5f;

        if (over && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            // Check affordability using current money from Economy
            if (Upgrades_CanPurchase(u, Economy_GetTotalMoney()))
            {
                Upgrades_Purchase(u.id);
            }
            else
            {
                // optional feedback: not enough funds (console for now)
                std::cout << "Cannot afford upgrade " << static_cast<int>(u.id)
                    << " (cost=" << u.cost << ", money=" << Economy_GetTotalMoney() << ")\n";
            }
            break;
        }

        ++shownUp;
    }

    // ================= Inventory header icons input =================
    if (popupOpen && activePopupIndex == BUTTON_INVENTORY)
    {
        // Panel is drawn at (0,0) with size INV_PANEL_W x INV_PANEL_H in UI_Draw()
        const float invpanelX = 0.0f, invpanelY = 0.0f;
        const float leftEdge = invpanelX - INV_PANEL_W * 0.5f;
        const float topEdge = invpanelY + INV_PANEL_H * 0.5f;

        // Use your normal icon size as the "slot" (stable hitbox)
        const float slotW = INV_FRUIT_W;
        const float slotH = INV_FRUIT_H;

        // Compute centers from the same paddings you use in UI_Draw()
        const float fruitCx = leftEdge + INV_TAB_LEFT_PAD + slotW * 0.5f;
        const float fruitCy = topEdge - INV_TAB_TOP_PAD - slotH * 0.5f;

        const float seedCx = fruitCx + INV_TAB_SPACING_X;
        const float seedCy = fruitCy;

        const float hw = slotW * 0.5f;
        const float hh = slotH * 0.5f;

        auto PtIn = [&](float px, float py, float cx, float cy, float halfW, float halfH) -> bool {
            return px >= cx - halfW && px <= cx + halfW &&
                py >= cy - halfH && py <= cy + halfH;
            };

        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            if (PtIn(worldX, worldY, fruitCx, fruitCy, hw, hh))
            {
                gActiveInvTab = TAB_FRUITS;
            }
            else if (PtIn(worldX, worldY, seedCx, seedCy, hw, hh))
            {
                gActiveInvTab = TAB_SEEDS;
            }
        }
    }

    // ================= Inventory slider input =================
    if (popupOpen && activePopupIndex == BUTTON_INVENTORY)
    {
        // Panel edges
        const float panelCenterX = 0.0f, panelCenterY = 0.0f;
        const float leftEdge = panelCenterX - INV_PANEL_W * 0.5f;
        const float topEdge = panelCenterY + INV_PANEL_H * 0.5f;

        // Base endpoints from paddings
        const float baseX0 = leftEdge + INV_SLD_LEFT_PAD;
        const float baseX1 = baseX0 + INV_SLD_TRACK_W;

        // Inside rounded caps
        const float trackX0 = baseX0 + INV_SLD_TRACK_CAP_INSET;
        const float trackX1 = baseX1 - INV_SLD_TRACK_CAP_INSET;

        // Trough centerline (+ vertical nudge)
        const float trackY = (topEdge - INV_SLD_TOP_PAD) + INV_SLD_FILL_Y_OFFSET;

        // Active tab stock -> range
        const int curCount = (gActiveInvTab == TAB_FRUITS)
            ? (gSelectedInvItem == INV_ITEM_PEAR ? GetPearCount()
                : gSelectedInvItem == INV_ITEM_BANANA ? GetBananaCount()
                : GetAppleCount())
            : (gSelectedInvItem == INV_ITEM_PEAR_SEED ? Inventory_GetSeedCount(SEED_PEAR)
                : gSelectedInvItem == INV_ITEM_BANANA_SEED ? Inventory_GetSeedCount(SEED_BANANA)
                : Inventory_GetSeedCount(SEED_APPLE));



        const int minVal = (curCount > 0) ? 1 : 0;
        const int maxVal = curCount;

        gInvSliderValue = (std::max)(minVal, (std::min)(maxVal, gInvSliderValue));

        // Current knob (for hit-tests)
        float t = (maxVal > minVal) ? float(gInvSliderValue - minVal) / float(maxVal - minVal) : 0.0f;
        float knobX = trackX0 + t * (trackX1 - trackX0);
        float knobY = trackY;

        auto PtIn = [](float px, float py, float cx, float cy, float hw, float hh)->bool {
            return (px >= cx - hw) && (px <= cx + hw) && (py >= cy - hh) && (py <= cy + hh);
            };

        // Generous hitboxes that match the visible stripe
        const float knobHW = (std::max)(INV_SLD_KNOB_W * 0.6f, 18.0f);
        const float knobHH = (std::max)(INV_SLD_KNOB_H * 0.6f, 18.0f);
        const float trackHW = (trackX1 - trackX0) * 0.5f;
        const float trackHH = (std::max)(INV_SLD_FILL_THICKNESS * 0.8f, 10.0f);

        const bool overKnob = PtIn(worldX, worldY, knobX, knobY, knobHW, knobHH);
        const bool overTrack = PtIn(worldX, worldY, 0.5f * (trackX0 + trackX1), trackY, trackHW, trackHH);

        // AE input: held vs pressed
        const bool mouseDown = AEInputCheckCurr(AEVK_LBUTTON) != 0;
        const bool mousePress = AEInputCheckTriggered(AEVK_LBUTTON) != 0;

        // Snap on press if clicking the track, or start drag on knob
        if (mousePress && overTrack && maxVal >= minVal && (trackX1 > trackX0))
        {
            float clampedX = (worldX < trackX0) ? trackX0 : ((worldX > trackX1) ? trackX1 : worldX);
            float tt = (clampedX - trackX0) / (trackX1 - trackX0);
            int newVal = (int)std::round((float)minVal + tt * (float)(maxVal - minVal));
            gInvSliderValue = (std::max)(minVal, (std::min)(maxVal, newVal));
            gInvSliderDragging = true;
        }
        else if (mousePress && overKnob)
        {
            gInvSliderDragging = true;
        }

        if (!mouseDown) gInvSliderDragging = false;

        if (gInvSliderDragging && maxVal >= minVal && (trackX1 > trackX0))
        {
            float clampedX = (worldX < trackX0) ? trackX0 : ((worldX > trackX1) ? trackX1 : worldX);
            float tt = (clampedX - trackX0) / (trackX1 - trackX0);
            int newVal = (int)std::round((float)minVal + tt * (float)(maxVal - minVal));
            gInvSliderValue = (std::max)(minVal, (std::min)(maxVal, newVal));
        }
    }

    // ================= Inventory item click =================
    if (popupOpen && activePopupIndex == BUTTON_INVENTORY)
    {

        const float invpanelX = 0.0f;
        const float invpanelY = 0.0f;
        const float spacing = 100.0f; // must match draw section

        float itemX = invpanelX + INV_ITEM_X;
        float itemY = invpanelY + INV_ITEM_Y;

        auto HitTest = [&](float cx, float cy) -> bool {
            return worldX >= cx - INV_ITEM_SIZE * 0.5f &&
                worldX <= cx + INV_ITEM_SIZE * 0.5f &&
                worldY >= cy - INV_ITEM_SIZE * 0.5f &&
                worldY <= cy + INV_ITEM_SIZE * 0.5f;
            };

        gHoverApple =
            worldX >= itemX - INV_ITEM_SIZE * 0.5f &&
            worldX <= itemX + INV_ITEM_SIZE * 0.5f &&
            worldY >= itemY - INV_ITEM_SIZE * 0.5f &&
            worldY <= itemY + INV_ITEM_SIZE * 0.5f;

        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            if (gActiveInvTab == TAB_FRUITS)
            {
                if (HitTest(itemX, itemY)) { gSelectedInvItem = INV_ITEM_APPLE;  gInvSliderValue = 1; }
                else if (HitTest(itemX + spacing, itemY)) { gSelectedInvItem = INV_ITEM_PEAR;   gInvSliderValue = 1; }
                else if (HitTest(itemX + spacing * 2, itemY)) { gSelectedInvItem = INV_ITEM_BANANA; gInvSliderValue = 1; }
            }
            else // TAB_SEEDS
            {
                if (HitTest(itemX, itemY)) { gSelectedInvItem = INV_ITEM_APPLE_SEED;  gInvSliderValue = 1; }
                else if (HitTest(itemX + spacing, itemY)) { gSelectedInvItem = INV_ITEM_PEAR_SEED;   gInvSliderValue = 1; }
                else if (HitTest(itemX + spacing * 2, itemY)) { gSelectedInvItem = INV_ITEM_BANANA_SEED; gInvSliderValue = 1; }
            }

        }

    }
    // Trash
    if (popupOpen && activePopupIndex == BUTTON_INVENTORY && gSelectedInvItem != INV_ITEM_NONE)
    {
        const float invpanelX = 0.0f;
        const float invpanelY = 0.0f;

        float tx = invpanelX + INV_TRASH_X;
        float ty = invpanelY + INV_TRASH_Y;

        gHoverTrash =
            worldX >= tx - INV_TRASH_SIZE * 0.5f &&
            worldX <= tx + INV_TRASH_SIZE * 0.5f &&
            worldY >= ty - INV_TRASH_SIZE * 0.5f &&
            worldY <= ty + INV_TRASH_SIZE * 0.5f;

        if (gHoverTrash && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            if (gSelectedInvItem != INV_ITEM_NONE)
                gInvConfirmOpen = true;
        }
    }

    //Confirmation
    if (gInvConfirmOpen)
    {
        auto Btn = [&](float x, float y)
            {
                return worldX >= x - 40 && worldX <= x + 40 &&
                    worldY >= y - 20 && worldY <= y + 20;
            };

        // YES
        if (Btn(-60, -40) && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            if (gSelectedInvItem == INV_ITEM_APPLE)
                Inventory_RemoveFruitTyped(static_cast<u8>(gInvSliderValue), 0); // 0 = Apple
            else if (gSelectedInvItem == INV_ITEM_PEAR)
                Inventory_RemoveFruitTyped(static_cast<u8>(gInvSliderValue), 1); // 1 = Pear
            else if (gSelectedInvItem == INV_ITEM_BANANA)
                Inventory_RemoveFruitTyped(static_cast<u8>(gInvSliderValue), 2); // 2 = Banana

            else
            {
                // Seeds: map item enum to seed type
                u8 seedType = 0;
                if (gSelectedInvItem == INV_ITEM_APPLE_SEED)  seedType = SEED_APPLE;
                else if (gSelectedInvItem == INV_ITEM_PEAR_SEED)   seedType = SEED_PEAR;
                else if (gSelectedInvItem == INV_ITEM_BANANA_SEED) seedType = SEED_BANANA;

                Inventory_RemoveSeed(
                    static_cast<u8>(gInvSliderValue < 0 ? 0 : (gInvSliderValue > 255 ? 255 : gInvSliderValue)),
                    seedType

                );

                gInvConfirmOpen = false;
                gSelectedInvItem = INV_ITEM_NONE;
            }

            // NO
            if (Btn(60, -40) && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                gInvConfirmOpen = false;
            }

            // ================= Confirm buttons hover =================

            const float btnW = 80.0f;
            const float btnH = 40.0f;

            const float yesX = -60.0f;
            const float yesY = -40.0f;

            const float noX = 60.0f;
            const float noY = -40.0f;

            gHoverYes =
                worldX >= yesX - btnW * 0.5f &&
                worldX <= yesX + btnW * 0.5f &&
                worldY >= yesY - btnH * 0.5f &&
                worldY <= yesY + btnH * 0.5f;

            gHoverNo =
                worldX >= noX - btnW * 0.5f &&
                worldX <= noX + btnW * 0.5f &&
                worldY >= noY - btnH * 0.5f &&
                worldY <= noY + btnH * 0.5f;

        }

        // ================= Settings input =================
        if (popupOpen && activePopupIndex == BUTTON_SETTINGS)
        {
            // SOUND toggle
            gHoverSoundToggle =
                worldX >= (SET_PANEL_X + SET_TOGGLE_X) - SET_TOGGLE_W * 0.5f &&
                worldX <= (SET_PANEL_X + SET_TOGGLE_X) + SET_TOGGLE_W * 0.5f &&
                worldY >= (SET_PANEL_Y + SET_SOUND_Y + 45.0f) - SET_TOGGLE_H * 0.5f &&
                worldY <= (SET_PANEL_Y + SET_SOUND_Y + 45.0f) + SET_TOGGLE_H * 0.5f;

            if (gHoverSoundToggle && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                gSoundEnabled = !gSoundEnabled;
                UIAudio_EnableSFX(gSoundEnabled);
                UIAudio_PlayToggle();
            }

            // MUSIC toggle
            gHoverMusicToggle =
                worldX >= (SET_PANEL_X + SET_TOGGLE_X) - SET_TOGGLE_W * 0.5f &&
                worldX <= (SET_PANEL_X + SET_TOGGLE_X) + SET_TOGGLE_W * 0.5f &&
                worldY >= (SET_PANEL_Y + SET_MUSIC_Y + 5.0f) - SET_TOGGLE_H * 0.5f &&
                worldY <= (SET_PANEL_Y + SET_MUSIC_Y + 5.0f) + SET_TOGGLE_H * 0.5f;

            if (gHoverMusicToggle && AEInputCheckTriggered(AEVK_LBUTTON))
            {
                gMusicEnabled = !gMusicEnabled;
                UIAudio_SetMusicEnabled(gMusicEnabled);
                MainBGM_SetEnabled(gMusicEnabled);
                UIAudio_PlayToggle();
            }
        }

    }
    // -------------------------------------------------
        // CLOSE POPUP WHEN CLICKING OUTSIDE
        // -------------------------------------------------
    if (popupOpen && AEInputCheckTriggered(AEVK_LBUTTON) && !clickConsumed)
    {
        bool clickInside = false;

        switch (activePopupIndex)
        {
        case BUTTON_INVENTORY:
        {
            clickInside =
                worldX >= -INV_PANEL_W * 0.5f &&
                worldX <= INV_PANEL_W * 0.5f &&
                worldY >= -INV_PANEL_H * 0.5f &&
                worldY <= INV_PANEL_H * 0.5f;
            break;
        }

        case BUTTON_COLLECTION:
        {
            const float panelCenterX = 180.0f;
            const float panelCenterY = 0.0f;

            const float panelHalfW = 800.0f * 0.5f;
            const float panelHalfH = 600.0f * 0.4f;

            clickInside =
                worldX >= panelCenterX - panelHalfW &&
                worldX <= panelCenterX + panelHalfW &&
                worldY >= panelCenterY - panelHalfH &&
                worldY <= panelCenterY + panelHalfH;

            break;
        }
        case BUTTON_SETTINGS:
        {
            float panelHalfW = (SET_PANEL_W * ScaleX) * 0.5f;
            float panelHalfH = (SET_PANEL_H * ScaleY) * 0.46f;

            clickInside =
                worldX >= SET_PANEL_X - panelHalfW &&
                worldX <= SET_PANEL_X + panelHalfW &&
                worldY >= SET_PANEL_Y - panelHalfH &&
                worldY <= SET_PANEL_Y + panelHalfH;
            break;
        }
        }

        if (clickInside)
        {
            clickConsumed = true;
        }

        if (!clickInside)
        {
            popupOpen = false;
            activePopupIndex = -1;
        }
    }
}

bool isHovered_apple = false;
bool isSelected_apple = false;
int shiftUP = 10;
int price_apple = 0;


void UI_Draw()
{
    AEMtx33 scale, trans, transform;

    //============== Crate Panel =====================
    if (cratePopupOpen) {
        // Draw crate background
        // Use nearest sampling for crisp UI
        if (pMeshCratePanelBG) {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetTransparency(1.0f);

            AEGfxSetTextureMode(AE_GFX_TM_PRECISE);
            AEGfxTextureSet(pCratePanelBGTexture, 0, 0);  // Set the texture

            AEMtx33Trans(&trans, 0, 0);  // Apply position transformation
            AEMtx33Scale(&scale, 617 * gScaleX, 871 * gScaleY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
            AEGfxMeshDraw(pMeshCratePanelBG, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
        }
        if (pMeshCrateCost) {
            float x = 130.0f;
            float y = 180.0f;
            // Draw crate cost
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetTransparency(1.0f);
            AEGfxSetTextureMode(AE_GFX_TM_PRECISE);
            AEGfxTextureSet(pCrateCostTexture, 0, 0);  // Set the texture
            AEMtx33Trans(&trans, x, y);  // Apply position transformation
            AEMtx33Scale(&scale, 169 * gScaleX, 53 * gScaleY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
            AEGfxMeshDraw(pMeshCrateCost, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad

            // --- Gold number text ---
            if (selectedFruit == APPLE && Crate_GetFruitCount(crateID) != 0) {
                price_apple = Economy_GetBasePrice(APPLE);
            }

            if (Crate_GetFruitCount(crateID) == 0) {
                price_apple = 0;
            }

            char appleText[32];
            sprintf_s(appleText, "%d", price_apple);


            //float x = -530.0f / 800.0f;  // normalize X by 800.0f
            //float y = 350.0f / 450.0f;  // normalize Y by 450.0f
            float xOf = x;
            float yOf = static_cast<float>(y) - 9.0f;
            const float halfW = 800.0f;
            const float halfH = 450.0f;
            float xNorm = xOf / halfW;
            float yNorm = yOf / halfH;

            float r = 60.0f / 255.0f;
            float g = 68.0f / 255.0f;
            float b = 92.0f / 255.0f;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);

            AEGfxPrint(fontId, appleText, xNorm, yNorm, 1.0f, r, g, b, 1);
        }

        if (CrateAppleButton.mesh && Crate_GetFruitType(crateID) == 0) {
            AEGfxTexture* textureApple = nullptr;
            float scaleAppleX = 0.0f;
            float scaleAppleY = 0.0f;
            float AppleX = CrateAppleButton.x, AppleY = CrateAppleButton.y;

            if (Crate_GetFruitCount(crateID) > 0) {
                textureApple = CrateAppleButton.unselectedTex;
                scaleAppleX = CrateAppleButton.unselectedScaleX;
                scaleAppleY = CrateAppleButton.unselectedScaleY;
                AppleX = CrateAppleButton.x + 8.1f;
                AppleY = CrateAppleButton.y - 7.8f;


                if (AEInputCheckTriggered(AEVK_LBUTTON) && isButtonHovered(CrateAppleButton.x, CrateAppleButton.y, CrateAppleButton.unselectedScaleX, CrateAppleButton.unselectedScaleY)) {
                    isHovered_apple = true;

                }
                if (AEInputCheckTriggered(AEVK_LBUTTON) && !isButtonHovered(CrateAppleButton.x, CrateAppleButton.y, CrateAppleButton.unselectedScaleX, CrateAppleButton.unselectedScaleY)
                    && !isMouseOver4Corners(-220.00, -218.00, 219.00, -323.00)) {
                    isHovered_apple = false;
                }

                if (isHovered_apple) {
                    textureApple = CrateAppleButton.selectedTex;
                    scaleAppleX = CrateAppleButton.selectedScaleX;
                    scaleAppleY = CrateAppleButton.selectedScaleY;
                }

            }

            if (Crate_GetFruitCount(crateID) <= 0) {
                textureApple = CrateAppleButton.availableTex;
                scaleAppleX = CrateAppleButton.availableScaleX;
                scaleAppleY = CrateAppleButton.availableScaleY;
            }

            // Draw "Apple" button in crate UI
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetTransparency(1.0f);
            AEGfxSetTextureMode(AE_GFX_TM_PRECISE);
            AEGfxTextureSet(textureApple, 0, 0);  // Set the texture
            AEMtx33Trans(&trans, AppleX, AppleY);  // Apply position transformation
            AEMtx33Scale(&scale, scaleAppleX * gScaleX, scaleAppleY * gScaleY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
            AEGfxMeshDraw(CrateAppleButton.mesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad

            if (textureApple == CrateAppleButton.unselectedTex || textureApple == CrateAppleButton.selectedTex) {

                // --- Quantity number text ---
                int qty_apple = Crate_GetFruitCount(crateID);

                char appleQTYText[32];
                sprintf_s(appleQTYText, "%d", qty_apple);


                //float x = -530.0f / 800.0f;  // normalize X by 800.0f
                //float y = 350.0f / 450.0f;  // normalize Y by 450.0f
                float xOf = 0.0f;
                if (qty_apple <= 9) {
                    xOf = -112;
                    if (textureApple == CrateAppleButton.selectedTex) {
                        xOf = -106;
                    }
                }
                else {
                    xOf = -118;
                    if (textureApple == CrateAppleButton.selectedTex) {
                        xOf = -112;
                    }
                }
                float yOf = 131 - 2;
                const float halfW = 800.0f;
                const float halfH = 450.0f;
                float xNorm = xOf / halfW;
                float yNorm = yOf / halfH;

                float r = 243.0f / 255.0f;
                float g = 196.0f / 255.0f;
                float b = 115.0f / 255.0f;

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1, 1, 1, 1);

                AEGfxPrint(fontId, appleQTYText, xNorm, yNorm, 1.0f, r, g, b, 1);
            }

            if (InvAppleButton.mesh) {
                textureApple = nullptr;
                scaleAppleX = 0;
                scaleAppleY = 0;
                AppleX = InvAppleButton.x;
                AppleY = InvAppleButton.y;

                if (AEInputCheckTriggered(AEVK_LBUTTON) && isButtonHovered(InvAppleButton.x, InvAppleButton.y, InvAppleButton.unselectedScaleX, InvAppleButton.unselectedScaleY)) {
                    isSelected_apple = true;
                }

                if (AEInputCheckTriggered(AEVK_LBUTTON) && !isButtonHovered(InvAppleButton.x, InvAppleButton.y, InvAppleButton.unselectedScaleX, InvAppleButton.unselectedScaleY)
                    && !isMouseOver4Corners(-220.00, -218.00, 219.00, -323.00)) {
                    isSelected_apple = false;
                    printf("Deselected apple\n");
                }

                if (isSelected_apple) {
                    textureApple = InvAppleButton.selectedTex;
                    scaleAppleX = InvAppleButton.selectedScaleX;
                    scaleAppleY = InvAppleButton.selectedScaleY;
                }
                else {
                    textureApple = InvAppleButton.unselectedTex;
                    scaleAppleX = InvAppleButton.unselectedScaleX;
                    scaleAppleY = InvAppleButton.unselectedScaleY;
                }

                // Draw "Apple" button in inventory UI (for reference, not interactive here)
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxSetTransparency(1.0f);
                AEGfxSetTextureMode(AE_GFX_TM_PRECISE);
                AEGfxTextureSet(textureApple, 0, 0);  // Set the texture
                AEMtx33Trans(&trans, AppleX, AppleY + shiftUP);  // Apply position transformation
                AEMtx33Scale(&scale, scaleAppleX * gScaleX, scaleAppleY * gScaleY);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(InvAppleButton.mesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad

                // --- Quantity number text ---
                int qty_apple = GetAppleCount();


                char appleQTYText[32];
                sprintf_s(appleQTYText, "%d", qty_apple);


                //float x = -530.0f / 800.0f;  // normalize X by 800.0f
                //float y = 350.0f / 450.0f;  // normalize Y by 450.0f
                float xOf = 0.0f;
                if (qty_apple <= 9) {
                    xOf = -111;
                    if (textureApple == InvAppleButton.selectedTex) {
                        xOf = -105;
                    }
                }
                else {
                    xOf = -117;
                    if (textureApple == InvAppleButton.selectedTex) {
                        xOf = -111;
                    }
                }
                float yOf = -83.0f + shiftUP;
                const float halfW = 800.0f;
                const float halfH = 450.0f;
                float xNorm = xOf / halfW;
                float yNorm = yOf / halfH;

                float r = 243.0f / 255.0f;
                float g = 196.0f / 255.0f;
                float b = 115.0f / 255.0f;

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1, 1, 1, 1);

                AEGfxPrint(fontId, appleQTYText, xNorm, yNorm, 1.0f, r, g, b, 1);
            }

            if (pMeshCrateInfo) {
                if (Crate_GetFruitType(crateID) == APPLE) {
                    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                    AEGfxSetColorToMultiply(1, 1, 1, 1);
                    AEGfxTextureSet(pCrateInfoATexture, 0, 0);  // Set the texture
                    AEMtx33Trans(&trans, 0, -155);  // Apply position transformation
                    AEMtx33Scale(&scale, 530 * gScaleX, 117 * gScaleY);
                    AEMtx33Concat(&transform, &trans, &scale);
                    AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                    AEGfxMeshDraw(pMeshCrateInfo, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad

                    // --- Gold number text ---

                    if (selectedInventoryFruitType == APPLE) {
                        price_apple = Economy_GetBasePrice(APPLE);
                    }
                    char appleText[32];
                    sprintf_s(appleText, "%d", price_apple);



                    float xOf = 130;
                    float yOf = -161.5;
                    const float halfW = 800.0f;
                    const float halfH = 450.0f;
                    float xNorm = xOf / halfW;
                    float yNorm = yOf / halfH;

                    float r = 60.0f / 255.0f;
                    float g = 68.0f / 255.0f;
                    float b = 92.0f / 255.0f;

                    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                    AEGfxSetColorToMultiply(1, 1, 1, 1);

                    AEGfxPrint(fontId, appleText, xNorm, yNorm, 1.0f, r, g, b, 1);
                }
            }

            if (pMeshCrateSliderCircle) {
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxSetTransparency(1.0f);
                AEGfxSetTextureMode(AE_GFX_TM_PRECISE);
                AEGfxTextureSet(pCrateSliderCircleTexture, 0, 0);  // Set the texture
                AEMtx33Trans(&trans, sliderX, sliderY);  // Apply position transformation
                AEMtx33Scale(&scale, 43 * gScaleX, 65 * gScaleY);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(pMeshCrateSliderCircle, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad

                // --- Quantity number text ---
                int qty_fruits = GetSliderFruitCount(selectedLocation);


                char fruitQTYText[32];
                sprintf_s(fruitQTYText, "%d", qty_fruits);


                //float x = -530.0f / 800.0f;  // normalize X by 800.0f
                //float y = 350.0f / 450.0f;  // normalize Y by 450.0f
                float xOf;
                if (qty_fruits <= 9) {
                    xOf = sliderX - 8;
                }
                else {
                    xOf = sliderX - 12;

                }
                float yOf = sliderY + 9;
                const float halfW = 800.0f;
                const float halfH = 450.0f;
                float xNorm = xOf / halfW;
                float yNorm = yOf / halfH;

                float r = 243.0f / 255.0f;
                float g = 196.0f / 255.0f;
                float b = 115.0f / 255.0f;

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1, 1, 1, 1);

                AEGfxPrint(fontId, fruitQTYText, xNorm, yNorm, 0.9f, r, g, b, 1);

            }

            if (Store_Crate.mesh && selectedLocation != NIL) {
                AEGfxTexture* tex = nullptr;
                float scaleX, scaleY;
                float X = Store_Crate.x, Y = Store_Crate.y;

                if (selectedLocation == INVENTORY) {
                    tex = Store_Crate.normalTex;
                    scaleX = Store_Crate.normalScaleX;
                    scaleY = Store_Crate.normalScaleY;
                }
                else if (selectedLocation == CRATE) {
                    tex = Store_Crate.altTex;
                    scaleX = Store_Crate.altScaleX;
                    scaleY = Store_Crate.altScaleY;
                }
                else {
                    tex = Store_Crate.normalTex;
                    scaleX = 0;
                    scaleY = 0;
                }

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxSetTransparency(1.0f);
                AEGfxSetTextureMode(AE_GFX_TM_PRECISE);
                AEGfxTextureSet(tex, 0, 0);  // Set the texture
                AEMtx33Trans(&trans, X, Y);  // Apply position transformation
                AEMtx33Scale(&scale, scaleX * gScaleX, scaleY * gScaleY);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
                AEGfxMeshDraw(Store_Crate.mesh, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
            }

            // Confirmation popup
            if (gInvConfirmOpen)
            {
                AEGfxTextureSet(confirmBG, 0, 0);
                AEMtx33Scale(&scale, 300 * gScaleX, 180 * gScaleY);
                AEMtx33Trans(&trans, 0, 0);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                //color
                float r = 243.0f / 255.0f;
                float g = 196.0f / 255.0f;
                float b = 115.0f / 255.0f;


                // YES

                AEGfxSetColorToMultiply(
                    gHoverYes ? r : 1.0f,
                    gHoverYes ? g : 1.0f,
                    gHoverYes ? b : 1.0f,
                    1.0f
                );

                AEGfxTextureSet(confirmYes, 0, 0);
                AEMtx33Scale(&scale, 71 * gScaleX, 49 * gScaleY);
                AEMtx33Trans(&trans, -60, -40);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                // NO

                AEGfxSetColorToMultiply(
                    gHoverNo ? r : 1.0f,
                    gHoverNo ? g : 1.0f,
                    gHoverNo ? b : 1.0f,
                    1.0f
                );

                AEGfxTextureSet(confirmNo, 0, 0);
                AEMtx33Scale(&scale, 71 * gScaleX, 49 * gScaleY);
                AEMtx33Trans(&trans, 60, -40);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }
        }
    }

    // THEN menu stuff
    if (!menuOpen)
        return;

    // --- Menu Background ---
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxTextureSet(menuTexture, 0, 0);

    AEMtx33Scale(&scale, 480, 850);
    AEMtx33Trans(&trans, -770 + 240, 0);
    AEMtx33Concat(&transform, &trans, &scale);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

    // --- Gold number text ---
    char goldText[32];
    sprintf_s(goldText, "%d", Economy_GetTotalMoney());


    float x = -530.0f / 800.0f;  // normalize X by 800.0f
    float y = 350.0f / 450.0f;  // normalize Y by 450.0f

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(0, 0, 0, 1);

    AEGfxPrint(fontId, goldText, x, y, 1.2f, 0, 0, 0, 1);

    // --- Menu Buttons ---
    for (auto& button : menuButtons)
    {
        AEGfxTextureSet(
            button.type == BUTTON_INVENTORY ? inventoryIcon :
            button.type == BUTTON_COLLECTION ? collectionIcon :
            settingsIcon, 0, 0);

        AEGfxSetColorToMultiply(
            1.0f,
            button.isHovered ? 0.9f : 1.0f,
            button.isHovered ? 0.9f : 1.0f,
            1.0f);

        AEMtx33Scale(&scale, button.width, button.height);
        AEMtx33Trans(&trans, button.x, button.y);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
    }

    if (popupOpen)
    {
        switch (activePopupIndex)
        {
        case BUTTON_INVENTORY:

        {
            const float panelW = 520.0f;
            const float panelH = 680.0f;
            const float panelX = 0.0f;   // centered
            const float panelY = 0.0f;   // centered

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxTextureSet(inventoryBG, 0, 0);

            AEMtx33Scale(&scale, panelW, panelH);
            AEMtx33Trans(&trans, panelX, panelY);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);


            // --- Fruits / Seeds icons (left-aligned under "Inventory") ---
            {
                const float leftEdge = panelX - INV_PANEL_W * 0.5f;
                const float topEdge = panelY + INV_PANEL_H * 0.5f;

                // Slot (base) size; both icons are drawn at this size
                const float slotW = INV_FRUIT_W;
                const float slotH = INV_FRUIT_H;

                // Centers computed from padding (so it's aligned to the left under the title)
                const float fruitCx = leftEdge + INV_TAB_LEFT_PAD + slotW * 0.5f;
                const float fruitCy = topEdge - INV_TAB_TOP_PAD - slotH * 0.5f;

                const float seedCx = fruitCx + INV_TAB_SPACING_X;
                const float seedCy = fruitCy;

                const bool fruitActive = (gActiveInvTab == TAB_FRUITS);
                const bool seedActive = (gActiveInvTab == TAB_SEEDS);

                // ----- FRUIT -----
                {
                    const float w = fruitActive ? INV_FRUIT_HL_W : INV_FRUIT_W;
                    const float h = fruitActive ? INV_FRUIT_HL_H : INV_FRUIT_H;

                    AEGfxTextureSet(fruitActive ? invFruitHighlight : invFruit, 0, 0);
                    AEMtx33Scale(&scale, w, h);
                    AEMtx33Trans(&trans, fruitCx, fruitCy); // same center
                    AEMtx33Concat(&transform, &trans, &scale);
                    AEGfxSetTransform(transform.m);
                    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
                }

                // ----- SEED -----
                {
                    const float w = seedActive ? INV_SEED_HL_W : INV_SEED_W;
                    const float h = seedActive ? INV_SEED_HL_H : INV_SEED_H;

                    AEGfxTextureSet(seedActive ? invSeedHighlight : invSeed, 0, 0);
                    AEMtx33Scale(&scale, w, h);
                    AEMtx33Trans(&trans, seedCx, seedCy);
                    AEMtx33Concat(&transform, &trans, &scale);
                    AEGfxSetTransform(transform.m);
                    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
                }
            }

            // --- Draw capacity text "current/limit" ---
            const int invcur = (gActiveInvTab == TAB_FRUITS) ? GetFruitCount() : GetSeedCount();// from Economy.cpp
            const int invmax = GetInventoryLimit();  // from Economy.cpp

            char capText[16];
            sprintf_s(capText, "%d/%d", invcur, invmax);

            // Position inside the small rounded header box on your PNG.
            // Nudge the multipliers a little if it’s off by a few pixels.
            float textX = (panelX + panelW * 0.24f) / 800.0f;
            float textY = (panelY + panelH * 0.41f) / 450.0f;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(0, 0, 0, 1); // dark text to match your PNG style
            AEGfxPrint(fontId, capText, textX, textY, 0.9f, 0, 0, 0, 1);

            // --- Slider fill + knob + min/max labels (single scope) ---
            // Panel edges
            const float panelCenterX = 0.0f, panelCenterY = 0.0f;
            const float leftEdge = panelCenterX - INV_PANEL_W * 0.5f;
            const float topEdge = panelCenterY + INV_PANEL_H * 0.5f;

            // Base endpoints from paddings
            const float baseX0 = leftEdge + INV_SLD_LEFT_PAD;
            const float baseX1 = baseX0 + INV_SLD_TRACK_W;

            // Inside rounded caps
            const float trackX0 = baseX0 + INV_SLD_TRACK_CAP_INSET;
            const float trackX1 = baseX1 - INV_SLD_TRACK_CAP_INSET;

            // Trough centerline (+ vertical nudge)
            const float trackY = (topEdge - INV_SLD_TOP_PAD) + INV_SLD_FILL_Y_OFFSET;

            // Range
            const int curCount = (gActiveInvTab == TAB_FRUITS)
                ? (gSelectedInvItem == INV_ITEM_PEAR ? GetPearCount()
                    : gSelectedInvItem == INV_ITEM_BANANA ? GetBananaCount()
                    : GetAppleCount())
                : (gSelectedInvItem == INV_ITEM_PEAR_SEED ? Inventory_GetSeedCount(SEED_PEAR)
                    : gSelectedInvItem == INV_ITEM_BANANA_SEED ? Inventory_GetSeedCount(SEED_BANANA)
                    : Inventory_GetSeedCount(SEED_APPLE));

            const int minVal = (curCount > 0) ? 1 : 0;   // set to 0 if you prefer [0..N]
            const int maxVal = curCount;

            gInvSliderValue = (std::max)(minVal, (std::min)(maxVal, gInvSliderValue));

            // Fraction and positions
            const float t = (maxVal > minVal)
                ? (float)(gInvSliderValue - minVal) / (float)(maxVal - minVal)
                : 0.0f;
            const float knobX = trackX0 + (trackX1 - trackX0) * t;
            const float knobY = trackY;
            const float stripeH = INV_SLD_FILL_THICKNESS;
            const float fillW = (trackX1 - trackX0) * t;
            const float pngCenterY = knobY + INV_SLD_KNOB_ANCHOR_Y;

            // -------- FILL --------
            if (fillW > 0.5f) {
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                if (invSliderFill) {
                    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                    AEGfxSetColorToMultiply(1.00f, 0.95f, 0.80f, 1.0f); // optional warm tint
                    AEGfxTextureSet(invSliderFill, 0, 0);
                }
                else {
                    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                    AEGfxSetColorToMultiply(1.0f, 0.92f, 0.6f, 1.0f);
                }
                AEMtx33Scale(&scale, fillW, stripeH);
                AEMtx33Trans(&trans, trackX0 + 0.5f * fillW, trackY);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }

            // -------- KNOB + BUBBLE --------
            if (invSliderKnob)
            {
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1, 1, 1, 1);
                AEGfxTextureSet(invSliderKnob, 0, 0);

                AEMtx33Scale(&scale, INV_SLD_KNOB_W, INV_SLD_KNOB_H);
                AEMtx33Trans(&trans, knobX, pngCenterY);
                AEMtx33Concat(&transform, &trans, &scale);

                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }


            // -------- Labels --------
            char lb[8], rb[8];
            sprintf_s(lb, "%d", minVal);
            sprintf_s(rb, "%d", maxVal);
            const float halfW = 800.0f, halfH = 450.0f;

            const float leftLabelX = (trackX0 + INV_SLD_LABEL_LEFT_XPAD) / halfW;
            const float leftLabelY = (trackY + INV_SLD_LABEL_Y_OFFSET) / halfH;
            const float rightLabelX = (trackX1 - INV_SLD_LABEL_RIGHT_XPAD) / halfW;
            const float rightLabelY = leftLabelY;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(0, 0, 0, 1);
            AEGfxPrint(fontId, lb, leftLabelX, leftLabelY, 0.8f, 0, 0, 0, 1);
            AEGfxPrint(fontId, rb, rightLabelX, rightLabelY, 0.8f, 0, 0, 0, 1);
            // -------- Value text INSIDE bubble (centered) --------
            {
                char valTxt[8];
                sprintf_s(valTxt, "%d", gInvSliderValue);

                const float vhalfW = 800.0f;
                const float vhalfH = 450.0f;

                // Bubble center
                const float textWorldX = knobX + INV_SLD_TEXT_X_OFFSET;
                const float textWorldY = knobY + INV_SLD_KNOB_ANCHOR_Y + INV_SLD_TEXT_FROM_PNG_CENTER_Y;

                // Approx width per character in normalized coordinates (tweak if needed)
                float charWidth = 0.012f; // ~1.2% of screen width per character
                int len = static_cast<int>(strlen(valTxt));
                float adjX = textWorldX / vhalfW - charWidth * (len - 1) * 0.5f;
                float vtextY = textWorldY / vhalfH;

                // Main text
                AEGfxSetColorToMultiply(0, 0, 0, 1);
                AEGfxPrint(fontId, valTxt, adjX, vtextY, 0.8f, 0, 0, 0, 1);
            }

            // ================= Inventory items =================
            {
                const float itemX = panelX + INV_ITEM_X;
                const float itemY = panelY + INV_ITEM_Y;

                const float iconSize = 100.0f;
                const float spacing = 100.0f; // horizontal gap between icons

                // Helper: draw one inventory slot (fruit or seed)
                auto DrawInvSlot = [&](
                    AEGfxTexture* tex,
                    float cx, float cy,
                    bool selected,
                    float tintR, float tintG, float tintB,
                    int count,
                    float cntOffX, float cntOffY,
                    float cntR, float cntG, float cntB)
                    {
                        bool hasItem = count > 0;

                        // --- ICON ---
                        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                        AEGfxSetColorToMultiply(1, 1, 1, 1);

                        AEGfxTextureSet(tex, 0, 0);
                        AEMtx33Scale(&scale, iconSize, iconSize);
                        AEMtx33Trans(&trans, cx, cy);
                        AEMtx33Concat(&transform, &trans, &scale);
                        AEGfxSetTransform(transform.m);
                        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                        // --- SELECTION HIGHLIGHT ---
                        if (selected && hasItem)
                        {
                            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                            AEGfxSetColorToMultiply(tintR, tintG, tintB, 0.4f);
                            AEMtx33Scale(&scale, (iconSize + 10.0f), (iconSize + 10.0f));
                            AEMtx33Trans(&trans, cx, cy);
                            AEMtx33Concat(&transform, &trans, &scale);
                            AEGfxSetTransform(transform.m);
                            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
                        }

                        // --- COUNT ---
                        char cnt[8];
                        sprintf_s(cnt, "%d", count);
                        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                        AEGfxPrint(fontId, cnt,
                            (cx + cntOffX) / 800.0f,
                            (cy + cntOffY) / 450.0f,
                            0.7f, cntR, cntG, cntB, 1);
                    };

                if (gActiveInvTab == TAB_FRUITS)
                {
                    DrawInvSlot(appleIcon, itemX, itemY,
                        gSelectedInvItem == INV_ITEM_APPLE,
                        1.0f, 0.75f, 0.2f,
                        GetAppleCount(), +40.0f, -40.0f, 0, 0, 0);
                    DrawInvSlot(pearIcon, itemX + spacing, itemY,
                        gSelectedInvItem == INV_ITEM_PEAR,
                        1.0f, 0.75f, 0.2f,
                        GetPearCount(), +40.0f, -40.00f, 0, 0, 0);
                    DrawInvSlot(bananaIcon, itemX + spacing * 2, itemY,
                        gSelectedInvItem == INV_ITEM_BANANA,
                        1.0f, 0.75f, 0.2f,
                        GetBananaCount(), +40.0f, -40.0f, 0, 0, 0);
                }
                else // TAB_SEEDS
                {
                    DrawInvSlot(appleSeedIcon, itemX, itemY,
                        gSelectedInvItem == INV_ITEM_APPLE_SEED,
                        0.3f, 0.9f, 0.2f,
                        Inventory_GetSeedCount(SEED_APPLE), +20.0f, -35.0f, 1, 1, 1);
                    DrawInvSlot(pearSeedIcon, itemX + spacing, itemY,
                        gSelectedInvItem == INV_ITEM_PEAR_SEED,
                        0.3f, 0.9f, 0.2f,
                        Inventory_GetSeedCount(SEED_PEAR), +20.0f, -35.0f, 1, 1, 1);
                    DrawInvSlot(bananaSeedIcon, itemX + spacing * 2, itemY,
                        gSelectedInvItem == INV_ITEM_BANANA_SEED,
                        0.3f, 0.9f, 0.2f,
                        Inventory_GetSeedCount(SEED_BANANA), +20.0f, -35.0f, 1, 1, 1);
                }
            }

            // ================= Trash button =================
            if (gSelectedInvItem != INV_ITEM_NONE)
            {
                float tx = panelX + INV_TRASH_X;
                float ty = panelY + INV_TRASH_Y;

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1, 1, 1, 1);

                AEGfxTextureSet(trashIcon, 0, 0);
                AEMtx33Scale(&scale, INV_TRASH_SIZE, INV_TRASH_SIZE);
                AEMtx33Trans(&trans, tx, ty);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }

            // Confirmation popup
            if (gInvConfirmOpen)
            {
                AEGfxTextureSet(confirmBG, 0, 0);
                AEMtx33Scale(&scale, 300, 180);
                AEMtx33Trans(&trans, 0, 0);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                // YES

                AEGfxSetColorToMultiply(
                    1.0f,
                    gHoverYes ? 0.9f : 1.0f,
                    gHoverYes ? 0.9f : 1.0f,
                    1.0f
                );

                AEGfxTextureSet(confirmYes, 0, 0);
                AEMtx33Scale(&scale, 80, 40);
                AEMtx33Trans(&trans, -60, -40);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                // NO

                AEGfxSetColorToMultiply(
                    1.0f,
                    gHoverNo ? 0.9f : 1.0f,
                    gHoverNo ? 0.9f : 1.0f,
                    1.0f
                );

                AEGfxTextureSet(confirmNo, 0, 0);
                AEMtx33Scale(&scale, 80, 40);
                AEMtx33Trans(&trans, 60, -40);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
            }


            break;
        }

        case BUTTON_COLLECTION:
        {
            const float panelW = 800.0f;
            const float panelH = 600.0f;
            const float panelX = 180.0f;   // to the right
            const float panelY = 0.0f;   // centered

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxTextureSet(collectionBG, 0, 0);

            AEMtx33Scale(&scale, panelW, panelH);
            AEMtx33Trans(&trans, panelX, panelY);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            break;
        }
        case BUTTON_SETTINGS:
        {
            // Background
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxTextureSet(settingsBG, 0, 0);

            AEMtx33Scale(&scale, SET_PANEL_W * ScaleX, SET_PANEL_H * ScaleY);
            AEMtx33Trans(&trans, SET_PANEL_X, SET_PANEL_Y);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            // SOUND toggle
            AEGfxSetColorToMultiply(1, gHoverSoundToggle ? 0.85f : 1, gHoverSoundToggle ? 0.85f : 1, 1);
            AEGfxTextureSet(gSoundEnabled ? settingsOn : settingsOff, 0, 0);

            AEMtx33Scale(&scale, SET_TOGGLE_W * ScaleX, SET_TOGGLE_H * ScaleY);
            AEMtx33Trans(&trans, SET_TOGGLE_X + SET_PANEL_X, SET_SOUND_Y + SET_PANEL_Y + 45.0f);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            // MUSIC toggle
            AEGfxSetColorToMultiply(1, gHoverMusicToggle ? 0.85f : 1, gHoverMusicToggle ? 0.85f : 1, 1);
            AEGfxTextureSet(gMusicEnabled ? settingsOn : settingsOff, 0, 0);

            AEMtx33Scale(&scale, SET_TOGGLE_W * ScaleX, SET_TOGGLE_H * ScaleY);
            AEMtx33Trans(&trans, SET_TOGGLE_X + SET_PANEL_X, SET_MUSIC_Y + SET_PANEL_Y + 5.0f);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            break;
        }
        }
    }


    if (popupOpen && AEInputCheckTriggered(AEVK_Q))
    {
        popupOpen = false;
    }

    // --- Upgrades Panel ---



    // Draw upgrades text & hover highlight

    float upgradesPanelW = UPGRADES_PANEL_W;
    float upgradesPanelH = UPGRADES_PANEL_H;
    float upgradesPanelX = UPGRADES_PANEL_X;
    float upgradesPanelY = UPGRADES_PANEL_Y;
    AEMtx33Scale(&scale, upgradesPanelW, upgradesPanelH);
    AEMtx33Trans(&trans, upgradesPanelX, upgradesPanelY);
    AEMtx33Concat(&transform, &trans, &scale);
    /*AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(0.2f, 0.2f, 0.2f, 1.0f);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);*/

    // Draw upgrades text & hover highlight
    float upgStartY = upgradesPanelY + 60.0f;
    float upgSpacing = 70.0f;

    int visibleSlot = 0;
    auto& upgradesList = Upgrades_GetList();

    for (size_t i = upgradesStartIndex;
        i < upgradesList.size() && visibleSlot < MAX_VISIBLE_UPGRADES;
        ++i)
    {
        if (upgradesList[i].purchased)
            continue;

        if (!upgradesList[i].texture) {
            printf("Upgrade %d missing texture!\n", static_cast<int>(i));
        }


        float rowY = upgStartY - visibleSlot * upgSpacing;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(upgradesList[i].texture, 0, 0);

        AEMtx33Scale(&scale, 369 * ScaleX, 70 * ScaleY);
        AEMtx33Trans(&trans, upgradesPanelX, rowY);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        visibleSlot++;
    }

    // --- Seeds Panel ---
    if (seedsPopupOpen)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxSetTransparency(1.0f);

        float seedspanelX = -70.0f;
        float seedspanelY = 20.0f;
        float seedY = seedspanelY + 115.0f;

        // Panel background (always drawn)
        AEGfxTextureSet(seedsTexture, 0, 0);
        AEMtx33Scale(&scale, 400, 550);
        AEMtx33Trans(&trans, seedspanelX, seedspanelY);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        // Left arrow
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxTextureSet(leftArrowTexture, 0, 0);
        AEMtx33Scale(&scale, 40, 40);
        AEMtx33Trans(&trans, seedspanelX - 150.0f, seedspanelY + 150.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        // Right arrow
        AEGfxTextureSet(rightArrowTexture, 0, 0);
        AEMtx33Scale(&scale, 40, 40);
        AEMtx33Trans(&trans, seedspanelX + 150.0f, seedspanelY + 150.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

        // --------------------------------------------------
        // Only draw seed icon + badge + info for apple slot
        // Empty slots show just the bare panel background
        // --------------------------------------------------
        if (currentSeedIndex == SEED_APPLE)
        {
            // Hover highlight
            if (hoveredSeed == SEED_APPLE)
            {
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 0.55f, 0.0f, 0.9f);

                AEMtx33Scale(&scale, 112, 112);
                AEMtx33Trans(&trans, seedspanelX, seedY);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetColorToMultiply(1, 1, 1, 1);
            }

            // Seed icon
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxTextureSet(appleSeedIcon, 0, 0);

            AEMtx33Scale(&scale, 100, 100);
            AEMtx33Trans(&trans, seedspanelX, seedY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            // Seed count badge
            int seedCount = Inventory_GetSeedCount(SEED_APPLE);
            char seedCountText[8];
            sprintf_s(seedCountText, "%d", seedCount);

            float badgeX = seedspanelX + 35.0f;
            float badgeY = seedY - 30.0f;
            float textBadgeX = badgeX - ((seedCount < 10) ? 4.0f : 8.0f);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxPrint(fontId, seedCountText, textBadgeX / 800.0f, badgeY / 450.0f, 0.7f, 1, 1, 1, 1);

            // Info text
            const float panelLeft = seedspanelX - 200.0f;
            const float panelTop = seedspanelY + 275.0f;
            const float scaleTexX = 400.0f / 616.0f;
            const float scaleTexY = 550.0f / 662.0f;

            const float coinCx = panelLeft + 118.0f * scaleTexX;
            const float coinCy = panelTop - 462.0f * scaleTexY;
            const float waterCx = panelLeft + 398.0f * scaleTexX;
            const float waterCy = coinCy;
            const float clockCx = coinCx;
            const float clockCy = panelTop - 525.0f * scaleTexY;

            const float tOff = 21.0f;
            const float tr = 0.25f, tg = 0.13f, tb = 0.02f;

            const float brownBoxCx = panelLeft + 308.0f * scaleTexX;
            const float brownBoxCy = panelTop - 385.0f * scaleTexY;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);

            SeedData& data = seedDatabase[SEED_APPLE];

            char costText[32];
            sprintf_s(costText, "%d / min", data.cost);

            char waterText[32];
            sprintf_s(waterText, "%d", data.waterNeeded);

            char growText[32];
            sprintf_s(growText, "%.0f mins", data.growTime);

            AEGfxPrint(fontId, data.name,
                (brownBoxCx - 80.0f) / 800.0f, brownBoxCy / 450.0f,
                1.1f, tr, tg, tb, 1);

            AEGfxPrint(fontId, costText,
                (coinCx + tOff) / 800.0f, coinCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            AEGfxPrint(fontId, waterText,
                (waterCx + tOff) / 800.0f, waterCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            AEGfxPrint(fontId, growText,
                (clockCx + tOff) / 800.0f, clockCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            const float descBoxTop = panelTop - 585.0f * scaleTexY + 10.0f;
            const float descScale = 0.5f;
            const float lineH = 13.0f;
            const float descX = (panelLeft + 68.0f) / 800.0f;

            AEGfxPrint(fontId, "The apple seed -- simple, but my favorite.",
                descX, (descBoxTop - lineH * 0.5f) / 450.0f, descScale, tr, tg, tb, 1);
            AEGfxPrint(fontId, "Used to give as a kid: 'Plant it, watch it grow.'",
                descX, (descBoxTop - lineH * 1.5f) / 450.0f, descScale, tr, tg, tb, 1);
            AEGfxPrint(fontId, "Funny how the smallest things become the best.",
                descX, (descBoxTop - lineH * 2.5f) / 450.0f, descScale, tr, tg, tb, 1);
        }
        else if (currentSeedIndex == 1)
        {
            // Hover highlight
            if (hoveredSeed == 1)
            {
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(0.4f, 0.8f, 0.2f, 0.9f);

                AEMtx33Scale(&scale, 112, 112);
                AEMtx33Trans(&trans, seedspanelX, seedY);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetColorToMultiply(1, 1, 1, 1);
            }

            // Seed icon
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxTextureSet(pearSeedIcon, 0, 0);

            AEMtx33Scale(&scale, 100, 100);
            AEMtx33Trans(&trans, seedspanelX, seedY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            // Seed count badge
            int pearSeedCount = Inventory_GetSeedCount(SEED_PEAR);
            char pearSeedCountText[8];
            sprintf_s(pearSeedCountText, "%d", pearSeedCount);

            float badgeX = seedspanelX + 35.0f;
            float badgeY = seedY - 30.0f;
            float textBadgeX = badgeX - ((pearSeedCount < 10) ? 4.0f : 8.0f);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxPrint(fontId, pearSeedCountText, textBadgeX / 800.0f, badgeY / 450.0f, 0.7f, 1, 1, 1, 1);

            // Info text — same layout as apple block
            const float panelLeft = seedspanelX - 200.0f;
            const float panelTop = seedspanelY + 275.0f;
            const float scaleTexX = 400.0f / 616.0f;
            const float scaleTexY = 550.0f / 662.0f;

            const float coinCx = panelLeft + 118.0f * scaleTexX;
            const float coinCy = panelTop - 462.0f * scaleTexY;
            const float waterCx = panelLeft + 398.0f * scaleTexX;
            const float waterCy = coinCy;
            const float clockCx = coinCx;
            const float clockCy = panelTop - 525.0f * scaleTexY;
            const float tOff = 21.0f;
            const float tr = 0.25f, tg = 0.13f, tb = 0.02f;
            const float brownBoxCx = panelLeft + 308.0f * scaleTexX;
            const float brownBoxCy = panelTop - 385.0f * scaleTexY;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);

            SeedData& data = seedDatabase[SEED_PEAR];

            char costText[32];
            sprintf_s(costText, "%d / min", data.cost);

            char waterText[32];
            sprintf_s(waterText, "%d", data.waterNeeded);

            char growText[32];
            sprintf_s(growText, "%.0f mins", data.growTime);

            AEGfxPrint(fontId, data.name,
                (brownBoxCx - 80.0f) / 800.0f, brownBoxCy / 450.0f,
                1.1f, tr, tg, tb, 1);

            AEGfxPrint(fontId, costText,
                (coinCx + tOff) / 800.0f, coinCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            AEGfxPrint(fontId, waterText,
                (waterCx + tOff) / 800.0f, waterCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            AEGfxPrint(fontId, growText,
                (clockCx + tOff) / 800.0f, clockCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            const float descBoxTop = panelTop - 585.0f * scaleTexY + 10.0f;
            const float descScale = 0.5f;
            const float lineH = 13.0f;
            const float descX = (panelLeft + 68.0f) / 800.0f;

            AEGfxPrint(fontId, "The pear seed -- takes patience, but worth it.",
                descX, (descBoxTop - lineH * 0.5f) / 450.0f, descScale, tr, tg, tb, 1);
            AEGfxPrint(fontId, "Soft, sweet fruit. Grandma's favorite.",
                descX, (descBoxTop - lineH * 1.5f) / 450.0f, descScale, tr, tg, tb, 1);
            AEGfxPrint(fontId, "Give it time and it will never disappoint.",
                descX, (descBoxTop - lineH * 2.5f) / 450.0f, descScale, tr, tg, tb, 1);
        }
        // else: empty slot — nothing drawn on top of the panel background


        //BANANA
        if (currentSeedIndex == SEED_BANANA)
        {
            // Hover highlight
            if (hoveredSeed == SEED_BANANA)
            {
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 0.85f, 0.0f, 0.9f);

                AEMtx33Scale(&scale, 112, 112);
                AEMtx33Trans(&trans, seedspanelX, seedY);
                AEMtx33Concat(&transform, &trans, &scale);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetColorToMultiply(1, 1, 1, 1);
            }

            // Seed icon
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxTextureSet(bananaSeedIcon, 0, 0);

            AEMtx33Scale(&scale, 100, 100);
            AEMtx33Trans(&trans, seedspanelX, seedY);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            // Seed count badge
            int bananaSeedCount = Inventory_GetSeedCount(SEED_BANANA);
            char bananaSeedCountText[8];
            sprintf_s(bananaSeedCountText, "%d", bananaSeedCount);

            float badgeX = seedspanelX + 35.0f;
            float badgeY = seedY - 30.0f;
            float textBadgeX = badgeX - ((bananaSeedCount < 10) ? 4.0f : 8.0f);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
            AEGfxPrint(fontId, bananaSeedCountText, textBadgeX / 800.0f, badgeY / 450.0f, 0.7f, 1, 1, 1, 1);

            // Info text
            const float panelLeft = seedspanelX - 200.0f;
            const float panelTop = seedspanelY + 275.0f;
            const float scaleTexX = 400.0f / 616.0f;
            const float scaleTexY = 550.0f / 662.0f;

            const float coinCx = panelLeft + 118.0f * scaleTexX;
            const float coinCy = panelTop - 462.0f * scaleTexY;
            const float waterCx = panelLeft + 398.0f * scaleTexX;
            const float waterCy = coinCy;
            const float clockCx = coinCx;
            const float clockCy = panelTop - 525.0f * scaleTexY;
            const float tOff = 21.0f;
            const float tr = 0.25f, tg = 0.13f, tb = 0.02f;
            const float brownBoxCx = panelLeft + 308.0f * scaleTexX;
            const float brownBoxCy = panelTop - 385.0f * scaleTexY;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1, 1, 1, 1);

            SeedData& data = seedDatabase[SEED_BANANA];

            char costText[32];
            sprintf_s(costText, "%d / min", data.cost);

            char waterText[32];
            sprintf_s(waterText, "%d", data.waterNeeded);

            char growText[32];
            sprintf_s(growText, "%.0f mins", data.growTime);

            AEGfxPrint(fontId, data.name,
                (brownBoxCx - 80.0f) / 800.0f, brownBoxCy / 450.0f,
                1.1f, tr, tg, tb, 1);

            AEGfxPrint(fontId, costText,
                (coinCx + tOff) / 800.0f, coinCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            AEGfxPrint(fontId, waterText,
                (waterCx + tOff) / 800.0f, waterCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            AEGfxPrint(fontId, growText,
                (clockCx + tOff) / 800.0f, clockCy / 450.0f,
                1.0f, tr, tg, tb, 1);

            const float descBoxTop = panelTop - 585.0f * scaleTexY + 10.0f;
            const float descScale = 0.5f;
            const float lineH = 13.0f;
            const float descX = (panelLeft + 68.0f) / 800.0f;

            AEGfxPrint(fontId, "The banana seed -- tropical and fast-growing.",
                descX, (descBoxTop - lineH * 0.5f) / 450.0f, descScale, tr, tg, tb, 1);
            AEGfxPrint(fontId, "Bright yellow fruit with a natural sweetness.",
                descX, (descBoxTop - lineH * 1.5f) / 450.0f, descScale, tr, tg, tb, 1);
            AEGfxPrint(fontId, "A crowd favourite at the stall.",
                descX, (descBoxTop - lineH * 2.5f) / 450.0f, descScale, tr, tg, tb, 1);
        }
    }

    //Plot Slots
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxTextureSet(plotSlotTexture, 0, 0);

    for (int i = 0; i < (int)plotSlots.size(); i++)
    {
        PlotSlot& slot = plotSlots[i];

        // Pick texture based on planted seed type
        AEGfxTexture* slotTex = plotSlotTexture; // default = apple/empty
        if (Farm_IsPlotPlanted(i) && farmPlots[i].seedType == SEED_PEAR)
            slotTex = plotSlotPearTexture;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxTextureSet(slotTex, 0, 0);  // <-- was hardcoded plotSlotTexture

        AEMtx33Scale(&scale, slot.width, slot.height);
        AEMtx33Trans(&trans, slot.x, slot.y);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
        // Hover tint
        if ((int)i == hoveredPlotIndex)
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(0.2f, 0.8f, 0.3f, 0.55f);

            AEMtx33Scale(&scale, slot.width, slot.height);
            AEMtx33Trans(&trans, slot.x, slot.y);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);
        }
    }

}

bool UI_IsMenuOpen()
{
    return menuOpen;
}

// ============================
// Crate hover + tints
// ============================

// Build crate hover rectangles so they align with the wooden bins on the stall.
// Uses the SAME transform in MainScreen_Render() for the stall image.

static void UI_ResetCrateConfigToDefaults()
{
    // tuned values
    gCrateCfg.bins[0] = { 0.285f, 0.740f, 0.200f, 0.135f }; // Left bin
    gCrateCfg.bins[1] = { 0.475f, 0.740f, 0.170f, 0.135f }; // Middle bin
    gCrateCfg.bins[2] = { 0.660f, 0.740f, 0.200f, 0.135f }; // Right bin

    // Tooltip defaults
    gCrateCfg.tipWidth = 200.0f;
    gCrateCfg.tipHeight = 100.0f;
    gCrateCfg.tipMargin = 24.0f;
    gCrateCfg.tipCenterOnCrate = true;
}

static inline void UI_EnsureCrateCfg()
{
    if (!gCrateCfgInitialized) {
        UI_ResetCrateConfigToDefaults();
        gCrateCfgInitialized = true;
    }
}

const CrateLayoutConfig& UI_GetCrateLayoutConfig()
{
    UI_EnsureCrateCfg();
    return gCrateCfg;
}

void UI_SetCrateLayoutConfig(const CrateLayoutConfig& cfg)
{
    gCrateCfg = cfg;
    gCrateCfgInitialized = true;
}

// ---- builder ( uses config) ----
void UI_RebuildCrateHitboxesFromStall(float stallX, float stallY, float stallW, float stallH)
{
    UI_EnsureCrateCfg();

    gFruitBaskets.clear();


    auto uvToWorldRect = [stallX, stallY, stallW, stallH](float uC, float vC, float uW, float vH) -> FruitBasket
        {
            FruitBasket b{};
            const float xLocal = (uC - 0.5f) * stallW;  // mesh centered
            const float yLocal = (0.5f - vC) * stallH;  // invert V (texture top = 0)
            b.x = stallX + xLocal;
            b.y = stallY + yLocal;
            b.width = uW * stallW;
            b.height = vH * stallH;
            return b;
        };


    for (int i = 0; i < 3; ++i) {
        const auto& r = gCrateCfg.bins[i];

        FruitBasket b = uvToWorldRect(r.uCenter, r.vCenter, r.uWidth, r.vHeight);

        b.fruitType = (fruitType)i;   // 0 Apple, 1 Pear, 2 Banana
        b.stock = 0;

        gFruitBaskets.push_back(b);
    }
}

// AABB test for a basket/crate
static bool IsMouseOverBasket(const FruitBasket& basket)
{
    float worldX, worldY;
    GetWorldMouse(worldX, worldY);

    return worldX >= basket.x - basket.width * 0.5f &&
        worldX <= basket.x + basket.width * 0.5f &&
        worldY >= basket.y - basket.height * 0.5f &&
        worldY <= basket.y + basket.height * 0.5f;
}


// Draw a tooltip clamped to the screen, centered at world (cx, cy)
static void DrawTooltipClampedAt(float cx, float cy, const char* text,
    float w, float h)
{
    const float halfW = 1600.0f * 0.5f;
    const float halfH = 900.0f * 0.5f;

    const float minX = -halfW + w * 0.5f + 6.0f;
    const float maxX = halfW - w * 0.5f - 6.0f;
    const float minY = -halfH + h * 0.5f + 6.0f;
    const float maxY = halfH - h * 0.5f - 6.0f;

    const float x = clampf(cx, minX, maxX);
    const float y = clampf(cy, minY, maxY);

    AEMtx33 scale, trans, transform;
    AEMtx33Scale(&scale, w, h);
    AEMtx33Trans(&trans, x, y);
    AEMtx33Concat(&transform, &trans, &scale);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(0.10f, 0.10f, 0.10f, 0.85f); // panel color
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

    // AE text uses coords normalized by half sizes
    const float xText = (x - w * 0.45f) / halfW;
    const float yText = (y + h * 0.25f) / halfH;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxPrint(fontId, text, xText, yText, 1.0f, 1, 1, 1, 1);
}

void UI_DrawFruitBasketTooltips()
{
    if (cratePopupOpen) return; // hide tooltips if crate panel is open

    UI_EnsureCrateCfg();
    const auto& C = UI_GetCrateLayoutConfig();
    const auto& baskets = GetFruitBaskets();

    int mx, my;
    AEInputGetCursorPosition(&mx, &my);
    const float halfW = 1600.0f * 0.5f;
    const float halfH = 900.0f * 0.5f;
    const float worldX = (float)mx - halfW;
    const float worldY = halfH - (float)my;

    for (const auto& b : baskets)
    {
        const bool isHover =
            worldX >= b.x - b.width * 0.5f &&
            worldX <= b.x + b.width * 0.5f &&
            worldY >= b.y - b.height * 0.5f &&
            worldY <= b.y + b.height * 0.5f;

        if (!isHover) continue;

        // Tooltip panel position
        float tipX = C.tipCenterOnCrate ? b.x : (b.x - b.width * 0.5f + C.tipWidth * 0.5f);
        float tipY = (b.y - b.height * 0.5f) - C.tipMargin - (C.tipHeight * 0.5f);

        if (tipY - C.tipHeight * 0.5f < -halfH + 6.0f) {
            tipY = (b.y + b.height * 0.5f) + C.tipMargin + (C.tipHeight * 0.5f);
        }

        // Panel background
        DrawTooltipClampedAt(tipX, tipY, "", C.tipWidth, C.tipHeight);

        // --- Text for the fruit ---
        const char* fruitName = "Unknown";
        const char* stockText = "Stock: ?";
        const char* inventoryText = "Inventory: ?";


        char stockBuf[32];
        char invBuf[32];

        // GET LIVE STOCK DATA FROM CRATE SYSTEM
        int liveStock = Crate_GetFruitCount(b.fruitType);


        switch (b.fruitType)
        {
        case FRUIT_APPLE:
            fruitName = "Apple";
            snprintf(stockBuf, sizeof(stockBuf), "Stock: %d", liveStock);
            snprintf(invBuf, sizeof(invBuf), "Inventory: %d", GetAppleCount());
            break;

        case FRUIT_PEAR:
            fruitName = "Pear";
            snprintf(stockBuf, sizeof(stockBuf), "Stock: %d", liveStock);
            snprintf(invBuf, sizeof(invBuf), "Inventory: %d", GetPearCount());
            break;

        case FRUIT_BANANA:
            fruitName = "Banana";
            snprintf(stockBuf, sizeof(stockBuf), "Stock: %d", liveStock);
            snprintf(invBuf, sizeof(invBuf), "Inventory: %d", GetBananaCount());
            break;
        }

        stockText = stockBuf;
        inventoryText = invBuf;


        // --- Draw the text inside the panel ---
        const float textStartY = tipY + C.tipHeight * 0.25f; // start a bit below top
        const float lineSpacing = 25.0f;

        const float xTextNorm = (tipX - C.tipWidth * 0.45f) / halfW;

        // Fruit name at top
        const float yNameNorm = textStartY / halfH;
        AEGfxPrint(fontId, fruitName, xTextNorm, yNameNorm, 1.0f, 1, 1, 1, 1);

        // Stock underneath
        const float yStockNorm = (textStartY - lineSpacing) / halfH;
        AEGfxPrint(fontId, stockText, xTextNorm, yStockNorm, 1.0f, 1, 1, 1, 1);

        // Inventory underneath
        const float yInvNorm = (textStartY - 2.0f * lineSpacing) / halfH;
        AEGfxPrint(fontId, inventoryText, xTextNorm, yInvNorm, 1.0f, 1, 1, 1, 1);

        break; // only show one tooltip at a time
    }
}

void UI_DrawPlotTooltips()
{
    // Get mouse position
    int mx, my;
    AEInputGetCursorPosition(&mx, &my);

    const float halfW = 1600.0f * 0.5f;
    const float halfH = 900.0f * 0.5f;
    const float worldX = (float)mx - halfW;
    const float worldY = halfH - (float)my;

    // Get reference to farm plots
    extern std::vector<FarmPlot> farmPlots;

    // Check each plot slot
    for (int i = 0; i < (int)plotSlots.size(); i++)
    {
        const PlotSlot& slot = plotSlots[i];

        // Skip if plot is locked
        if (Farm_IsPlotLocked(static_cast<int>(i)))
            continue;

        // Skip if plot is not planted
        if (!Farm_IsPlotPlanted(static_cast<int>(i)))
            continue;

        // Check if mouse is over this plot
        bool isOver = worldX >= slot.x - slot.width * 0.5f &&
            worldX <= slot.x + slot.width * 0.5f &&
            worldY >= slot.y - slot.height * 0.5f &&
            worldY <= slot.y + slot.height * 0.5f;

        if (isOver)
        {
            const FarmPlot& plot = farmPlots[i];
            float growTime = Farm_GetGrowTime();

            // Calculate remaining time
            char line1[64] = "";
            char line2[64] = "";
            int lineCount = 1;

            if (plot.isReady)
            {
                sprintf_s(line1, "Ready to harvest!");
                sprintf_s(line2, "Press SPACE");
                lineCount = 2;

                // Draw tooltip for ready plot
                float tipX = slot.x;
                float tipY = slot.y + slot.height * 0.7f;

                const float tooltipW = 200.0f;
                const float tooltipH = (lineCount == 2) ? 70.0f : 40.0f;

                // Draw background
                AEMtx33 scale, trans, transform;
                AEMtx33Scale(&scale, tooltipW, tooltipH);
                AEMtx33Trans(&trans, tipX, tipY);
                AEMtx33Concat(&transform, &trans, &scale);

                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 0.85f);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                // Draw text lines
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 0.5f, 1.0f);

                float startY = tipY + (lineCount == 2 ? 10.0f : 0.0f);

                float nx = (tipX - tooltipW * 0.4f) / halfW;
                float ny = (startY + 8.0f) / halfH;
                AEGfxPrint(fontId, line1, nx, ny, 0.65f, 1.0f, 1.0f, 0.5f, 0.9f);

                if (lineCount == 2)
                {
                    ny = (startY - 18.0f) / halfH;
                    AEGfxPrint(fontId, line2, nx, ny, 0.65f, 1.0f, 1.0f, 0.5f, 0.9f);
                }
            }
            else if (plot.isPlanted && !plot.growthFrozen)
            {
                float remaining = growTime - plot.growTimer;
                if (remaining < 0.0f) remaining = 0.0f;

                sprintf_s(line1, "Growing...");

                // Format time
                if (remaining >= 60.0f)
                {
                    int minutes = (int)(remaining / 60.0f);
                    int seconds = (int)remaining % 60;
                    sprintf_s(line2, "%d:%02d remaining", minutes, seconds);
                }
                else
                {
                    sprintf_s(line2, "%.0f sec remaining", remaining);
                }
                lineCount = 2;

                // Draw tooltip
                float tipX = slot.x;
                float tipY = slot.y + slot.height * 0.7f;

                const float tooltipW = 200.0f;
                const float tooltipH = 70.0f;

                // Draw background
                AEMtx33 scale, trans, transform;
                AEMtx33Scale(&scale, tooltipW, tooltipH);
                AEMtx33Trans(&trans, tipX, tipY);
                AEMtx33Concat(&transform, &trans, &scale);

                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 0.85f);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                // Draw text lines
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(0.8f, 0.9f, 1.0f, 1.0f);

                float startY = tipY + 10.0f;

                float nx = (tipX - tooltipW * 0.4f) / halfW;
                float ny = (startY + 8.0f) / halfH;
                AEGfxPrint(fontId, line1, nx, ny, 0.65f, 0.8f, 0.9f, 1.0f, 0.9f);

                ny = (startY - 18.0f) / halfH;
                AEGfxPrint(fontId, line2, nx, ny, 0.65f, 0.8f, 0.9f, 1.0f, 0.9f);
            }
            else if (plot.growthFrozen)
            {
                sprintf_s(line1, "Growth frozen!");
                sprintf_s(line2, "Need rhythm game");
                lineCount = 2;

                // Draw tooltip
                float tipX = slot.x;
                float tipY = slot.y + slot.height * 0.7f;

                const float tooltipW = 180.0f;
                const float tooltipH = 70.0f;

                // Draw background
                AEMtx33 scale, trans, transform;
                AEMtx33Scale(&scale, tooltipW, tooltipH);
                AEMtx33Trans(&trans, tipX, tipY);
                AEMtx33Concat(&transform, &trans, &scale);

                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 0.85f);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

                // Draw text lines
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 0.7f, 0.5f, 1.0f);

                float startY = tipY + 10.0f;

                float nx = (tipX - tooltipW * 0.4f) / halfW;
                float ny = (startY + 8.0f) / halfH;
                AEGfxPrint(fontId, line1, nx, ny, 0.65f, 1.0f, 0.7f, 0.5f, 0.9f);

                ny = (startY - 18.0f) / halfH;
                AEGfxPrint(fontId, line2, nx, ny, 0.65f, 1.0f, 0.7f, 0.5f, 0.9f);
            }

            break; // Only show one tooltip at a time
        }
    }
}

void UI_DrawCrateHoverTint_Yellow()
{
    if (cratePopupOpen) return; // hide hover tint if crate panel is open

    AEMtx33 scale, trans, transform;
    const auto& baskets = GetFruitBaskets();

    for (const auto& b : baskets)
    {
        if (IsMouseOverBasket(b))
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(0.9f, 0.8f, 0.2f, 0.5f);

            AEMtx33Scale(&scale, b.width, b.height);
            AEMtx33Trans(&trans, b.x, b.y);
            AEMtx33Concat(&transform, &trans, &scale);

            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1, 1, 1, 1);

            break;
        }
    }
}

//FARM PLOTS

float UI_GetPlotCenterX()
{
    return plotPlusButton.x;
}

float UI_GetPlotCenterY()
{
    return plotPlusButton.y;
}

float UI_GetPlotSlotX(int index)
{
    if (index < 0 || index >= plotSlots.size())
        return 0.0f;

    return plotSlots[index].x;
}

float UI_GetPlotSlotY(int index)
{
    if (index < 0 || index >= plotSlots.size())
        return 0.0f;

    return plotSlots[index].y;
}

float UI_GetCrateSlotX(int index)
{
    // Crate slots are positioned on the stall — adjust these values to match your layout
    const float crateBaseX = -300.0f;
    const float crateSpacing = 100.0f;
    return crateBaseX + (float)index * crateSpacing;
}

float UI_GetCrateSlotY(int index)
{
    (void)index;
    return -150.0f; // all crates sit on the same horizontal row
}

void UI_Exit()
{
    // free textures here
    AEGfxTextureUnload(menuTexture);
    AEGfxTextureUnload(seedsTexture);

    AEGfxTextureUnload(inventoryIcon);
    AEGfxTextureUnload(inventoryBG);
    AEGfxTextureUnload(invFruit);
    AEGfxTextureUnload(invFruitHighlight);
    AEGfxTextureUnload(invSeed);
    AEGfxTextureUnload(invSeedHighlight);
    AEGfxTextureUnload(invSliderKnob);
    AEGfxTextureUnload(invSliderFill);
    AEGfxTextureUnload(trashIcon);
    AEGfxTextureUnload(confirmBG);
    AEGfxTextureUnload(confirmYes);
    AEGfxTextureUnload(confirmNo);
    AEGfxTextureUnload(appleIcon);
    AEGfxTextureUnload(pearIcon);
    AEGfxTextureUnload(bananaIcon);

    AEGfxTextureUnload(collectionIcon);
    AEGfxTextureUnload(collectionBG);

    AEGfxTextureUnload(settingsIcon);
    AEGfxTextureUnload(settingsBG);
    AEGfxTextureUnload(settingsOn);
    AEGfxTextureUnload(settingsOff);

    AEGfxTextureUnload(appleSeedIcon);
    AEGfxTextureUnload(pearSeedIcon);
    AEGfxTextureUnload(bananaSeedIcon);
    AEGfxTextureUnload(appleSeedInfo);
    AEGfxTextureUnload(plotSlotTexture);
    AEGfxTextureUnload(plotSlotBananaTexture);
    AEGfxTextureUnload(leftArrowTexture);
    AEGfxTextureUnload(rightArrowTexture);

    Upgrades_Unload();
}