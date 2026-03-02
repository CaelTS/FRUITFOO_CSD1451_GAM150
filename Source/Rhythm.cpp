#include "Rhythm.h"
#include "AEEngine.h"
#include "AEAudio.h"
#include <algorithm>
#include <stdio.h>
#include <cstring>
#include <cstdlib>  // For rand()
#include <ctime>    // For time()

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

// Vertical offset to shift game upward (positive = up)
static const float VERTICAL_OFFSET = 200.0f;

// ================= TEXT POSITION CONSTANTS =================
// Each text has its own X and Y position (normalized coordinates: -1.0 to 1.0)

// Score display (top left)
static const float SCORE_TEXT_X = -0.95f;
static const float SCORE_TEXT_Y = 0.85f;

// Combo number (center area)
static const float COMBO_NUM_X = -0.20f;
static const float COMBO_NUM_Y = 0.75f;

// Combo label
static const float COMBO_LABEL_X = -0.1f;
static const float COMBO_LABEL_Y = 0.75f;

// Hit feedback (PERFECT/GOOD/MISS)
static const float FEEDBACK_TEXT_X = -0.15f;
static const float FEEDBACK_TEXT_Y = 0.05f;

// Stats (Perfect/Good/Miss count) - bottom left
static const float STATS_TEXT_X = -0.95f;
static const float STATS_TEXT_Y = -0.85f;

// Song complete title
static const float COMPLETE_TEXT_X = -0.3f;
static const float COMPLETE_TEXT_Y = 0.20f;

// Final score
static const float FINAL_SCORE_X = -0.25f;
static const float FINAL_SCORE_Y = 0.05f;

// Return prompt
static const float RETURN_TEXT_X = -0.25f;
static const float RETURN_TEXT_Y = -0.05f;


// Countdown
static const float COUNTDOWN_TEXT_X = -0.3f;
static const float COUNTDOWN_TEXT_Y = 0.20f;

// Random spawn parameters
static const float MIN_SPAWN_INTERVAL = 0.2f;   // Minimum time between notes
static const float MAX_SPAWN_INTERVAL = 0.8f;   // Maximum time between notes
static const float DOUBLE_NOTE_CHANCE = 0.15f;  // 15% chance for double notes
static const float PREMIUM_NOTE_CHANCE = 0.25f;    // 25% chance for premium notes

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

// Random seed initialization flag
static bool g_randomInitialized = false;

// Texture handles for notes
static AEGfxTexture* g_pTexNormalNote = nullptr;
static AEGfxTexture* g_pTexPremiumNote = nullptr;
static AEGfxTexture* g_pTexBackground = nullptr;
static AEGfxTexture* g_pTexWateringCan = nullptr;

// Watering can state
static float g_wateringCanRotation = 0.0f;  // Current rotation
static float g_wateringCanAnimTimer = 0.0f;  // Animation timer
static const float WATERING_CAN_ANIM_DURATION = 0.3f;  // Total animation time (up and back)
static bool g_wateringCanIsAnimating = false;  // Whether animation is active

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
        // Trigger watering can animation
        g_wateringCanIsAnimating = true;
        g_wateringCanAnimTimer = 0.0f;
        break;
    case HIT_GOOD:
        g_score.goodHits++;
        g_score.combo++;
        g_score.totalScore += SCORE_GOOD + (g_score.combo * 5);
        // Trigger watering can animation
        g_wateringCanIsAnimating = true;
        g_wateringCanAnimTimer = 0.0f;
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

// Helper function to get random float between min and max
static float RandomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

// Helper function to check if we should spawn a double note
static bool ShouldSpawnDouble() {
    return RandomFloat(0.0f, 1.0f) < DOUBLE_NOTE_CHANCE;
}

// Helper function to get random note type
static NoteType GetRandomNoteType() {
    float rng = RandomFloat(0.0f, 1.0f);
    if (rng < PREMIUM_NOTE_CHANCE) {
        return NOTE_PREMIUM;
    }
    return NOTE_NORMAL;
}

