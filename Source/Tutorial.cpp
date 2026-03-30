#include "Tutorial.h"
#include "AEEngine.h"
#include "Utilities.h"
#include <cstring>
#include <cstdio>

// ---------------------------------------------------------------------------
// Extern resources owned by StartScreen / Main
// ---------------------------------------------------------------------------
extern float gScaleX;
extern float gScaleY;
extern s8 fontId;           // Crayon pastel font from Main.cpp
extern AEGfxVertexList* g_pMeshFullScreen;


// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------
static AEGfxTexture* pTexPanel = nullptr;  // panel_blue.png
static AEGfxTexture* pTexArrowLeft = nullptr;  // arrowBrown_left.png
static AEGfxTexture* pTexArrowRight = nullptr;  // arrowBrown_right.png
static AEGfxTexture* pTexDot = nullptr;  // iconCircle_brown.png
static AEGfxVertexList* pMeshQuad = nullptr;
static s8 tutFontRegular = -1;      // Nunito Regular
static s8 tutFontSemibold = -1;     // Nunito Semibold

// ---------------------------------------------------------------------------
// Layout constants  (pixel-space, world origin = screen centre)
// ---------------------------------------------------------------------------
// ---- Tutorial panel ----
static const float PANEL_W = 900.0f;
static const float PANEL_H = 580.0f;
static const float PANEL_X = 0.0f;
static const float PANEL_Y = 20.0f;

// ---- Arrow buttons ----
static const float ARROW_SIZE = 60.0f;
static const float ARROW_Y = -220.0f;
static const float ARROW_LEFT_X = -380.0f;
static const float ARROW_RIGHT_X = 380.0f;

// ---- Page-indicator dots ----
static const float DOT_SIZE = 20.0f;
static const float DOT_Y = -245.0f;
static const float DOT_SPACING = 30.0f;

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
static bool g_tutOpen = false;
static int  g_page = 0;
static bool g_hoverL = false;
static bool g_hoverR = false;

// ---------------------------------------------------------------------------
// Page content
// ---------------------------------------------------------------------------
struct TutPage
{
    const char* title;
    const char* lines[6];
    int         lineCount;
};

static const TutPage PAGES[] =
{
    {
        "Welcome to Fruit Stall!",
        {
            "Grow fruit, run a market stall,",
            "and earn coins to build your",
            "farming empire!",
            "",
            "Use arrows or LEFT / RIGHT keys",
            "to flip through this guide."
        },
        6
    },
    {
        "Controls",
        {
            "[ M ]  Open / close the side menu",
            "[ ESC ]  Pause & main menu",
            "",
            "[ SPACE ]  Harvest ready crops",
            "[ LMB ]  Click to interact",
            ""
        },
        5
    },
    {
        "The Farm - Planting",
        {
            "Open the menu and select a",
            "plot to plant your seed.",
            "",
            "* Plot 1 is unlocked by default",
            "* Buy upgrades to unlock more",
            "* Different seeds, different yields"
        },
        6
    },
    {
        "The Farm - Growing",
        {
            "Seeds grow automatically over",
            "time - watch the plot indicator.",
            "",
            "* At 50% growth a Rhythm event",
            "  triggers - don't miss it!",
            ""
        },
        5
    },
    {
        "Rhythm Mini-Game",
        {
            "Hit notes in time with the beat",
            "to boost your crop's growth!",
            "",
            "* Great score  =  bonus fruit",
            "* Poor score   =  growth penalty",
            "* Practice makes perfect!"
        },
        6
    },
    {
        "Harvesting & Selling",
        {
            "A glowing plot means it's ready!",
            "Press SPACE to harvest all crops",
            "into your inventory at once.",
            "",
            "Drag fruit into crates to put",
            "them up for sale at your stall."
        },
        6
    },
    {
        "Upgrades",
        {
            "Visit the Upgrades section in",
            "the side menu to improve your",
            "stall and farm.",
            "",
            "Try: Speed Boost, Crate Storage,",
            "Faster Growth & more!"
        },
        6
    },
    {
        "Top Tips",
        {
            "* Keep ALL plots planted always",
            "* Never let your crates go empty",
            "* Nail the rhythm game every time",
            "* Spend coins on upgrades early",
            "",
            "Now go grow that stall!  :)"
        },
        6
    }
};

