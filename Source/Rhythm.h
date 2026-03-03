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

#endif // RHYTHM_H