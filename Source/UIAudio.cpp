#include "UIAudio.h"
#include "AEAudio.h"
#include "Rhythm.h"

// ---------------- Audio State ----------------
static AEAudio sClickSound;
static AEAudio sToggleSound;
static AEAudioGroup sUIAudioGroup;
static bool sSFXEnabled = true;

// ---------------- Init ----------------
void UIAudio_Init()
{
    // Create audio group (REQUIRED)
    sUIAudioGroup = AEAudioCreateGroup();

    // Load UI sounds
    sClickSound = AEAudioLoadSound("Assets/ui_click.wav");
    sToggleSound = AEAudioLoadSound("Assets/ui_toggle.wav");
}

// ---------------- Playback ----------------
void UIAudio_PlayClick()
{
    if (sSFXEnabled && AEAudioIsValidAudio(sClickSound))
    {
        AEAudioPlay(sClickSound, sUIAudioGroup, 1.0f, 1.0f, 0);
    }
}

void UIAudio_PlayToggle()
{
    if (sSFXEnabled && AEAudioIsValidAudio(sToggleSound))
    {
        AEAudioPlay(sToggleSound, sUIAudioGroup, 1.0f, 1.0f, 0);
    }
}

// ---------------- Settings ----------------
void UIAudio_EnableSFX(bool enable)
{
    sSFXEnabled = enable;
}


void UIAudio_SetMusicEnabled(bool enabled)
{
    Rhythm_SetMusicEnabled(enabled);
}

// Hard-stop Rhythm music immediately on state transitions (e.g. returning to
// the start screen).  Rhythm_SetMusicEnabled(false) only PAUSES the group —
// AEAudio keeps the channel active and it bleeds into the next state.
// Rhythm_Stop() calls AEAudioStopGroup which fully kills playback.
void UIAudio_StopMusic()
{
    Rhythm_Stop();
}

bool UIAudio_SFXEnabled()
{
    return sSFXEnabled;
}