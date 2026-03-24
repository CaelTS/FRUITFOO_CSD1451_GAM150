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
extern s8 fontId;          // Crayon pastel font from Main.cpp
extern AEGfxVertexList* g_pMeshFullScreen;

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------
static AEGfxTexture* pTexTutBtn = nullptr; // Tutorial.png
static AEGfxTexture* pTexTutBtnHover = nullptr; // Tutorial_selected.png
static AEGfxTexture* pTexPanel = nullptr; // panel_blue.png
static AEGfxTexture* pTexArrowLeft = nullptr; // arrowBrown_left.png
static AEGfxTexture* pTexArrowRight = nullptr; // arrowBrown_right.png
static AEGfxTexture* pTexDot = nullptr; // iconCircle_brown.png
static AEGfxVertexList* pMeshQuad = nullptr;
static s8 tutFont = -1;

// ---------------------------------------------------------------------------
// Layout constants  (pixel-space, world origin = screen centre)
// ---------------------------------------------------------------------------

// ---- Tutorial button (above Continue / New Game) ----
static const float BTN_W = 190.0f;
static const float BTN_H = 41.0f;
static const float BTN_HOVER_W = 211.0f;
static const float BTN_HOVER_H = 61.0f;

// Base positions (no-save / has-save set in Tutorial_Load / StartScreen_Init)
static float g_btnX = 0.0f;
static float g_btnY = 0.0f;
static bool  g_btnHovered = false;

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
static const float DOT_Y = -245.0f;  // just below content area
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
    // Up to 6 lines of body text
    const char* lines[6];
    int         lineCount;
};

static const TutPage PAGES[] =
{
    {
        "Welcome!",
        {
            "Welcome to Fruit Stall!",
            "Run your own market stall,",
            "grow fruit on your farm, and",
            "sell to earn coins.",
            "Use the arrows to navigate.",
            ""
        },
        5
    },
    {
        "Movement & Controls",
        {
            "Press  M  to open / close the",
            "side menu at any time.",
            "",
            "Press  ESC  to pause the game",
            "and access the main menu.",
            ""
        },
        4
    },
    {
        "The Farm - Planting",
        {
            "Open the menu and click a",
            "plot slot to plant a seed.",
            "",
            "Plot 1 is unlocked by default.",
            "Purchase upgrades to unlock",
            "more plots."
        },
        6
    },
    {
        "The Farm - Growing",
        {
            "Seeds grow over time.",
            "Watch the growth indicator",
            "on each plot.",
            "",
            "At 50% growth a Rhythm game",
            "mini-event will trigger!"
        },
        6
    },
    {
        "Rhythm Mini-Game",
        {
            "Hit notes in time to boost",
            "your crop's growth.",
            "",
            "Great performance = bonus",
            "fruit at harvest time.",
            "Poor = growth penalty."
        },
        6
    },
    {
        "Harvesting & Selling",
        {
            "When a plot glows it is ready.",
            "Press  SPACE  to harvest all",
            "ready crops into your inventory.",
            "",
            "Drag fruit into your crates",
            "to put them up for sale!"
        },
        6
    },
    {
        "Upgrades",
        {
            "Open the menu and visit the",
            "Upgrades section.",
            "",
            "Buy Speed Boost, Crate Storage,",
            "Faster Growth, and more to",
            "improve your stall!"
        },
        6
    },
    {
        "Tips",
        {
            "Keep all plots planted for",
            "maximum coin income.",
            "",
            "Check your inventory regularly",
            "and never let crates run empty.",
            "Good luck!"
        },
        6
    }
};

static const int PAGE_COUNT = static_cast<int>(sizeof(PAGES) / sizeof(PAGES[0]));

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Build a simple UV quad (reused everywhere).
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
    AEMtx33Trans(&tr, cx * gScaleX, cy * gScaleY);
    AEMtx33Concat(&tf, &tr, &sc);
    AEGfxSetTransform(tf.m);
    AEGfxMeshDraw(pMeshQuad, AE_GFX_MDM_TRIANGLES);
}

