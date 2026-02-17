#include "Rhythm.h"
#include "AEEngine.h"
#include "AEAudio.h"
#include <algorithm>
#include <stdio.h>
#include <cstring>

// ================= CONSTANTS =================

static const float PERFECT_WINDOW = 0.050f;  // 50ms
static const float GOOD_WINDOW = 0.100f;  // 100ms
static const float MISS_WINDOW = 0.150f;  // 150ms

static const float JUDGMENT_LINE_X = -200.0f;
static const float SPAWN_X = 900.0f;
static const float NOTE_SPEED = 500.0f;
static const float NOTE_SIZE = 40.0f;

static const int SCORE_PERFECT = 300;
static const int SCORE_GOOD = 100;

// ================= STATE =================

static bool g_isPlaying = false;
static float g_songTime = 0.0f;
static float g_songDuration = 0.0f;
static bool g_songFinished = false;
static std::vector<RhythmNote> g_notes;
static std::vector<RhythmNote> g_activeNotes;
static RhythmScore g_score = { 0 };

static AEGfxVertexList* g_pMeshNote = nullptr;
static AEGfxVertexList* g_pMeshLine = nullptr;
static s8 g_fontId = -1;

// AUDIO STATE
static AEAudio g_currentSong;
static AEAudioGroup g_musicGroup;
static float g_audioOffset = 0.0f;
static bool g_audioStarted = false;
static float g_preSongTimer = 0.0f;

// Input feedback
static float g_hitFeedbackTimer = 0.0f;
static HitRating g_lastHitRating = HIT_NONE;

static int g_nextSpawnIndex = 0;

// ================= HELPERS =================

static s32 IsValidAudio(AEAudio audio) {
    return AEAudioIsValidAudio(audio);
}

static s32 IsValidGroup(AEAudioGroup group) {
    return AEAudioIsValidGroup(group);
}

static void ResetAudio(AEAudio& audio) {
    memset(&audio, 0, sizeof(AEAudio));
}

static void ResetAudioGroup(AEAudioGroup& group) {
    memset(&group, 0, sizeof(AEAudioGroup));
}

static void UpdateScore(HitRating rating) {
    g_lastHitRating = rating;
    g_hitFeedbackTimer = 0.3f;

    switch (rating) {
    case HIT_PERFECT:
        g_score.perfectHits++;
        g_score.combo++;
        g_score.totalScore += SCORE_PERFECT + (g_score.combo * 10);
        break;
    case HIT_GOOD:
        g_score.goodHits++;
        g_score.combo++;
        g_score.totalScore += SCORE_GOOD + (g_score.combo * 5);
        break;
    case HIT_MISS:
        g_score.misses++;
        g_score.combo = 0;
        break;
    default:
        break;
    }

    if (g_score.combo > g_score.maxCombo) {
        g_score.maxCombo = g_score.combo;
    }
}

// Create chart that lasts the entire song duration
static void CreateChartForDuration(float totalDuration, float bpm) {
    g_notes.clear();
    g_nextSpawnIndex = 0;

    float beatTime = 60.0f / bpm;
    float time = g_audioOffset;

    int beatCount = 0;

    while (time < totalDuration - 2.0f) {
        RhythmNote note = {};

        if (beatCount % 8 == 7) {
            // Double note
            note.type = NOTE_NORMAL;
            note.hitTime = time;
            note.xPosition = SPAWN_X;
            note.hit = false;
            note.missed = false;
            note.rating = HIT_NONE;
            g_notes.push_back(note);

            RhythmNote note2 = {};
            note2.type = NOTE_NORMAL;
            note2.hitTime = time + beatTime * 0.5f;
            note2.xPosition = SPAWN_X;
            note2.hit = false;
            note2.missed = false;
            note2.rating = HIT_NONE;
            g_notes.push_back(note2);

            time += beatTime;
        }
        else {
            // Regular note
            note.type = (beatCount % 4 == 0) ? NOTE_GOLD : NOTE_NORMAL;
            note.hitTime = time;
            note.xPosition = SPAWN_X;
            note.hit = false;
            note.missed = false;
            note.rating = HIT_NONE;
            g_notes.push_back(note);

            time += beatTime;
        }

        beatCount++;
    }

    printf("Generated %d notes for %.1f second song at %.1f BPM\n",
        (int)g_notes.size(), totalDuration, bpm);
}