static const int PAGE_COUNT = static_cast<int>(sizeof(PAGES) / sizeof(PAGES[0]));

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static AEGfxVertexList* MakeTutQuad()
{
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    return AEGfxMeshEnd();
}

// Draw a textured quad centred at (cx, cy) with pixel dimensions (w, h).
// Position is NOT multiplied by gScale here -- caller passes raw world coords.
static void DrawTex(AEGfxTexture* tex, float cx, float cy, float w, float h, float alpha = 1.0f)
{
    if (!tex || !pMeshQuad) return;
    AEMtx33 sc, tr, tf;
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetTransparency(alpha);
    AEGfxTextureSet(tex, 0, 0);
    AEMtx33Scale(&sc, w * gScaleX, h * gScaleY);
    AEMtx33Trans(&tr, cx, cy);
    AEMtx33Concat(&tf, &tr, &sc);
    AEGfxSetTransform(tf.m);
    AEGfxMeshDraw(pMeshQuad, AE_GFX_MDM_TRIANGLES);
}

// Draw a colour-only rect (used for the dim overlay).
static void DrawColorRect(float cx, float cy, float w, float h,
    float r, float g, float b, float a)
{
    if (!g_pMeshFullScreen) return;
    AEMtx33 sc, tr, tf;
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(r, g, b, a);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEMtx33Scale(&sc, w * gScaleX, h * gScaleY);
    AEMtx33Trans(&tr, cx, cy);
    AEMtx33Concat(&tf, &tr, &sc);
    AEGfxSetTransform(tf.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void Tutorial_Load()
{
    // Guard every pointer so this function is safe to call from both
    // StartScreen_Load AND StartScreen_Init (matching the pattern used
    // for all other button resources in StartScreen.cpp).
    if (!pTexPanel)       pTexPanel = AEGfxTextureLoad("Assets/panel_blue.png");
    if (!pTexArrowLeft)   pTexArrowLeft = AEGfxTextureLoad("Assets/arrowBrown_left.png");
    if (!pTexArrowRight)  pTexArrowRight = AEGfxTextureLoad("Assets/arrowBrown_right.png");
    if (!pTexDot)         pTexDot = AEGfxTextureLoad("Assets/iconCircle_brown.png");
    if (!pMeshQuad)       pMeshQuad = MakeTutQuad();

    // Load Nunito fonts
    if (tutFontRegular < 0)
    {
        tutFontRegular = AEGfxCreateFont("Assets/Nunito-Regular.ttf", 22);
    }

    if (tutFontSemibold < 0)
    {
        tutFontSemibold = AEGfxCreateFont("Assets/Nunito-SemiBold.ttf", 26);
        // Fallback to regular if semibold fails
        if (tutFontSemibold < 0)
            tutFontSemibold = tutFontRegular;
    }

    g_tutOpen = false;
    g_page = 0;
}

void Tutorial_Unload()
{
    if (pTexPanel) { AEGfxTextureUnload(pTexPanel);       pTexPanel = nullptr; }
    if (pTexArrowLeft) { AEGfxTextureUnload(pTexArrowLeft);   pTexArrowLeft = nullptr; }
    if (pTexArrowRight) { AEGfxTextureUnload(pTexArrowRight);  pTexArrowRight = nullptr; }
    if (pTexDot) { AEGfxTextureUnload(pTexDot);         pTexDot = nullptr; }
    if (pMeshQuad) { AEGfxMeshFree(pMeshQuad);            pMeshQuad = nullptr; }
    if (tutFontRegular >= 0) { AEGfxDestroyFont(tutFontRegular);   tutFontRegular = -1; }
    if (tutFontSemibold >= 0 && tutFontSemibold != tutFontRegular)
    {
        AEGfxDestroyFont(tutFontSemibold);
        tutFontSemibold = -1;
    }
}

bool Tutorial_IsOpen()
{
    return g_tutOpen;
}

void Tutorial_Open()
{
    g_tutOpen = true;
    g_page = 0;
}

void Tutorial_Update()
{
    if (!g_tutOpen) return;

    // Convert mouse pixel position to world space
    int mx, my;
    AEInputGetCursorPosition(&mx, &my);
    float wx = static_cast<float>(mx) - 800.0f;
    float wy = 450.0f - static_cast<float>(my);

    // ---- Panel is open: handle navigation ----
    if (AEInputCheckTriggered(AEVK_ESCAPE))
    {
        g_tutOpen = false;
        return;
    }

    // Arrow hit-boxes (panel elements stay centred -- no slideOffset)
    float lx = ARROW_LEFT_X;
    float rx = ARROW_RIGHT_X;
    float ay = ARROW_Y;
    float hs = ARROW_SIZE * 0.5f;

    g_hoverL = (wx >= lx - hs && wx <= lx + hs && wy >= ay - hs && wy <= ay + hs);
    g_hoverR = (wx >= rx - hs && wx <= rx + hs && wy >= ay - hs && wy <= ay + hs);

    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        if (g_hoverL && g_page > 0)
            g_page--;
        else if (g_hoverR && g_page < PAGE_COUNT - 1)
            g_page++;
        else if (!g_hoverL && !g_hoverR)
        {
            // Click outside panel to dismiss
            float px = PANEL_X;
            float py = PANEL_Y;
            float phw = PANEL_W * 0.5f;
            float phh = PANEL_H * 0.5f;
            bool insidePanel = (wx >= px - phw && wx <= px + phw &&
                wy >= py - phh && wy <= py + phh);
            if (!insidePanel)
                g_tutOpen = false;
        }
    }

    // Keyboard shortcuts
    if (AEInputCheckTriggered(AEVK_LEFT) && g_page > 0)              g_page--;
    if (AEInputCheckTriggered(AEVK_RIGHT) && g_page < PAGE_COUNT - 1) g_page++;
}

void Tutorial_Draw()
{
    if (!g_tutOpen || !pMeshQuad) return;

    // ---- Tutorial panel ----

    // 1. Dim overlay (full screen)
    DrawColorRect(0.0f, 0.0f, 1600.0f / gScaleX, 900.0f / gScaleY,
        0.0f, 0.0f, 0.0f, 0.65f);

    // 2. Panel background (panel_blue.png)
    DrawTex(pTexPanel, PANEL_X, PANEL_Y, PANEL_W, PANEL_H);

    // 3. Title + body text using Nunito fonts
    s8 fontRegular = (tutFontRegular >= 0) ? tutFontRegular : fontId;
    s8 fontSemibold = (tutFontSemibold >= 0) ? tutFontSemibold : fontRegular;

    if (fontRegular >= 0)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        const TutPage& pg = PAGES[g_page];

        // Title using Nunito Semibold (larger, bold appearance)
        float titleScale = 1.25f;
        int   titleLen = static_cast<int>(strlen(pg.title));
        float titleX = -(titleLen * 11.0f * titleScale) / (2.0f * 800.0f);
        float titleY = 0.32f;
        // Use semibold for title with gold color
        AEGfxPrint(fontSemibold, pg.title, titleX, titleY, titleScale,
            1.0f, 0.88f, 0.35f, 1.0f);

        // Body lines using Nunito Regular
        float bodyScale = 0.82f;
        float lineY = 0.13f;
        float lineStep = 0.083f;
        for (int i = 0; i < pg.lineCount; i++)
        {
            if (pg.lines[i][0] == '\0') { lineY -= lineStep * 0.5f; continue; }
            int   len = static_cast<int>(strlen(pg.lines[i]));
            float lx = -(len * 10.5f * bodyScale) / (2.0f * 800.0f);
            AEGfxPrint(fontRegular, pg.lines[i], lx, lineY, bodyScale,
                0.95f, 0.92f, 0.82f, 1.0f);
            lineY -= lineStep;
        }

        // Page number e.g. "3 / 8" using Nunito Regular
        char pageStr[16];
        sprintf_s(pageStr, sizeof(pageStr), "%d / %d", g_page + 1, PAGE_COUNT);
        AEGfxPrint(fontRegular, pageStr, -0.03f, -0.36f, 0.70f, 1.0f, 0.88f, 0.35f, 0.8f);

        // ESC hint using Nunito Regular (smaller)
        AEGfxPrint(fontRegular, "[ ESC ] or click outside to close",
            -0.25f, -0.43f, 0.55f, 0.85f, 0.82f, 0.72f, 0.75f);
    }

    // 4. Left arrow
    {
        float alpha = (g_page > 0) ? 1.0f : 0.35f;
        float tint = (g_hoverL && g_page > 0) ? 1.3f : 1.0f;
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(tint, tint, tint, alpha);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetTransparency(1.0f);
        if (pTexArrowLeft)
        {
            AEMtx33 sc, tr, tf;
            AEGfxTextureSet(pTexArrowLeft, 0, 0);
            AEMtx33Scale(&sc, ARROW_SIZE * gScaleX, ARROW_SIZE * gScaleY);
            AEMtx33Trans(&tr, ARROW_LEFT_X, ARROW_Y);
            AEMtx33Concat(&tf, &tr, &sc);
            AEGfxSetTransform(tf.m);
            AEGfxMeshDraw(pMeshQuad, AE_GFX_MDM_TRIANGLES);
        }
    }

    // 5. Right arrow
    {
        float alpha = (g_page < PAGE_COUNT - 1) ? 1.0f : 0.35f;
        float tint = (g_hoverR && g_page < PAGE_COUNT - 1) ? 1.3f : 1.0f;
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(tint, tint, tint, alpha);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetTransparency(1.0f);
        if (pTexArrowRight)
        {
            AEMtx33 sc, tr, tf;
            AEGfxTextureSet(pTexArrowRight, 0, 0);
            AEMtx33Scale(&sc, ARROW_SIZE * gScaleX, ARROW_SIZE * gScaleY);
            AEMtx33Trans(&tr, ARROW_RIGHT_X, ARROW_Y);
            AEMtx33Concat(&tf, &tr, &sc);
            AEGfxSetTransform(tf.m);
            AEGfxMeshDraw(pMeshQuad, AE_GFX_MDM_TRIANGLES);
        }
    }

    // 6. Page indicator dots (iconCircle_brown.png)
    if (pTexDot && pMeshQuad)
    {
        float totalW = (PAGE_COUNT - 1) * DOT_SPACING;
        float startX = -totalW * 0.5f;
        for (int i = 0; i < PAGE_COUNT; i++)
        {
            float dotX = startX + i * DOT_SPACING;
            float dotY = DOT_Y;
            float sz = (i == g_page) ? DOT_SIZE * 1.4f : DOT_SIZE;
            float alpha = (i == g_page) ? 1.0f : 0.45f;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, alpha);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxSetTransparency(1.0f);
            AEGfxTextureSet(pTexDot, 0, 0);

            AEMtx33 sc, tr, tf;
            AEMtx33Scale(&sc, sz * gScaleX, sz * gScaleY);
            AEMtx33Trans(&tr, dotX, dotY);
            AEMtx33Concat(&tf, &tr, &sc);
            AEGfxSetTransform(tf.m);
            AEGfxMeshDraw(pMeshQuad, AE_GFX_MDM_TRIANGLES);
        }
    }
}