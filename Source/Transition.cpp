#include "Transition.h"
#include "AEEngine.h"

static float  g_timer = 0.0f;
static bool   g_active = false, g_phase = false;
static int    g_fromState, g_toState;
static const float DURATION = 0.3f;

void TR_Start(int fromState, int toState)
{
	g_fromState = fromState;
	g_toState = toState;
	g_timer = 0.0f;
	g_active = true;
	g_phase = false;
}

void TR_ResetTimer()          // call this right after state swap
{
	g_timer = 0.0f;
	g_phase = false;          // make sure we begin fade-from-black at 0
}

bool TR_IsActive() { return g_active; }

bool TR_Update()
{
	if (!g_active) return false;
	float dt = (float)AEFrameRateControllerGetFrameTime();
	g_timer += dt / DURATION;
	if (g_timer >= 1.0f) { g_timer = 1.0f; g_active = false; }
	return !g_active;
}

// 0…1  :  0 = old screen full, 1 = new screen full
float TR_GetAlpha() { return g_timer; }