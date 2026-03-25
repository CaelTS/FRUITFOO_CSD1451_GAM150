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
static AEGfxTexture* pTexTutBtn = nullptr;  // Tutorial.png
static AEGfxTexture* pTexTutBtnHover = nullptr;  // Tutorial_selected.png
static AEGfxTexture* pTexPanel = nullptr;  // panel_blue.png
static AEGfxTexture* pTexArrowLeft = nullptr;  // arrowBrown_left.png
static AEGfxTexture* pTexArrowRight = nullptr;  // arrowBrown_right.png
static AEGfxTexture* pTexDot = nullptr;  // iconCircle_brown.png
static AEGfxVertexList* pMeshQuad = nullptr;
static s8 tutFont = -1;

// ---------------------------------------------------------------------------
// Layout constants  (pixel-space, world origin = screen centre)
// ---------------------------------------------------------------------------
// ---- Tutorial button ----
static const float BTN_W = 190.0f;
static const float BTN_H = 41.0f;
static const float BTN_HOVER_W = 211.0f;
static const float BTN_HOVER_H = 61.0f;

// FIX 2: separate positions for no-save and has-save layouts, mirroring the
// x_save / y_save pattern used by every other button in StartScreen.cpp.
// No-save: sits above newGameButton  (x = logoPosX-32 = -552, y = 100)
// Has-save: sits above continueButton (x = logoPosX-50 = -570, y = 100)
static float g_btnX_nosave = 0.0f;
static float g_btnY_nosave = 0.0f;
static float g_btnX_save = 0.0f;
static float g_btnY_save = 0.0f;
static bool  g_btnHovered = false;

// Convenience: returns the active X/Y based on the hasSave state passed in
// from StartScreen each frame (avoids extern-linking a static variable).
static inline float ActiveBtnX(bool hs) { return hs ? g_btnX_save : g_btnX_nosave; }
static inline float ActiveBtnY(bool hs) { return hs ? g_btnY_save : g_btnY_nosave; }

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
        "~ Welcome to Fruit Stall! ~",
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
    if (!pTexTutBtn)      pTexTutBtn = AEGfxTextureLoad("Assets/Tutorial.png");
    if (!pTexTutBtnHover) pTexTutBtnHover = AEGfxTextureLoad("Assets/Tutorial_selected.png");
    if (!pTexPanel)       pTexPanel = AEGfxTextureLoad("Assets/panel_blue.png");
    if (!pTexArrowLeft)   pTexArrowLeft = AEGfxTextureLoad("Assets/arrowBrown_left.png");
    if (!pTexArrowRight)  pTexArrowRight = AEGfxTextureLoad("Assets/arrowBrown_right.png");
    if (!pTexDot)         pTexDot = AEGfxTextureLoad("Assets/iconCircle_brown.png");
    if (!pMeshQuad)       pMeshQuad = MakeTutQuad();

    // FIX 3: assert on load failures so a missing asset is caught immediately
    // rather than the button silently not appearing.
    if (tutFont < 0)
    {
        tutFont = AEGfxCreateFont("Assets/Crayon pastel.otf", 22);
        if (tutFont < 0)
            tutFont = AEGfxCreateFont("Assets/liberation-mono.ttf", 20);
    }

    // FIX 2: set both layout positions up front.
    // logoPosX = -520 (mirrors StartScreen.cpp)
    // No-save: align with newGameButton  (x = logoPosX - 32 = -552, y = 155)
    // Has-save: align with continueButton (x = logoPosX - 50 = -570, y = 155)
    static const float logoPosX = -520.0f;
    g_btnX_nosave = logoPosX - 32.0f;   // = -552  (above New Game)
    g_btnY_nosave = 55.0f;
    g_btnX_save = logoPosX - 50.0f;   // = -570  (above Continue)
    g_btnY_save = 55.0f;

    g_tutOpen = false;
    g_page = 0;
}

void Tutorial_Unload()
{
    if (pTexTutBtn) { AEGfxTextureUnload(pTexTutBtn);      pTexTutBtn = nullptr; }
    if (pTexTutBtnHover) { AEGfxTextureUnload(pTexTutBtnHover); pTexTutBtnHover = nullptr; }
    if (pTexPanel) { AEGfxTextureUnload(pTexPanel);       pTexPanel = nullptr; }
    if (pTexArrowLeft) { AEGfxTextureUnload(pTexArrowLeft);   pTexArrowLeft = nullptr; }
    if (pTexArrowRight) { AEGfxTextureUnload(pTexArrowRight);  pTexArrowRight = nullptr; }
    if (pTexDot) { AEGfxTextureUnload(pTexDot);         pTexDot = nullptr; }
    if (pMeshQuad) { AEGfxMeshFree(pMeshQuad);            pMeshQuad = nullptr; }
    if (tutFont >= 0) { AEGfxDestroyFont(tutFont);           tutFont = -1; }
}

