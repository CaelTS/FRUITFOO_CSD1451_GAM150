#include "Credits.h"
#include "AEEngine.h"
#include "GameStateManager.h"
#include "Main.h"

// ---------------------------------------------------------------
// Shared mesh (created in Main.cpp)
// ---------------------------------------------------------------
extern AEGfxVertexList* g_pMeshFullScreen;

// ---------------------------------------------------------------
// Assets
// ---------------------------------------------------------------
static AEGfxTexture* g_creditsBackground = nullptr;
static AEGfxTexture* g_closeBtnTex = nullptr;
static s8            g_creditsFont = -1;   // role label font  (smaller)
static s8            g_creditsFontName = -1;   // name / header font (larger)

// ---------------------------------------------------------------
// Scroll state
// ---------------------------------------------------------------
static const float SCROLL_SPEED = 55.0f;   // px per second (upward)
static const float LINE_H_LABEL = 36.0f;
static const float LINE_H_NAME = 54.0f;
static const float SECTION_GAP = 28.0f;
static const float HEADER_GAP = 50.0f;

// g_scrollY = world-Y of the TOP edge of the first credit line.
// Starts below the screen and increases each frame (scrolls upward).
static float g_scrollY = 0.0f;
static float g_totalH = 0.0f;
static bool  g_done = false;

// ---------------------------------------------------------------
// Credit entry table
// ---------------------------------------------------------------
enum CreditLineType { CL_HEADER, CL_LABEL, CL_NAME, CL_GAP };

struct CreditLine
{
    CreditLineType type;
    const char* text;
};

static const CreditLine k_lines[] =
{
    // ---- Team ----
    { CL_LABEL,  "DIRECTOR and PROGRAMMER"              },
    { CL_NAME,   "Daniel"                               },
    { CL_GAP,    nullptr                                },
    { CL_LABEL,  "PRODUCER and DESIGN LEAD"             },
    { CL_NAME,   "Putra"                                },
    { CL_GAP,    nullptr                                },
    { CL_LABEL,  "TECHNICAL and AUDIO LEAD"             },
    { CL_NAME,   "Thierry"                              },
    { CL_GAP,    nullptr                                },
    { CL_LABEL,  "LEVEL DESIGNER, GAMEPLAY PROGRAMMER"  },
    { CL_NAME,   "Tiara, Simone"                        },

    // ---- Faculty ----
    { CL_GAP,    nullptr                                },
    { CL_GAP,    nullptr                                },
    { CL_HEADER, "Faculty and Advisors"                 },
    { CL_GAP,    nullptr                                },
    { CL_LABEL,  "Design and Production"                },
    { CL_NAME,   "Prof. Gerald, Prof. Tommy, Dr. Soroor"},
    { CL_GAP,    nullptr                                },
    { CL_LABEL,  "Art"                                  },
    { CL_NAME,   "Prof. Gerald, Prof. Tommy, Dr. Soroor"},
    { CL_GAP,    nullptr                                },
    { CL_LABEL,  "Programming"                          },
    { CL_NAME,   "Prof. Gerald, Prof. Tommy, Dr. Soroor"},
    { CL_GAP,    nullptr                                },
    { CL_LABEL,  "Audio"                                },
    { CL_NAME,   "Prof. Gerald, Prof. Tommy, Dr. Soroor"},
    { CL_GAP,    nullptr                                },
    { CL_LABEL,  "Lab management and IT"                },
    { CL_NAME,   "Prof. Gerald, Prof. Tommy, Dr. Soroor"},

    // ---- Institution ----
    { CL_GAP,    nullptr                                },
    { CL_GAP,    nullptr                                },
    { CL_LABEL,  "Created at"                           },
    { CL_NAME,   "DigiPen Institute of Technology Singapore" },
    { CL_GAP,    nullptr                                },
    { CL_HEADER, "PRESIDENT"                            },
    { CL_NAME,   "CLAUDE COMAIR"                        },
    { CL_GAP,    nullptr                                },
    { CL_HEADER, "EXECUTIVES"                           },
    { CL_NAME,   "JASON CHU  SAMIR ABOU SAMRA  MICHELE COMAIR" },
    { CL_NAME,   "ANGELA KUGLER   ERIK MOHRMANN"        },
    { CL_NAME,   "BENJAMIN ELLINGER   MELVIN GONSALVEZ" },

    // ---- Credits ----
    { CL_GAP,    nullptr                                },
    { CL_GAP,    nullptr                                },
    { CL_HEADER, "CREDITS"                              },
    { CL_GAP,    nullptr                                },
    { CL_LABEL,  "Sprites and Icons"                    },
    { CL_NAME,   "Game icon pack by Kenney Vleugels (www.kenney.nl)" },
    { CL_GAP,    nullptr                                },
    { CL_LABEL,  "Songs"                                },
    { CL_NAME,   "Soundly (getsoundly.com)"             },

    // ---- Footer ----
    { CL_GAP,    nullptr                                },
    { CL_GAP,    nullptr                                },
    { CL_LABEL,  "WWW.DIGIPEN.EDU"                      },
    { CL_LABEL,  "All content (c) 2026 DigiPen Institute of Technology Singapore." },
    { CL_LABEL,  "All Rights Reserved."                 },
    { CL_GAP,    nullptr                                },
    { CL_GAP,    nullptr                                },
    { CL_GAP,    nullptr                                },
};
static const int k_lineCount = sizeof(k_lines) / sizeof(k_lines[0]);