// Create randomized chart for the song duration
static void CreateRandomChart(float totalDuration) {
    g_notes.clear();
    g_nextSpawnIndex = 0;

    float currentTime = g_audioOffset;

    // Continue spawning notes until we reach near the end of the song
    while (currentTime < totalDuration - 2.0f) {
        // Random interval until next note
        float interval = RandomFloat(MIN_SPAWN_INTERVAL, MAX_SPAWN_INTERVAL);

        // Create the main note
        RhythmNote note = {};
        note.type = GetRandomNoteType();
        note.hitTime = currentTime;
        note.xPosition = SPAWN_X;
        note.hit = false;
        note.missed = false;
        note.rating = HIT_NONE;
        g_notes.push_back(note);

        // Chance to spawn a double note (quick second note)
        if (ShouldSpawnDouble()) {
            RhythmNote doubleNote = {};
            doubleNote.type = NOTE_NORMAL; // Double notes are always normal
            doubleNote.hitTime = currentTime + 0.15f; // 150ms after first note
            doubleNote.xPosition = SPAWN_X;
            doubleNote.hit = false;
            doubleNote.missed = false;
            doubleNote.rating = HIT_NONE;
            g_notes.push_back(doubleNote);

            // Add extra time after double note to prevent overcrowding
            interval += 0.3f;
        }

        currentTime += interval;
    }

    printf("Generated %d random notes for %.1f second song\n",
        (int)g_notes.size(), totalDuration);
}

// ================= LIFECYCLE =================