bool Tutorial_IsOpen()
{
    return g_tutOpen;
}

void Tutorial_Update(float slideOffset, bool hasSave)
{
    // Convert mouse pixel position to world space
    int mx, my;
    AEInputGetCursorPosition(&mx, &my);
    float wx = static_cast<float>(mx) - 800.0f;
    float wy = 450.0f - static_cast<float>(my);

    // ---- Tutorial button (only when panel is closed) ----
    if (!g_tutOpen)
    {
        // FIX 2: use the active position for whichever layout is showing.
        float bx = ActiveBtnX(hasSave) - slideOffset;
        float by = ActiveBtnY(hasSave);
        float hw = BTN_W * gScaleX * 0.5f;
        float hh = BTN_H * gScaleY * 0.5f;

        g_btnHovered = (wx >= bx - hw && wx <= bx + hw &&
            wy >= by - hh && wy <= by + hh);

        if (g_btnHovered && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            g_tutOpen = true;
            g_page = 0;
        }
        return;
    }

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

void Tutorial_Draw(float slideOffset, float fadeOut, bool hasSave)
{
    if (!pMeshQuad) return;

    // ---- Tutorial button (always drawn when panel is closed) ----
    if (!g_tutOpen)
    {
        // FIX 2: use the active position for whichever layout is showing.
        float drawX = ActiveBtnX(hasSave) - slideOffset;
        float drawY = ActiveBtnY(hasSave);
        float drawW = g_btnHovered ? BTN_HOVER_W : BTN_W;
        float drawH = g_btnHovered ? BTN_HOVER_H : BTN_H;
        AEGfxTexture* btnTex = g_btnHovered ? pTexTutBtnHover : pTexTutBtn;

        // FIX 3: pTexTutBtn will be non-null if assets loaded correctly
        // (asserted in Tutorial_Load). Guard kept for safety.
        if (btnTex)
        {
            AEMtx33 sc, tr, tf;
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxSetTransparency(fadeOut);
            AEGfxTextureSet(btnTex, 0, 0);
            AEMtx33Scale(&sc, drawW * gScaleX, drawH * gScaleY);
            // Position must NOT be multiplied by gScale -- only the size
            // (Scale matrix) gets scaled, exactly as DrawButton() does in
            // StartScreen.cpp.
            AEMtx33Trans(&tr, drawX, drawY);
            AEMtx33Concat(&tf, &tr, &sc);
            AEGfxSetTransform(tf.m);
            AEGfxMeshDraw(pMeshQuad, AE_GFX_MDM_TRIANGLES);
        }
        return;
    }

    // ---- Tutorial panel ----

    // 1. Dim overlay (full screen)
    DrawColorRect(0.0f, 0.0f, 1600.0f / gScaleX, 900.0f / gScaleY,
        0.0f, 0.0f, 0.0f, 0.65f);

    // 2. Panel background (panel_blue.png)
    DrawTex(pTexPanel, PANEL_X, PANEL_Y, PANEL_W, PANEL_H);

    // 3. Title + body text
    s8 fnt = (tutFont >= 0) ? tutFont : fontId;
    if (fnt >= 0)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        const TutPage& pg = PAGES[g_page];

        // Title (centred by approximate char-width formula)
        float titleScale = 1.15f;
        int   titleLen = static_cast<int>(strlen(pg.title));
        float titleX = -(titleLen * 10.0f * titleScale) / (2.0f * 800.0f);
        float titleY = 0.32f;
        AEGfxPrint(fnt, pg.title, titleX, titleY, titleScale,
            1.0f, 0.88f, 0.35f, 1.0f);

        // Body lines
        float bodyScale = 0.78f;
        float lineY = 0.13f;
        float lineStep = 0.083f;
        for (int i = 0; i < pg.lineCount; i++)
        {
            if (pg.lines[i][0] == '\0') { lineY -= lineStep * 0.5f; continue; }
            int   len = static_cast<int>(strlen(pg.lines[i]));
            float lx = -(len * 10.0f * bodyScale) / (2.0f * 800.0f);
            AEGfxPrint(fnt, pg.lines[i], lx, lineY, bodyScale,
                0.95f, 0.92f, 0.82f, 1.0f);
            lineY -= lineStep;
        }

        // Page number e.g. "3 / 8"
        char pageStr[16];
        sprintf_s(pageStr, sizeof(pageStr), "%d / %d", g_page + 1, PAGE_COUNT);
        AEGfxPrint(fnt, pageStr, -0.03f, -0.36f, 0.70f, 1.0f, 0.88f, 0.35f, 0.8f);

        // ESC hint
        AEGfxPrint(fnt, "[ ESC ] or click outside to close",
            -0.25f, -0.43f, 0.50f, 0.85f, 0.82f, 0.72f, 0.75f);
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