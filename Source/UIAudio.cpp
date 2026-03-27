#include "UIAudio.h"
#include "AEAudio.h"
#include "Rhythm.h"
#include "Main.h"
#include <cstdio>

// ---------------- Audio State ----------------
static AEAudio sClickSound;
static AEAudio sToggleSound;
static AEAudioGroup sUIAudioGroup;
static bool sSFXEnabled = true;
static bool sMusicEnabled = true;

// ---------------- Persistence ----------------
// Shares Assets/audio_settings.txt with StartScreen so all audio prefs
// are stored in one file and survive restarts.
static const char* AUDIO_SETTINGS_FILE = "Assets/audio_settings.txt";

static void UIAudio_SaveSettings()
{
    FILE* f = nullptr;
    if (fopen_s(&f, AUDIO_SETTINGS_FILE, "w") != 0 || !f) return;
    fprintf(f, "music_enabled=%d\n", sMusicEnabled ? 1 : 0);
    fprintf(f, "sfx_enabled=%d\n", sSFXEnabled ? 1 : 0);
    fclose(f);
}

static void UIAudio_LoadSettings()
{
    FILE* f = nullptr;
    if (fopen_s(&f, AUDIO_SETTINGS_FILE, "r") != 0 || !f) return;
    char line[64] = {};
    while (fgets(line, sizeof(line), f))
    {
        int val = 1;
        if (sscanf_s(line, "music_enabled=%d", &val) == 1) sMusicEnabled = (val != 0);
        else if (sscanf_s(line, "sfx_enabled=%d", &val) == 1) sSFXEnabled = (val != 0);
    }
    fclose(f);
}

// ---------------- Init ----------------
void UIAudio_Init()
{
    // Load persisted preferences so toggles reflect the saved state
    UIAudio_LoadSettings();

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
        AEAudioPlay(sClickSound, sUIAudioGroup, 1.0f, 1.0f, 0);
}

void UIAudio_PlayToggle()
{
    if (sSFXEnabled && AEAudioIsValidAudio(sToggleSound))
        AEAudioPlay(sToggleSound, sUIAudioGroup, 1.0f, 1.0f, 0);
}

// ---------------- Settings ----------------
void UIAudio_EnableSFX(bool enable)
{
    sSFXEnabled = enable;
    UIAudio_SaveSettings();
}

// Controls ALL in-game music: Rhythm track + main screen BGM.
// Persists the preference to audio_settings.txt so it survives restarts.
void UIAudio_SetMusicEnabled(bool enabled)
{
    sMusicEnabled = enabled;
    Rhythm_SetMusicEnabled(enabled);  // pause/resume Rhythm group
    MainBGM_SetEnabled(enabled);      // stop/start main screen BGM
    UIAudio_SaveSettings();
}

// Hard-stop Rhythm music immediately on state transitions (e.g. going back
// to the start screen). Rhythm_Stop() calls AEAudioStopGroup, fully killing
// playback — unlike SetMusicEnabled which only pauses.
void UIAudio_StopMusic()
{
    Rhythm_Stop();
    // Does NOT touch MainBGM — MainScreen_Free() handles that on state exit.
}

bool UIAudio_SFXEnabled()
{
    return sSFXEnabled;
}

bool UIAudio_MusicEnabled()
{
    return sMusicEnabled;
}