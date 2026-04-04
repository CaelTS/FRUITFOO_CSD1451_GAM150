#pragma once

#ifndef RHYTHM_H
#define RHYTHM_H

#include <vector>
#include <string>

// Note types for visual variety
enum NoteType {
    NOTE_NORMAL = 0,   // Regular note
    NOTE_PREMIUM          // Bonus note (worth more)
};

// Hit accuracy
enum HitRating {
    HIT_PERFECT,
    HIT_GOOD,
    HIT_MISS,
    HIT_NONE
};

// Single note structure
struct RhythmNote {
    NoteType type;
    float hitTime;       // When note should be hit (seconds)
    float xPosition;     // Current X position
    bool hit;
    bool missed;
    HitRating rating;
    float duration;      // For long notes
};

// Score data
struct RhythmScore {
    int perfectHits;
    int goodHits;
    int misses;
    int combo;
    int maxCombo;
    int totalScore;
};

// Difficulty levels
enum RhythmDifficulty {
    DIFFICULTY_EASY = 0,
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD
};

// Per-difficulty tuning parameters
struct RhythmDifficultyConfig {
    float noteSpeed;            // Pixels per second notes travel
    float minSpawnInterval;     // Minimum seconds between notes
    float maxSpawnInterval;     // Maximum seconds between notes
    float doubleNoteChance;     // 0-1 probability of a quick double note
    float premiumNoteChance;    // 0-1 probability of a premium note

    // ---------------------------------------------------------------
    // MODIFY THESE: paths to each difficulty's audio and background
    // ---------------------------------------------------------------
    const char* audioFile;      // Path to song audio file
    const char* backgroundFile; // Path to background image for gameplay
    // ---------------------------------------------------------------
};

// ================= DIFFICULTY =================

void Rhythm_SetDifficulty(RhythmDifficulty difficulty);
RhythmDifficulty Rhythm_GetDifficulty();
const RhythmDifficultyConfig& Rhythm_GetDifficultyConfig();

// ================= LIFECYCLE =================

void Rhythm_Load();
void Rhythm_Initialize();
void Rhythm_Update();
void Rhythm_Render();
void Rhythm_Free();
void Rhythm_Unload();

// ================= GAMEPLAY =================

void Rhythm_Start();
void Rhythm_Stop();
bool Rhythm_IsPlaying();
void Rhythm_Hit();
bool Rhythm_IsSongFinished();

// ================= GETTERS =================

const RhythmScore& Rhythm_GetScore();
float Rhythm_GetSongDuration();
float Rhythm_GetCurrentTime();

// ================= REWARDS =================

enum RhythmRewardTier {
    REWARD_POOR = 0,
    REWARD_AVERAGE,
    REWARD_GOOD
};

RhythmRewardTier Rhythm_GetRewardTier();

// ================= SETTINGS =================
void Rhythm_SetMusicEnabled(bool enabled);

// Call this before starting the rhythm game to set which fruit/seed textures to use.
// seedType: 0 = Apple, 1 = Pear, 2 = Banana  (matches SeedType enum in UI.h)
void Rhythm_SetSeedType(int seedType);


#endif // RHYTHM_H