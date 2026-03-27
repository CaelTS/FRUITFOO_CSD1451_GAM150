#pragma once
void UIAudio_Init();
void UIAudio_PlayClick();
void UIAudio_PlayToggle();
void UIAudio_EnableSFX(bool enable);
void UIAudio_SetMusicEnabled(bool enabled);
void UIAudio_StopMusic();

// getter
bool UIAudio_SFXEnabled();