// ================= LIFECYCLE =================

void Rhythm_Load() {
    ResetAudio(g_currentSong);
    ResetAudioGroup(g_musicGroup);

    g_musicGroup = AEAudioCreateGroup();
    if (!IsValidGroup(g_musicGroup)) {
        printf("WARNING: Failed to create audio group!\n");
    }

    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    g_pMeshNote = AEGfxMeshEnd();

    AEGfxMeshStart();
    AEGfxTriAdd(-4.0f, -60.0f, 0xFFFFFFFF, 0, 0,
        4.0f, -60.0f, 0xFFFFFFFF, 1, 0,
        -4.0f, 60.0f, 0xFFFFFFFF, 0, 1);
    AEGfxTriAdd(4.0f, -60.0f, 0xFFFFFFFF, 1, 0,
        4.0f, 60.0f, 0xFFFFFFFF, 1, 1,
        -4.0f, 60.0f, 0xFFFFFFFF, 0, 1);
    g_pMeshLine = AEGfxMeshEnd();

    g_fontId = AEGfxCreateFont("Assets/liberation-mono.ttf", 24);
}

void Rhythm_Initialize() {
    g_isPlaying = false;
    g_songTime = 0.0f;
    g_songFinished = false;
    g_score = { 0, 0, 0, 0, 0, 0 };
    g_activeNotes.clear();
    g_hitFeedbackTimer = 0.0f;
    g_audioStarted = false;
    g_preSongTimer = 0.0f;

    // SET YOUR SONG PARAMETERS HERE
    g_audioOffset = 2.0f;
    g_songDuration = 120.0f;     // CHANGE THIS: Your song length in seconds
    float bpm = 120.0f;          // CHANGE THIS: Your song BPM

    CreateChartForDuration(g_songDuration, bpm);

    g_currentSong = AEAudioLoadMusic("Assets/Baila_Mejor.wav");

    g_isPlaying = true;
}

void Rhythm_Update() {
    if (!g_isPlaying) return;

    float dt = (float)AEFrameRateControllerGetFrameTime();

    // Handle countdown
    if (!g_audioStarted) {
        g_preSongTimer += dt;
        if (g_preSongTimer >= g_audioOffset) {
            g_audioStarted = true;
            if (IsValidAudio(g_currentSong) && IsValidGroup(g_musicGroup)) {
                AEAudioPlay(g_currentSong, g_musicGroup, 1.0f, 1.0f, 0);
                printf("Audio started playing\n");
            }
        }
    }
    else {
        g_songTime += dt;
    }

    if (g_hitFeedbackTimer > 0) {
        g_hitFeedbackTimer -= dt;
    }

    // Spawn notes
    while (g_nextSpawnIndex < g_notes.size()) {
        RhythmNote& note = g_notes[g_nextSpawnIndex];
        if (note.hitTime - 1.5f <= g_songTime) {
            g_activeNotes.push_back(note);
            g_nextSpawnIndex++;
        }
        else {
            break;
        }
    }

    // Update notes
    for (auto it = g_activeNotes.begin(); it != g_activeNotes.end(); ) {
        float timeUntilHit = it->hitTime - g_songTime;
        it->xPosition = JUDGMENT_LINE_X + (timeUntilHit * NOTE_SPEED);

        if (!it->hit && !it->missed && timeUntilHit < -MISS_WINDOW) {
            it->missed = true;
            UpdateScore(HIT_MISS);
        }

        bool shouldRemove = it->hit ||
            (it->missed && it->xPosition < -300.0f) ||
            (it->xPosition < -600.0f);

        if (shouldRemove) {
            it = g_activeNotes.erase(it);
        }
        else {
            ++it;
        }
    }

    // Song ends when time exceeds duration and all notes processed
    if (g_audioStarted && g_songTime >= g_songDuration && g_activeNotes.empty()) {
        if (!g_songFinished) {
            g_songFinished = true;
            printf("Song finished! Final Score: %d, Max Combo: %d\n",
                g_score.totalScore, g_score.maxCombo);
        }
    }
}

