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
// (reward multipliers are intentionally left as placeholders for later)
struct RhythmDifficultyConfig {
    float noteSpeed;            // Pixels per second notes travel
    float minSpawnInterval;     // Minimum seconds between notes
    float maxSpawnInterval;     // Maximum seconds between notes
    float doubleNoteChance;     // 0�1 probability of a quick double note
    float premiumNoteChance;    // 0�1 probability of a premium note
};

// ================= DIFFICULTY =================

// Set difficulty before calling Rhythm_Start(). Defaults to DIFFICULTY_MEDIUM.
void Rhythm_SetDifficulty(RhythmDifficulty difficulty);
RhythmDifficulty Rhythm_GetDifficulty();

// Returns the config that is currently active (useful for UI display)
const RhythmDifficultyConfig& Rhythm_GetDifficultyConfig();

// ================= LIFECYCLE =================

void Rhythm_Load();
void Rhythm_Initialize();
void Rhythm_Update();
void Rhythm_Render();
void Rhythm_Free();
void Rhythm_Unload();

// ================= GAMEPLAY =================

void Rhythm_Start();   // Starts the default chart
void Rhythm_Stop();
bool Rhythm_IsPlaying();
void Rhythm_Hit();     // Call when SPACE is pressed
bool Rhythm_IsSongFinished();  // Returns true when song + all notes are done

// ================= GETTERS =================

const RhythmScore& Rhythm_GetScore();
float Rhythm_GetSongDuration();    // Total duration of the song
float Rhythm_GetCurrentTime();     // Current playback time

// ================= REWARDS =================

enum RhythmRewardTier {
    REWARD_POOR = 0,   // No reward (Easy only)
    REWARD_AVERAGE,    // 1 seed / 1-2 fruits depending on difficulty
    REWARD_GOOD        // 2 fruits / fruits+gold depending on difficulty
};

// Call after Rhythm_IsSongFinished() returns true to get the reward tier.
RhythmRewardTier Rhythm_GetRewardTier();

#endif // RHYTHM_H