// Draw a colour-only rect (used for the dim overlay).
static void DrawColorRect(float cx, float cy, float w, float h, float r, float g, float b, float a)
{
    if (!g_pMeshFullScreen) return;
    AEMtx33 sc, tr, tf;
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(r, g, b, a);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEMtx33Scale(&sc, w * gScaleX, h * gScaleY);
    AEMtx33Trans(&tr, cx * gScaleX, cy * gScaleY);
    AEMtx33Concat(&tf, &tr, &sc);
    AEGfxSetTransform(tf.m);
    AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void Tutorial_Load()
{
    pTexTutBtn = AEGfxTextureLoad("Assets/Tutorial.png");
    pTexTutBtnHover = AEGfxTextureLoad("Assets/Tutorial_selected.png");
    pTexPanel = AEGfxTextureLoad("Assets/panel_blue.png");
    pTexArrowLeft = AEGfxTextureLoad("Assets/arrowBrown_left.png");
    pTexArrowRight = AEGfxTextureLoad("Assets/arrowBrown_right.png");
    pTexDot = AEGfxTextureLoad("Assets/iconCircle_brown.png");
    pMeshQuad = MakeTutQuad();

    // Font fallback — use the same font as the game
    tutFont = AEGfxCreateFont("Assets/Crayon pastel.otf", 22);
    if (tutFont < 0)
        tutFont = AEGfxCreateFont("Assets/liberation-mono.ttf", 20);

    // Position tutorial button above Continue (y=155, same x as continue)
    // These mirror continueButton positions in StartScreen_Init
    g_btnX = -520.0f - 50.0f;   // logoPosX - 50 (same as continueButton.x)
    g_btnY = 155.0f;             // 55px above continueButton.y (100)

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

void Tutorial_Update(float slideOffset)
{
    // Convert mouse position to world space
    int mx, my;
    AEInputGetCursorPosition(&mx, &my);
    float wx = static_cast<float>(mx) - 800.0f;
    float wy = 450.0f - static_cast<float>(my);

    // ---- Tutorial button (only when panel is closed) ----
    if (!g_tutOpen)
    {
        float bx = (g_btnX - slideOffset) * gScaleX;
        float by = g_btnY * gScaleY;
        float hw = BTN_W * gScaleX * 0.5f;
        float hh = BTN_H * gScaleY * 0.5f;

        g_btnHovered = (wx >= bx - hw && wx <= bx + hw &&
            wy >= by - hh && wy <= by + hh);

        if (g_btnHovered && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            g_tutOpen = true;
            g_page = 0;
        }
        return; // no panel input needed
    }

    // ---- Panel is open: handle navigation ----

    // ESC closes
    if (AEInputCheckTriggered(AEVK_ESCAPE))
    {
        g_tutOpen = false;
        return;
    }

    // Left arrow
    float lx = ARROW_LEFT_X * gScaleX;
    float rx = ARROW_RIGHT_X * gScaleX;
    float ay = ARROW_Y * gScaleY;
    float hs = ARROW_SIZE * gScaleX * 0.5f;

    g_hoverL = (wx >= lx - hs && wx <= lx + hs &&
        wy >= ay - hs && wy <= ay + hs);
    g_hoverR = (wx >= rx - hs && wx <= rx + hs &&
        wy >= ay - hs && wy <= ay + hs);

    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        if (g_hoverL && g_page > 0)
            g_page--;
        else if (g_hoverR && g_page < PAGE_COUNT - 1)
            g_page++;
        else if (!g_hoverL && !g_hoverR)
        {
            // Click outside panel to dismiss
            float px = PANEL_X * gScaleX;
            float py = PANEL_Y * gScaleY;
            float phw = PANEL_W * gScaleX * 0.5f;
            float phh = PANEL_H * gScaleY * 0.5f;
            bool insidePanel = (wx >= px - phw && wx <= px + phw &&
                wy >= py - phh && wy <= py + phh);
            if (!insidePanel)
                g_tutOpen = false;
        }
    }

    // Keyboard shortcuts
    if (AEInputCheckTriggered(AEVK_LEFT) && g_page > 0)            g_page--;
    if (AEInputCheckTriggered(AEVK_RIGHT) && g_page < PAGE_COUNT - 1) g_page++;
}

void Tutorial_Draw(float slideOffset, float fadeOut)
{
    if (!pMeshQuad) return;

    // ---- Tutorial button (always drawn when panel is closed) ----
    if (!g_tutOpen)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetTransparency(fadeOut);

        float drawX = g_btnX - slideOffset;
        float drawW = g_btnHovered ? BTN_HOVER_W : BTN_W;
        float drawH = g_btnHovered ? BTN_HOVER_H : BTN_H;
        AEGfxTexture* btnTex = g_btnHovered ? pTexTutBtnHover : pTexTutBtn;

        if (btnTex)
        {
            AEMtx33 sc, tr, tf;
            AEGfxTextureSet(btnTex, 0, 0);
            AEMtx33Scale(&sc, drawW * gScaleX, drawH * gScaleY);
            AEMtx33Trans(&tr, drawX * gScaleX, g_btnY * gScaleY);
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

    // 3. Title text
    s8 fnt = (tutFont >= 0) ? tutFont : fontId;
    if (fnt >= 0)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        const TutPage& pg = PAGES[g_page];

        // Title  (centred by approximate char-width formula)
        float titleScale = 1.0f;
        int   titleLen = static_cast<int>(strlen(pg.title));
        float titleX = -(titleLen * 14.0f * titleScale) / (2.0f * 800.0f);
        float titleY = 0.30f;
        AEGfxPrint(fnt, pg.title, titleX, titleY, titleScale,
            1.0f, 0.95f, 0.75f, 1.0f);

        // Horizontal rule hint
        AEGfxPrint(fnt, "------------------------",
            -0.19f, 0.22f, 0.7f, 0.6f, 0.55f, 0.40f, 1.0f);

        // Body lines
        float bodyScale = 0.75f;
        float lineY = 0.15f;
        float lineStep = 0.09f;
        for (int i = 0; i < pg.lineCount; i++)
        {
            if (pg.lines[i][0] == '\0') { lineY -= lineStep * 0.5f; continue; }
            int   len = static_cast<int>(strlen(pg.lines[i]));
            float lx = -(len * 11.0f * bodyScale) / (2.0f * 800.0f);
            AEGfxPrint(fnt, pg.lines[i], lx, lineY, bodyScale,
                0.15f, 0.10f, 0.05f, 1.0f);
            lineY -= lineStep;
        }

        // Page number  e.g. "3 / 8"
        char pageStr[16];
        sprintf_s(pageStr, sizeof(pageStr), "%d / %d", g_page + 1, PAGE_COUNT);
        AEGfxPrint(fnt, pageStr, -0.03f, -0.38f, 0.65f, 0.5f, 0.45f, 0.35f, 1.0f);

        // ESC hint
        AEGfxPrint(fnt, "ESC or click outside to close",
            -0.22f, -0.44f, 0.5f, 0.55f, 0.50f, 0.40f, 1.0f);
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
            AEMtx33Trans(&tr, ARROW_LEFT_X * gScaleX, ARROW_Y * gScaleY);
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
            AEMtx33Trans(&tr, ARROW_RIGHT_X * gScaleX, ARROW_Y * gScaleY);
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

            // Current page dot is larger and fully opaque
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
            AEMtx33Trans(&tr, dotX * gScaleX, dotY * gScaleY);
            AEMtx33Concat(&tf, &tr, &sc);
            AEGfxSetTransform(tf.m);
            AEGfxMeshDraw(pMeshQuad, AE_GFX_MDM_TRIANGLES);
        }
    }
}