void Rhythm_Load() {
    // Initialize random seed once
    if (!g_randomInitialized) {
        srand(static_cast<unsigned int>(time(NULL)));
        g_randomInitialized = true;
    }

    ResetAudio(g_currentSong);
    ResetAudioGroup(g_musicGroup);

    g_musicGroup = AEAudioCreateGroup();
    if (!IsValidGroup(g_musicGroup)) {
        printf("WARNING: Failed to create audio group!\n");
    }

    // Create a simple square mesh for sprites (UV mapped)
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,  // Bottom-left
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,   // Bottom-right
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);  // Top-left
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,   // Bottom-right
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,    // Top-right
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);  // Top-left
    g_pMeshNote = AEGfxMeshEnd();

    // Judgment line mesh (unchanged)
    AEGfxMeshStart();
    AEGfxTriAdd(-4.0f, -60.0f, 0xFFFFFFFF, 0, 0,
        4.0f, -60.0f, 0xFFFFFFFF, 1, 0,
        -4.0f, 60.0f, 0xFFFFFFFF, 0, 1);
    AEGfxTriAdd(4.0f, -60.0f, 0xFFFFFFFF, 1, 0,
        4.0f, 60.0f, 0xFFFFFFFF, 1, 1,
        -4.0f, 60.0f, 0xFFFFFFFF, 0, 1);
    g_pMeshLine = AEGfxMeshEnd();

    // NOTE SPRITES
    g_pTexNormalNote = AEGfxTextureLoad("Assets/normal_note.png");
    g_pTexPremiumNote = AEGfxTextureLoad("Assets/premium_note.png");
    g_pTexBackground = AEGfxTextureLoad("Assets/background.png");
    g_pTexWateringCan = AEGfxTextureLoad("Assets/watering_can.jpg");

    if (!g_pTexNormalNote) {
        printf("ERROR: Failed to load normal note texture!\n");
    }
    if (!g_pTexPremiumNote) {
        printf("ERROR: Failed to load premium note texture!\n");
    }
    if (!g_pTexBackground) {
        printf("WARNING: Failed to load background texture!\n");
    }
    if (!g_pTexWateringCan) {
        printf("ERROR: Failed to load watering can texture!\n");
    }

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

    // Reset watering can
    g_wateringCanRotation = 0.0f;
    g_wateringCanAnimTimer = 0.0f;
    g_wateringCanIsAnimating = false;

    // SET SONG PARAMETERS HERE
    g_audioOffset = 2.0f;
    g_songDuration = 75.0f;     // Song length in seconds

    // Create random chart instead of pattern-based
    CreateRandomChart(g_songDuration);

    g_currentSong = AEAudioLoadMusic("Assets/kk.wav");

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

    // Update watering can rotation animation
    if (g_wateringCanIsAnimating) {
        g_wateringCanAnimTimer += dt;

        float halfDuration = WATERING_CAN_ANIM_DURATION / 2.0f;
        float progress;

        if (g_wateringCanAnimTimer <= halfDuration) {
            // First half: rotate anticlockwise to 45 degrees
            progress = g_wateringCanAnimTimer / halfDuration;
            // Ease out
            progress = progress * (2.0f - progress);
            g_wateringCanRotation = 45.0f * progress;
        }
        else if (g_wateringCanAnimTimer <= WATERING_CAN_ANIM_DURATION) {
            // Second half: rotate back to 0 degrees
            progress = (g_wateringCanAnimTimer - halfDuration) / halfDuration;
            // Ease in
            progress = progress * progress;
            g_wateringCanRotation = 45.0f * (1.0f - progress);
        }
        else {
            // Animation complete
            g_wateringCanRotation = 0.0f;
            g_wateringCanIsAnimating = false;
            g_wateringCanAnimTimer = 0.0f;
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

    // ================= DRAW BACKGROUND FIRST =================
    if (g_pTexBackground) {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);  // Full color, no tint
        AEGfxSetBlendMode(AE_GFX_BM_NONE);  // No blending for opaque background
        AEGfxTextureSet(g_pTexBackground, 0, 0);

        // Scale to fill the screen (adjust 1600x900 to your resolution)
        AEMtx33Scale(&scale, 1600.0f, 900.0f);
        AEMtx33Trans(&trans, 0.0f, 0.0f);  // Center of screen
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshNote, AE_GFX_MDM_TRIANGLES);
    }

    // ================= DRAW WATERING CAN =================
    if (g_pTexWateringCan) {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxTextureSet(g_pTexWateringCan, 0, 0);

        // Position above the judgment line
        float canSize = 40.0f;  // Size of watering can
        float canX = JUDGMENT_LINE_X;  // Same X as judgment line
        float canY = VERTICAL_OFFSET + 100.0f;  // Above the judgment line

        // Build transform: scale -> rotate -> translate
        // Use temp variables to avoid conflict with outer scope
        AEMtx33 canScale, canRot, canTrans, canTemp, canTransform;
        AEMtx33Scale(&canScale, canSize * 2.0f, canSize * 2.0f);
        AEMtx33RotDeg(&canRot, g_wateringCanRotation);  // Apply rotation
        AEMtx33Trans(&canTrans, canX, canY);

        // Combine: trans * rot * scale
        AEMtx33Concat(&canTemp, &canRot, &canScale);
        AEMtx33Concat(&canTransform, &canTrans, &canTemp);

        AEGfxSetTransform(canTransform.m);
        AEGfxMeshDraw(g_pMeshNote, AE_GFX_MDM_TRIANGLES);
    }

    // Draw lane line
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f);
    AEMtx33Scale(&scale, 1600.0f, 2.0f);
    AEMtx33Trans(&trans, 0.0f, VERTICAL_OFFSET);
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
    AEMtx33Trans(&trans, JUDGMENT_LINE_X, VERTICAL_OFFSET);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshLine, AE_GFX_MDM_TRIANGLES);

    // Draw hit zone
    AEGfxSetColorToMultiply(0.5f, 0.5f, 0.5f, 0.3f);
    AEMtx33Scale(&scale, NOTE_SIZE * 2.0f, NOTE_SIZE * 2.0f);
    AEMtx33Trans(&trans, JUDGMENT_LINE_X, VERTICAL_OFFSET);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(g_pMeshNote, AE_GFX_MDM_TRIANGLES);

    // Draw notes (shifted up)
    for (const auto& note : g_activeNotes) {
        if (note.hit) continue;

        AEGfxTexture* pTex = nullptr;
        float size = NOTE_SIZE;
        float alpha = 1.0f;
        float tintR = 1.0f, tintG = 1.0f, tintB = 1.0f;

        // Select texture based on note type
        switch (note.type) {
        case NOTE_NORMAL:
            pTex = g_pTexNormalNote;
            break;
        case NOTE_PREMIUM:
            pTex = g_pTexPremiumNote;
            size = NOTE_SIZE * 1.2f;
            break;
        default:
            break;
        }

        // Handle missed notes (grayed out)
        if (note.missed) {
            alpha = 0.3f;
            tintR = tintG = tintB = 0.5f;
        }

        // Draw the sprite
        if (pTex) {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(tintR, tintG, tintB, alpha);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(alpha);
            AEGfxTextureSet(pTex, 0, 0);
        }
        else {
            // Fallback to colored square if texture failed to load
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetColorToMultiply(tintR, tintG, tintB, alpha);
        }

        AEMtx33Scale(&scale, size * 2.0f, size * 2.0f);
        AEMtx33Trans(&trans, note.xPosition, VERTICAL_OFFSET);
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshNote, AE_GFX_MDM_TRIANGLES);
    }

    // UI
    if (g_fontId >= 0) {
        // Score
        sprintf_s(buffer, "Score: %d", g_score.totalScore);
        AEGfxPrint(g_fontId, buffer, SCORE_TEXT_X, SCORE_TEXT_Y, 1.0f, 1, 1, 1, 1);

        // Combo
        if (g_score.combo > 0) {
            sprintf_s(buffer, "%d", g_score.combo);
            float pulse = 1.0f + (g_hitFeedbackTimer * 0.5f);
            AEGfxPrint(g_fontId, buffer, COMBO_NUM_X, COMBO_NUM_Y, 1.5f * pulse, 1, 0.9f, 0.3f, 1);
            AEGfxPrint(g_fontId, "COMBO", COMBO_LABEL_X, COMBO_LABEL_Y, 1.5f, 1, 0.9f, 0.3f, 1);
        }

        // Hit feedback
        if (g_hitFeedbackTimer > 0) {
            const char* text = "";
            float tr = 1, tg = 1, tb = 1;
            switch (g_lastHitRating) {
            case HIT_PERFECT: text = "PERFECT!"; tr = 1; tg = 0.8f; tb = 0.2f; break;
            case HIT_GOOD: text = "GOOD"; tr = 0.3f; tg = 1; tb = 0.3f; break;
            case HIT_MISS: text = "MISS"; tr = 1; tg = 0.3f; tb = 0.3f; break;
            default: break;
            }
            AEGfxPrint(g_fontId, text, FEEDBACK_TEXT_X, FEEDBACK_TEXT_Y, 1.2f, tr, tg, tb, 1);
        }

        // Stats
        sprintf_s(buffer, "Perfect: %d  Good: %d  Miss: %d",
            g_score.perfectHits, g_score.goodHits, g_score.misses);
        AEGfxPrint(g_fontId, buffer, STATS_TEXT_X, STATS_TEXT_Y, 0.7f, 0.8f, 0.8f, 0.8f, 1);

        // Song finished
        if (g_songFinished) {
            AEGfxPrint(g_fontId, "SONG COMPLETE!", COMPLETE_TEXT_X, COMPLETE_TEXT_Y, 2.0f, 0.2f, 1.0f, 0.2f, 1);
            sprintf_s(buffer, "Final Score: %d", g_score.totalScore);
            AEGfxPrint(g_fontId, buffer, FINAL_SCORE_X, FINAL_SCORE_Y, 1.2f, 1, 1, 1, 1);
            AEGfxPrint(g_fontId, "Press E to return", RETURN_TEXT_X, RETURN_TEXT_Y, 0.8f, 0.8f, 0.8f, 0.8f, 1);
        }

        // Countdown
        if (!g_audioStarted && !g_songFinished) {
            int countdown = (int)(g_audioOffset - g_preSongTimer) + 1;
            sprintf_s(buffer, "Starting in: %d", countdown);
            AEGfxPrint(g_fontId, buffer, COUNTDOWN_TEXT_X, COUNTDOWN_TEXT_Y, 2.0f, 1, 1, 0, 1);
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

    // Free textures
    if (g_pTexNormalNote) AEGfxTextureUnload(g_pTexNormalNote);
    if (g_pTexPremiumNote) AEGfxTextureUnload(g_pTexPremiumNote);
    if (g_pTexBackground) AEGfxTextureUnload(g_pTexBackground);
    if (g_pTexWateringCan) AEGfxTextureUnload(g_pTexWateringCan);
    g_pTexNormalNote = nullptr;
    g_pTexPremiumNote = nullptr;
    g_pTexBackground = nullptr;
    g_pTexWateringCan = nullptr;

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