void Rhythm_Render() {
    if (!g_isPlaying) return;

    AEMtx33 scale, trans, transform;
    char buffer[64];

    // Draw lane line
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f);
    AEMtx33Scale(&scale, 1600.0f, 2.0f);
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshNote, AE_GFX_MDM_TRIANGLES);

    // Draw judgment line
    float lineR = 1.0f, lineG = 1.0f, lineB = 1.0f;
    if (g_hitFeedbackTimer > 0) {
        if (g_lastHitRating == HIT_PERFECT) { lineR = 1.0f; lineG = 0.8f; lineB = 0.2f; }
        else if (g_lastHitRating == HIT_GOOD) { lineR = 0.2f; lineG = 1.0f; lineB = 0.2f; }
        else if (g_lastHitRating == HIT_MISS) { lineR = 1.0f; lineG = 0.2f; lineB = 0.2f; }
    }
    AEGfxSetColorToMultiply(lineR, lineG, lineB, 1.0f);
    AEMtx33Scale(&scale, 1.0f, 1.0f);
    AEMtx33Trans(&trans, JUDGMENT_LINE_X, 0.0f);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshLine, AE_GFX_MDM_TRIANGLES);

    // Draw hit zone
    AEGfxSetColorToMultiply(0.5f, 0.5f, 0.5f, 0.3f);
    AEMtx33Scale(&scale, NOTE_SIZE * 2.0f, NOTE_SIZE * 2.0f);
    AEMtx33Trans(&trans, JUDGMENT_LINE_X, 0.0f);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshNote, AE_GFX_MDM_TRIANGLES);

    // Draw notes
    for (const auto& note : g_activeNotes) {
        if (note.hit) continue;

        float r = 1.0f, g = 1.0f, b = 1.0f;
        float size = NOTE_SIZE;

        switch (note.type) {
        case NOTE_NORMAL:
            r = 0.2f; g = 0.6f; b = 1.0f;
            break;
        case NOTE_GOLD:
            r = 1.0f; g = 0.8f; b = 0.2f;
            size = NOTE_SIZE * 1.2f;
            break;
        default:
            break;
        }

        if (note.missed) {
            r = 0.5f; g = 0.5f; b = 0.5f;
        }

        AEGfxSetColorToMultiply(r, g, b, note.missed ? 0.3f : 1.0f);
        AEMtx33Scale(&scale, size * 2.0f, size * 2.0f);
        AEMtx33Trans(&trans, note.xPosition, 0.0f);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshNote, AE_GFX_MDM_TRIANGLES);

        if (note.type == NOTE_NORMAL && !note.missed) {
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 0.8f);
            AEMtx33Scale(&scale, size, size);
            AEMtx33Trans(&trans, note.xPosition, 0.0f);
            AEMtx33Concat(&transform, &trans, &scale);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(g_pMeshNote, AE_GFX_MDM_TRIANGLES);
        }
    }

    // UI
    if (g_fontId >= 0) {
        sprintf_s(buffer, "Score: %d", g_score.totalScore);
        AEGfxPrint(g_fontId, buffer, -0.95f, 0.9f, 1.0f, 1, 1, 1, 1);

        if (g_score.combo > 0) {
            sprintf_s(buffer, "%d", g_score.combo);
            float pulse = 1.0f + (g_hitFeedbackTimer * 0.5f);
            AEGfxPrint(g_fontId, buffer, -0.05f, 0.6f, 1.5f * pulse, 1, 0.9f, 0.3f, 1);
            AEGfxPrint(g_fontId, "COMBO", -0.1f, 0.5f, 0.8f, 1, 0.9f, 0.3f, 1);
        }

        if (g_hitFeedbackTimer > 0) {
            const char* text = "";
            float tr = 1, tg = 1, tb = 1;
            switch (g_lastHitRating) {
            case HIT_PERFECT: text = "PERFECT!"; tr = 1; tg = 0.8f; tb = 0.2f; break;
            case HIT_GOOD: text = "GOOD"; tr = 0.3f; tg = 1; tb = 0.3f; break;
            case HIT_MISS: text = "MISS"; tr = 1; tg = 0.3f; tb = 0.3f; break;
            default: break;
            }
            AEGfxPrint(g_fontId, text, -0.15f, 0.3f, 1.2f, tr, tg, tb, 1);
        }

        sprintf_s(buffer, "Perfect: %d  Good: %d  Miss: %d",
            g_score.perfectHits, g_score.goodHits, g_score.misses);
        AEGfxPrint(g_fontId, buffer, -0.95f, -0.9f, 0.7f, 0.8f, 0.8f, 0.8f, 1);

        if (g_songFinished) {
            AEGfxPrint(g_fontId, "SONG COMPLETE!", -0.3f, 0.0f, 2.0f, 0.2f, 1.0f, 0.2f, 1);
            sprintf_s(buffer, "Final Score: %d", g_score.totalScore);
            AEGfxPrint(g_fontId, buffer, -0.25f, -0.15f, 1.2f, 1, 1, 1, 1);
            AEGfxPrint(g_fontId, "Press E to return", -0.25f, -0.3f, 0.8f, 0.8f, 0.8f, 0.8f, 1);
        }

        if (!g_audioStarted && !g_songFinished) {
            int countdown = (int)(g_audioOffset - g_preSongTimer) + 1;
            sprintf_s(buffer, "Starting in: %d", countdown);
            AEGfxPrint(g_fontId, buffer, -0.2f, 0.0f, 2.0f, 1, 1, 0, 1);
        }
    }
}

