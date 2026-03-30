#pragma once
void UIAudio_Init();
void UIAudio_PlayClick();
void UIAudio_PlayToggle();
void UIAudio_EnableSFX(bool enable);
void UIAudio_SetMusicEnabled(bool enabled);
void UIAudio_StopMusic();

// getters — used by UI to sync toggle display with persisted state
bool UIAudio_SFXEnabled();
bool UIAudio_MusicEnabled();