// ---------------------------------------------------------------
// Helper: pixel height of one entry
// ---------------------------------------------------------------
static float LineHeight(CreditLineType t)
{
    switch (t)
    {
    case CL_HEADER: return LINE_H_NAME + SECTION_GAP;
    case CL_LABEL:  return LINE_H_LABEL;
    case CL_NAME:   return LINE_H_NAME;
    case CL_GAP:    return SECTION_GAP;
    default:        return 0.0f;
    }
}

// ---------------------------------------------------------------
// GSM: Load
// ---------------------------------------------------------------
void Credits_Load()
{
    g_creditsBackground = AEGfxTextureLoad("Assets/MainMenu_Background.png");
    g_closeBtnTex = AEGfxTextureLoad("Assets/cross.png");

    g_creditsFont = AEGfxCreateFont("Assets/Crayon pastel.otf", 22);
    g_creditsFontName = AEGfxCreateFont("Assets/Crayon pastel.otf", 34);
}

// ---------------------------------------------------------------
// GSM: Initialize
// ---------------------------------------------------------------
void Credits_Initialize()
{
    g_done = false;

    // Total pixel height of all lines stacked
    g_totalH = 0.0f;
    for (int i = 0; i < k_lineCount; i++)
        g_totalH += LineHeight(k_lines[i].type);

    // g_scrollY = world-Y of the TOP edge of the first line (AE: +Y = up).
    // Start at -450 (screen bottom) so the first line enters from the bottom edge.
    // Each frame g_scrollY increases (SCROLL_SPEED * dt), scrolling the block upward.
    // End condition: g_scrollY > 450 + totalH  (last line has exited the top).
    g_scrollY = -450.0f;
}

// ---------------------------------------------------------------
// GSM: Update
// ---------------------------------------------------------------
void Credits_Update()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();

    // Move text block upward each frame
    g_scrollY += SCROLL_SPEED * dt;

    // ESC -> exit
    if (AEInputCheckTriggered(AEVK_ESCAPE))
        g_done = true;

    // X button click (top-right)
    {
        s32   mx, my;
        AEInputGetCursorPosition(&mx, &my);
        float wx = (float)mx - 800.0f;
        float wy = 450.0f - (float)my;
        const float BTNX = 720.0f, BTNY = 400.0f, BTNR = 30.0f;
        if (AEInputCheckTriggered(AEVK_LBUTTON) &&
            wx >= BTNX - BTNR && wx <= BTNX + BTNR &&
            wy >= BTNY - BTNR && wy <= BTNY + BTNR)
        {
            g_done = true;
        }
    }

    // Transition once the top of the block has scrolled past the top of the screen.
    // g_scrollY is the top edge of the first line; when it exceeds +450 all text is gone.
    if (g_scrollY > 450.0f + g_totalH)
        g_done = true;

    if (g_done)
        nextState = GS_MAIN_SCREEN;
}