void Rhythm_Free() {
    g_activeNotes.clear();
    if (IsValidGroup(g_musicGroup)) {
        AEAudioStopGroup(g_musicGroup);
    }
}

void Rhythm_Unload() {
    if (g_pMeshNote) AEGfxMeshFree(g_pMeshNote);
    if (g_pMeshLine) AEGfxMeshFree(g_pMeshLine);
    if (g_fontId >= 0) AEGfxDestroyFont(g_fontId);

    if (IsValidAudio(g_currentSong)) {
        AEAudioUnloadAudio(g_currentSong);
        ResetAudio(g_currentSong);
    }
    if (IsValidGroup(g_musicGroup)) {
        AEAudioUnloadAudioGroup(g_musicGroup);
        ResetAudioGroup(g_musicGroup);
    }
}

// ================= GAMEPLAY =================

void Rhythm_Start() {
    Rhythm_Initialize();
}

void Rhythm_Stop() {
    g_isPlaying = false;
    if (IsValidGroup(g_musicGroup)) {
        AEAudioStopGroup(g_musicGroup);
    }
}

bool Rhythm_IsPlaying() {
    return g_isPlaying;
}

void Rhythm_Hit() {
    if (!g_isPlaying || !g_audioStarted || g_songFinished) return;

    RhythmNote* closest = nullptr;
    float closestDiff = 999.0f;

    for (auto& note : g_activeNotes) {
        if (note.hit || note.missed) continue;

        float diff = fabsf(note.hitTime - g_songTime);

        if (diff < GOOD_WINDOW && diff < closestDiff) {
            closestDiff = diff;
            closest = &note;
        }
    }

    if (closest) {
        HitRating rating;
        if (closestDiff <= PERFECT_WINDOW) rating = HIT_PERFECT;
        else rating = HIT_GOOD;

        closest->hit = true;
        closest->rating = rating;
        UpdateScore(rating);
    }
}

bool Rhythm_IsSongFinished() {
    return g_songFinished;
}

float Rhythm_GetSongDuration() {
    return g_songDuration;
}

float Rhythm_GetCurrentTime() {
    return g_songTime;
}

const RhythmScore& Rhythm_GetScore() {
    return g_score;
}