// ---------------------------------------------------------------
// GSM: Draw
// ---------------------------------------------------------------
void Credits_Draw()
{
    const float halfW = 800.0f;
    const float halfH = 450.0f;

    // ---- Background ----
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    if (g_creditsBackground && g_pMeshFullScreen)
    {
        AEMtx33 scale, trans, transform;
        AEMtx33Scale(&scale, 1600.0f, 900.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetTransparency(1.0f);
        AEGfxTextureSet(g_creditsBackground, 0, 0);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
    }

    // ---- Scrolling text ----
    // g_scrollY = world-Y of the top edge of the FIRST line.
    // Each successive line sits BELOW the previous one (subtract height).
    // +Y = up in AE world space, so subtracting moves us downward.

    if (g_creditsFont < 0 && g_creditsFontName < 0)
        return;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    float curTopY = g_scrollY;   // world-Y of the top edge of the current line

    for (int i = 0; i < k_lineCount; i++)
    {
        const CreditLine& cl = k_lines[i];
        float             lh = LineHeight(cl.type);

        // Centre of this line
        float lineCenter = curTopY - lh * 0.5f;
        curTopY -= lh;   // next line is below this one

        // Cull lines outside visible window
        if (lineCenter > halfH + 60.0f || lineCenter < -halfH - 60.0f)
            continue;

        if (cl.type == CL_GAP || cl.text == nullptr)
            continue;

        // NDC y
        float ny = lineCenter / halfH;

        switch (cl.type)
        {

        case CL_HEADER:
        {
            if (g_creditsFontName >= 0)
            {
                f32 tw = 0.0f, th = 0.0f;
                AEGfxGetPrintSize(g_creditsFontName, cl.text, 0.9f, &tw, &th);
                float nx = -(tw * 0.5f);
                AEGfxPrint(g_creditsFontName, cl.text,
                    nx, ny, 0.9f,
                    0.15f, 0.08f, 0.02f, 1.0f);
            }
            break;
        }
        case CL_LABEL:
        {
            if (g_creditsFont >= 0)
            {
                f32 tw = 0.0f, th = 0.0f;
                AEGfxGetPrintSize(g_creditsFont, cl.text, 0.75f, &tw, &th);
                float nx = -(tw * 0.5f);
                AEGfxPrint(g_creditsFont, cl.text,
                    nx, ny, 0.75f,
                    0.20f, 0.12f, 0.04f, 1.0f);
            }
            break;
        }
        case CL_NAME:
        {
            if (g_creditsFontName >= 0)
            {
                f32 tw = 0.0f, th = 0.0f;
                AEGfxGetPrintSize(g_creditsFontName, cl.text, 0.85f, &tw, &th);
                float nx = -(tw * 0.5f);
                AEGfxPrint(g_creditsFontName, cl.text,
                    nx, ny, 0.85f,
                    0.65f, 0.30f, 0.02f, 1.0f);   // gold
            }
            break;
        }
        default: break;
        }
    }

    if (g_creditsFont >= 0)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxPrint(g_creditsFont, "Press ESC or X to exit",
            -0.95f, 0.90f, 0.70f,
            0.20f, 0.12f, 0.04f, 1.0f);
    }

    // ---- X close button top-right ----
    if (g_closeBtnTex && g_pMeshFullScreen)
    {
        const float BTNX = 720.0f, BTNY = 400.0f;
        const float BTNW = 48.0f, BTNH = 48.0f;

        s32   mx, my;
        AEInputGetCursorPosition(&mx, &my);
        float wx = (float)mx - 800.0f;
        float wy = 450.0f - (float)my;
        bool hovered = (wx >= BTNX - BTNW * 0.5f && wx <= BTNX + BTNW * 0.5f &&
            wy >= BTNY - BTNH * 0.5f && wy <= BTNY + BTNH * 0.5f);

        AEMtx33 sc, tr, tf;
        AEMtx33Scale(&sc, BTNW, BTNH);
        AEMtx33Trans(&tr, BTNX, BTNY);
        AEMtx33Concat(&tf, &tr, &sc);

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        float tint = hovered ? 1.0f : 0.75f;
        AEGfxSetColorToMultiply(tint, tint, tint, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetTransparency(1.0f);
        AEGfxTextureSet(g_closeBtnTex, 0, 0);
        AEGfxSetTransform(tf.m);
        AEGfxMeshDraw(g_pMeshFullScreen, AE_GFX_MDM_TRIANGLES);
    }
}

// ---------------------------------------------------------------
// GSM: Free
// ---------------------------------------------------------------
void Credits_Free()
{
    // Nothing owned here that isn't released in Unload
}

// ---------------------------------------------------------------
// GSM: Unload
// ---------------------------------------------------------------
void Credits_Unload()
{
    if (g_creditsBackground) { AEGfxTextureUnload(g_creditsBackground); g_creditsBackground = nullptr; }
    if (g_closeBtnTex) { AEGfxTextureUnload(g_closeBtnTex);       g_closeBtnTex = nullptr; }
    if (g_creditsFont >= 0) { AEGfxDestroyFont(g_creditsFont);      g_creditsFont = -1; }
    if (g_creditsFontName >= 0) { AEGfxDestroyFont(g_creditsFontName);  g_creditsFontName = -1; }
}