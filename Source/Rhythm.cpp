#include "Rhythm.h"
#include "AEEngine.h"
#include "AEAudio.h"
#include <algorithm>
#include <stdio.h>
#include <cstring>
#include <cstdlib>  // For rand()
#include <ctime>    // For time()
#include "Farm.h"   // For Farm_GetRhythmSeedType() in Rhythm_Initialize

// ================= CONSTANTS =================

static const float PERFECT_WINDOW = 0.050f;  // 50ms
static const float GOOD_WINDOW = 0.100f;  // 100ms
static const float MISS_WINDOW = 0.150f;  // 150ms

static const float JUDGMENT_LINE_X = -200.0f;
static const float SPAWN_X = 900.0f;
static const float NOTE_SIZE = 40.0f;

static const int SCORE_PERFECT = 300;
static const int SCORE_GOOD = 100;

// Vertical offset to shift game upward (positive = up)
static const float VERTICAL_OFFSET = 200.0f;

// ================= DIFFICULTY CONFIG =================
// Each row corresponds to DIFFICULTY_EASY / MEDIUM / HARD.
// Adjust these values freely to tune the feel of each tier.

static const RhythmDifficultyConfig DIFFICULTY_CONFIGS[3] = {
    // EASY
    // noteSpeed  minSpawn  maxSpawn  doubleChance  premiumChance  audioFile                        backgroundFile
    {  400.0f,    0.8f,     1.4f,     0.05f,        0.15f,         "Assets/TETR-NCS1.wav",          "Assets/bg_diff.png" },

    // MEDIUM
    {  500.0f,    0.4f,     1.0f,     0.15f,        0.25f,         "Assets/SAVJP-NCS2.wav",         "Assets/bg_diff.png" },

    // HARD
    {  600.0f,    0.2f,     0.8f,     0.30f,        0.40f,         "Assets/WDYS-NCS3.wav",         "Assets/bg_diff.png" },
};

static RhythmDifficulty        g_difficulty = DIFFICULTY_MEDIUM;
static RhythmDifficultyConfig  g_difficultyConfig = DIFFICULTY_CONFIGS[DIFFICULTY_MEDIUM];

// ================= PHASE =================
enum RhythmPhase {
    PHASE_DIFFICULTY_SELECT,
    PHASE_PLAYING,
    PHASE_FINISHED
};
static RhythmPhase g_phase = PHASE_DIFFICULTY_SELECT;

static int g_diffSelectHovered = -1;

// ================= TEXT POSITION CONSTANTS =================

static const float SCORE_TEXT_X = -0.95f;
static const float SCORE_TEXT_Y = 0.85f;

static const float COMBO_NUM_X = -0.10f;
static const float COMBO_NUM_Y = 0.75f;

static const float COMBO_LABEL_X = 0.0f;
static const float COMBO_LABEL_Y = 0.75f;

static const float FEEDBACK_TEXT_X = -0.15f;
static const float FEEDBACK_TEXT_Y = 0.05f;

static const float STATS_TEXT_X = -0.95f;
static const float STATS_TEXT_Y = -0.85f;

static const float COMPLETE_TEXT_X = -0.3f;
static const float COMPLETE_TEXT_Y = 0.20f;

static const float FINAL_SCORE_X = -0.25f;
static const float FINAL_SCORE_Y = 0.05f;

static const float RETURN_TEXT_X = -0.25f;
static const float RETURN_TEXT_Y = -0.05f;

static const float COUNTDOWN_TEXT_X = -0.3f;
static const float COUNTDOWN_TEXT_Y = 0.20f;

static const float DIFF_TITLE_X = -0.30f;
static const float DIFF_TITLE_Y = 0.55f;
static const float DIFF_EASY_X = -0.45f;
static const float DIFF_EASY_Y = 0.20f;
static const float DIFF_MED_X = -0.45f;
static const float DIFF_MED_Y = 0.00f;
static const float DIFF_HARD_X = -0.45f;
static const float DIFF_HARD_Y = -0.20f;
static const float DIFF_HINT_X = -0.32f;
static const float DIFF_HINT_Y = -0.55f;

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

static bool g_randomInitialized = false;

// Texture handles for notes
// g_pTexNormalNote / g_pTexPremiumNote point into the arrays below
static AEGfxTexture* g_pTexNormalNote = nullptr;
static AEGfxTexture* g_pTexPremiumNote = nullptr;
static AEGfxTexture* g_pTexWateringCan = nullptr;

// Per-fruit note textures  [0]=Apple  [1]=Pear  [2]=Banana
// Normal note  = the grown fruit image
// Premium note = the seed packet image
static AEGfxTexture* g_pTexFruitNotes[3] = { nullptr, nullptr, nullptr };
static AEGfxTexture* g_pTexSeedNotes[3] = { nullptr, nullptr, nullptr };

// ---------------------------------------------------------------
// Per-difficulty background textures
// Index 0 = Easy, 1 = Medium, 2 = Hard
// ---------------------------------------------------------------
static AEGfxTexture* g_pTexGameplayBg[3] = { nullptr, nullptr, nullptr };
static const char* DIFFICULTY_SELECT_BG_PATH = "Assets/bg_diff.png";
static AEGfxTexture* g_pTexDiffSelectBg = nullptr;

// Watering can state
static float g_wateringCanRotation = 0.0f;
static float g_wateringCanAnimTimer = 0.0f;
static const float WATERING_CAN_ANIM_DURATION = 0.3f;
static bool g_wateringCanIsAnimating = false;

static bool gMusicEnabled = true;

// ================= HELPERS =================

static void StartGameplay();  // Forward declaration

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
        g_wateringCanIsAnimating = true;
        g_wateringCanAnimTimer = 0.0f;
        break;
    case HIT_GOOD:
        g_score.goodHits++;
        g_score.combo++;
        g_score.totalScore += SCORE_GOOD + (g_score.combo * 5);
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

static float RandomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

static void CreateRandomChart(float totalDuration) {
    g_notes.clear();
    g_nextSpawnIndex = 0;

    const RhythmDifficultyConfig& cfg = g_difficultyConfig;

    float currentTime = g_audioOffset;

    while (currentTime < totalDuration - 2.0f) {
        float interval = RandomFloat(cfg.minSpawnInterval, cfg.maxSpawnInterval);

        RhythmNote note = {};
        note.type = (RandomFloat(0.0f, 1.0f) < cfg.premiumNoteChance) ? NOTE_PREMIUM : NOTE_NORMAL;
        note.hitTime = currentTime;
        note.xPosition = SPAWN_X;
        note.hit = false;
        note.missed = false;
        note.rating = HIT_NONE;
        g_notes.push_back(note);

        if (RandomFloat(0.0f, 1.0f) < cfg.doubleNoteChance) {
            RhythmNote doubleNote = {};
            doubleNote.type = NOTE_NORMAL;
            doubleNote.hitTime = currentTime + 0.15f;
            doubleNote.xPosition = SPAWN_X;
            doubleNote.hit = false;
            doubleNote.missed = false;
            doubleNote.rating = HIT_NONE;
            g_notes.push_back(doubleNote);
            interval += 0.3f;
        }

        currentTime += interval;
    }

    printf("Generated %d random notes for %.1f second song (difficulty: %d)\n",
        (int)g_notes.size(), totalDuration, (int)g_difficulty);
}

// ================= LIFECYCLE =================

void Rhythm_Load() {
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

    // Square mesh (UV mapped)
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    g_pMeshNote = AEGfxMeshEnd();

    // Judgment line mesh
    AEGfxMeshStart();
    AEGfxTriAdd(-4.0f, -60.0f, 0xFFFFFFFF, 0, 0,
        4.0f, -60.0f, 0xFFFFFFFF, 1, 0,
        -4.0f, 60.0f, 0xFFFFFFFF, 0, 1);
    AEGfxTriAdd(4.0f, -60.0f, 0xFFFFFFFF, 1, 0,
        4.0f, 60.0f, 0xFFFFFFFF, 1, 1,
        -4.0f, 60.0f, 0xFFFFFFFF, 0, 1);
    g_pMeshLine = AEGfxMeshEnd();

    // ---------------------------------------------------------------
    // NOTE SPRITES — per-fruit/seed textures
    // Normal note  = the grown fruit  (fruit_apple.png, plotpear.png, plotbanana.png)
    // Premium note = the seed packet  (appleseed.png,   pearseed.png,  bananaseed.png)
    // !! MODIFY paths here if your filenames differ !!
    g_pTexFruitNotes[0] = AEGfxTextureLoad("Assets/fruit_apple.png");
    g_pTexFruitNotes[1] = AEGfxTextureLoad("Assets/plotpear.png");
    g_pTexFruitNotes[2] = AEGfxTextureLoad("Assets/plotbanana.png");

    g_pTexSeedNotes[0] = AEGfxTextureLoad("Assets/appleseed.png");
    g_pTexSeedNotes[1] = AEGfxTextureLoad("Assets/pearseed.png");
    g_pTexSeedNotes[2] = AEGfxTextureLoad("Assets/bananaseed.png");

    // Default to apple until Rhythm_Initialize() applies the correct seed type
    g_pTexNormalNote = g_pTexFruitNotes[0];
    g_pTexPremiumNote = g_pTexSeedNotes[0];

    g_pTexWateringCan = AEGfxTextureLoad("Assets/watering_can.png");
    // !! END MODIFY !!
    // ---------------------------------------------------------------

    for (int i = 0; i < 3; i++) {
        if (!g_pTexFruitNotes[i])
            printf("ERROR: Failed to load fruit note texture [%d]!\n", i);
        if (!g_pTexSeedNotes[i])
            printf("ERROR: Failed to load seed note texture [%d]!\n", i);
    }
    if (!g_pTexWateringCan) printf("ERROR: Failed to load watering can texture!\n");

    // ---------------------------------------------------------------
    // Load per-difficulty gameplay backgrounds from the config table
    // (no changes needed here — edit the paths in DIFFICULTY_CONFIGS above)
    // ---------------------------------------------------------------
    for (int i = 0; i < 3; i++) {
        g_pTexGameplayBg[i] = AEGfxTextureLoad(DIFFICULTY_CONFIGS[i].backgroundFile);
        if (!g_pTexGameplayBg[i]) {
            printf("WARNING: Failed to load gameplay background for difficulty %d: %s\n",
                i, DIFFICULTY_CONFIGS[i].backgroundFile);
        }
    }

    // Load difficulty-select screen background
    // (no changes needed here — edit DIFFICULTY_SELECT_BG_PATH above)
    g_pTexDiffSelectBg = AEGfxTextureLoad(DIFFICULTY_SELECT_BG_PATH);
    if (!g_pTexDiffSelectBg) {
        printf("WARNING: Failed to load difficulty select background: %s\n",
            DIFFICULTY_SELECT_BG_PATH);
    }

    g_fontId = AEGfxCreateFont("Assets/liberation-mono.ttf", 24);
}

void Rhythm_Initialize() {
    g_phase = PHASE_DIFFICULTY_SELECT;
    g_diffSelectHovered = -1;

    g_isPlaying = true;
    g_songTime = 0.0f;
    g_songFinished = false;
    g_score = { 0, 0, 0, 0, 0, 0 };
    g_activeNotes.clear();
    g_notes.clear();
    g_nextSpawnIndex = 0;
    g_hitFeedbackTimer = 0.0f;
    g_audioStarted = false;
    g_preSongTimer = 0.0f;

    g_wateringCanRotation = 0.0f;
    g_wateringCanAnimTimer = 0.0f;
    g_wateringCanIsAnimating = false;

    // FIX: Apply the correct fruit/seed textures here, AFTER Rhythm_Load() has
    // finished loading all texture arrays. Calling Rhythm_SetSeedType() before
    // the state transition meant Rhythm_Load() would overwrite it with apple.
    int seedType = Farm_GetRhythmSeedType();
    if (seedType < 0 || seedType > 2) seedType = 0;
    g_pTexNormalNote = g_pTexFruitNotes[seedType];
    g_pTexPremiumNote = g_pTexSeedNotes[seedType];
    printf("Rhythm_Initialize: applied seedType=%d — normal=%s, premium=%s\n",
        seedType,
        g_pTexNormalNote ? "OK" : "NULL",
        g_pTexPremiumNote ? "OK" : "NULL");
}

// Called internally once the player has chosen a difficulty.
static void StartGameplay() {
    g_phase = PHASE_PLAYING;

    g_audioOffset = 2.0f;
    g_songDuration = 65.0f;

    CreateRandomChart(g_songDuration);

    // Unload any previously loaded song before loading the new one
    if (IsValidAudio(g_currentSong)) {
        AEAudioUnloadAudio(g_currentSong);
        ResetAudio(g_currentSong);
    }

    // Load the audio file specified for the selected difficulty
    // (no changes needed here — edit the paths in DIFFICULTY_CONFIGS above)
    g_currentSong = AEAudioLoadMusic(g_difficultyConfig.audioFile);
    if (!IsValidAudio(g_currentSong)) {
        printf("ERROR: Failed to load audio for difficulty %d: %s\n",
            (int)g_difficulty, g_difficultyConfig.audioFile);
    }
}

void Rhythm_Update() {
    if (!g_isPlaying) return;

    float dt = (float)AEFrameRateControllerGetFrameTime();

    // ---- DIFFICULTY SELECT PHASE ----
    if (g_phase == PHASE_DIFFICULTY_SELECT) {
        if (AEInputCheckTriggered(AEVK_1)) {
            Rhythm_SetDifficulty(DIFFICULTY_EASY);
            StartGameplay();
        }
        else if (AEInputCheckTriggered(AEVK_2)) {
            Rhythm_SetDifficulty(DIFFICULTY_MEDIUM);
            StartGameplay();
        }
        else if (AEInputCheckTriggered(AEVK_3)) {
            Rhythm_SetDifficulty(DIFFICULTY_HARD);
            StartGameplay();
        }
        return;
    }

    // ---- GAMEPLAY PHASE ----

    if (AEInputCheckTriggered(AEVK_W)) {
        Rhythm_Hit();
    }

    if (!g_audioStarted) {
        g_preSongTimer += dt;
        if (g_preSongTimer >= g_audioOffset) {
            g_audioStarted = true;
            if (gMusicEnabled && IsValidAudio(g_currentSong) && IsValidGroup(g_musicGroup)) {
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
    while (g_nextSpawnIndex < (int)g_notes.size()) {
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
        it->xPosition = JUDGMENT_LINE_X + (timeUntilHit * g_difficultyConfig.noteSpeed);

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

    // Update watering can animation
    if (g_wateringCanIsAnimating) {
        g_wateringCanAnimTimer += dt;

        float halfDuration = WATERING_CAN_ANIM_DURATION / 2.0f;
        float progress;

        if (g_wateringCanAnimTimer <= halfDuration) {
            progress = g_wateringCanAnimTimer / halfDuration;
            progress = progress * (2.0f - progress);
            g_wateringCanRotation = 75.0f * progress;          // FIX: was 45.0f
        }
        else if (g_wateringCanAnimTimer <= WATERING_CAN_ANIM_DURATION) {
            progress = (g_wateringCanAnimTimer - halfDuration) / halfDuration;
            progress = progress * progress;
            g_wateringCanRotation = 75.0f * (1.0f - progress); // FIX: was 45.0f
        }
        else {
            g_wateringCanRotation = 0.0f;
            g_wateringCanIsAnimating = false;
            g_wateringCanAnimTimer = 0.0f;
        }
    }

    if (g_audioStarted && g_songTime >= g_songDuration && g_activeNotes.empty()) {
        if (!g_songFinished) {
            g_songFinished = true;
            g_phase = PHASE_FINISHED;
            printf("Song finished! Final Score: %d, Max Combo: %d\n",
                g_score.totalScore, g_score.maxCombo);
        }
    }
}

// ---------------------------------------------------------------
// Helper: draw a full-screen background texture with brightness control.
// brightness: 0.0 = black, 1.0 = original colours. Values like 0.5 halve
// the brightness without changing the image tint.
// ---------------------------------------------------------------
static void DrawFullscreenTexture(AEGfxTexture* pTex, AEGfxVertexList* pMesh, float brightness = 1.0f)
{
    if (!pTex) return;
    AEMtx33 scale, trans, transform;
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(brightness, brightness, brightness, 1.0f);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxTextureSet(pTex, 0, 0);
    AEMtx33Scale(&scale, 1600.0f, 900.0f);
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&transform, &trans, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
}

void Rhythm_Render() {
    if (!g_isPlaying) return;

    AEMtx33 scale, trans, transform;
    char buffer[64];

    // ================= DIFFICULTY SELECT SCREEN =================
    if (g_phase == PHASE_DIFFICULTY_SELECT) {

        // Draw difficulty select background at reduced brightness so text is readable
        // !! MODIFY: adjust the 0.5f value (0.0 = black, 1.0 = full brightness) !!
        DrawFullscreenTexture(g_pTexDiffSelectBg, g_pMeshNote, 0.9f);

        // ---------------------------------------------------------------
        // Semi-transparent dark panel behind the difficulty text block.
        // Draws a rounded-ish dark rectangle centred on screen.
        // Tune panel width/height and alpha to taste.
        // ---------------------------------------------------------------
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.55f);
        AEGfxSetColorToMultiply(0.05f, 0.08f, 0.12f, 0.55f); // dark blue-black tint
        AEMtx33Scale(&scale, 900.0f, 550.0f);   // panel width x height in pixels
        AEMtx33Trans(&trans, 0.0f, 0.0f);        // centred on screen
        AEMtx33Concat(&transform, &trans, &scale);
        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_pMeshNote, AE_GFX_MDM_TRIANGLES);
        // ---------------------------------------------------------------

        if (g_fontId >= 0) {
            AEGfxPrint(g_fontId, "SELECT DIFFICULTY",
                DIFF_TITLE_X, DIFF_TITLE_Y, 1.5f, 1.0f, 1.0f, 0.2f, 1.0f);

            AEGfxPrint(g_fontId, "[1]  EASY   - Slow notes, relaxed spacing",
                DIFF_EASY_X, DIFF_EASY_Y, 1.0f, 0.3f, 1.0f, 0.3f, 1.0f);

            AEGfxPrint(g_fontId, "[2]  MEDIUM - Moderate speed and density",
                DIFF_MED_X, DIFF_MED_Y, 1.0f, 1.0f, 0.85f, 0.2f, 1.0f);

            AEGfxPrint(g_fontId, "[3]  HARD   - Fast notes, tight spacing",
                DIFF_HARD_X, DIFF_HARD_Y, 1.0f, 1.0f, 0.3f, 0.3f, 1.0f);

            AEGfxPrint(g_fontId, "Press 1 / 2 / 3 to choose and start",
                DIFF_HINT_X, DIFF_HINT_Y, 0.8f, 0.7f, 0.7f, 0.7f, 1.0f);
        }
        return;
    }

    // ================= DRAW GAMEPLAY BACKGROUND =================
    // Uses the background matching the selected difficulty, also dimmed
    // !! MODIFY: adjust the 0.5f value below to change gameplay bg brightness !!
    DrawFullscreenTexture(g_pTexGameplayBg[(int)g_difficulty], g_pMeshNote, 0.7f);

    // ================= DRAW WATERING CAN =================
    if (g_pTexWateringCan)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxTextureSet(g_pTexWateringCan, 0, 0);

        float canSize = 80.0f;
        float canX = JUDGMENT_LINE_X;
        float canY = VERTICAL_OFFSET + 120.0f;

        AEMtx33 canScale, canRot, canTrans, canTemp, canTransform;
        AEMtx33Scale(&canScale, canSize, canSize);
        AEMtx33RotDeg(&canRot, g_wateringCanRotation);
        AEMtx33Trans(&canTrans, canX, canY);
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

    // Draw notes
    for (const auto& note : g_activeNotes) {
        if (note.hit) continue;

        AEGfxTexture* pTex = nullptr;
        float size = NOTE_SIZE;
        float alpha = 1.0f;
        float tintR = 1.0f, tintG = 1.0f, tintB = 1.0f;

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

        if (note.missed) {
            alpha = 0.3f;
            tintR = tintG = tintB = 0.5f;
        }

        if (pTex) {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(tintR, tintG, tintB, alpha);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(alpha);
            AEGfxTextureSet(pTex, 0, 0);
        }
        else {
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
        sprintf_s(buffer, "Score: %d", g_score.totalScore);
        AEGfxPrint(g_fontId, buffer, SCORE_TEXT_X, SCORE_TEXT_Y, 1.0f, 1, 1, 1, 1);

        if (g_score.combo > 0) {
            sprintf_s(buffer, "%d", g_score.combo);
            float pulse = 1.0f + (g_hitFeedbackTimer * 0.5f);
            AEGfxPrint(g_fontId, buffer, COMBO_NUM_X, COMBO_NUM_Y, 1.5f * pulse, 1, 0.9f, 0.3f, 1);
            AEGfxPrint(g_fontId, "COMBO", COMBO_LABEL_X, COMBO_LABEL_Y, 1.5f, 1, 0.9f, 0.3f, 1);
        }

        if (g_hitFeedbackTimer > 0) {
            const char* text = "";
            float tr = 1, tg = 1, tb = 1;
            switch (g_lastHitRating) {
            case HIT_PERFECT: text = "PERFECT!"; tr = 1; tg = 0.8f; tb = 0.2f; break;
            case HIT_GOOD:    text = "GOOD";     tr = 0.3f; tg = 1; tb = 0.3f; break;
            case HIT_MISS:    text = "MISS";     tr = 1; tg = 0.3f; tb = 0.3f; break;
            default: break;
            }
            AEGfxPrint(g_fontId, text, FEEDBACK_TEXT_X, FEEDBACK_TEXT_Y, 1.2f, tr, tg, tb, 1);
        }

        sprintf_s(buffer, "Perfect: %d  Good: %d  Miss: %d",
            g_score.perfectHits, g_score.goodHits, g_score.misses);
        AEGfxPrint(g_fontId, buffer, STATS_TEXT_X, STATS_TEXT_Y, 0.7f, 0.8f, 0.8f, 0.8f, 1);

        if (g_songFinished) {
            AEGfxPrint(g_fontId, "SONG COMPLETE!", COMPLETE_TEXT_X, COMPLETE_TEXT_Y, 2.0f, 0.2f, 1.0f, 0.2f, 1);
            sprintf_s(buffer, "Final Score: %d", g_score.totalScore);
            AEGfxPrint(g_fontId, buffer, FINAL_SCORE_X, FINAL_SCORE_Y, 1.2f, 1, 1, 1, 1);
            AEGfxPrint(g_fontId, "Press E to return", RETURN_TEXT_X, RETURN_TEXT_Y, 0.8f, 0.8f, 0.8f, 0.8f, 1);
        }

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

    // Free note and UI textures
    for (int i = 0; i < 3; i++) {
        if (g_pTexFruitNotes[i])  AEGfxTextureUnload(g_pTexFruitNotes[i]);
        if (g_pTexSeedNotes[i])   AEGfxTextureUnload(g_pTexSeedNotes[i]);
        g_pTexFruitNotes[i] = nullptr;
        g_pTexSeedNotes[i] = nullptr;
    }
    // These are aliases into the arrays above — do NOT double-free them
    g_pTexNormalNote = nullptr;
    g_pTexPremiumNote = nullptr;

    if (g_pTexWateringCan) AEGfxTextureUnload(g_pTexWateringCan);
    if (g_pTexDiffSelectBg) AEGfxTextureUnload(g_pTexDiffSelectBg);
    g_pTexWateringCan = nullptr;
    g_pTexDiffSelectBg = nullptr;

    // Free per-difficulty gameplay backgrounds
    for (int i = 0; i < 3; i++) {
        if (g_pTexGameplayBg[i]) AEGfxTextureUnload(g_pTexGameplayBg[i]);
        g_pTexGameplayBg[i] = nullptr;
    }

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

// ================= DIFFICULTY =================

void Rhythm_SetDifficulty(RhythmDifficulty difficulty) {
    g_difficulty = difficulty;
    g_difficultyConfig = DIFFICULTY_CONFIGS[difficulty];
}

RhythmDifficulty Rhythm_GetDifficulty() {
    return g_difficulty;
}

const RhythmDifficultyConfig& Rhythm_GetDifficultyConfig() {
    return g_difficultyConfig;
}

// ================= GETTERS =================

float Rhythm_GetSongDuration() {
    return g_songDuration;
}

float Rhythm_GetCurrentTime() {
    return g_songTime;
}

const RhythmScore& Rhythm_GetScore() {
    return g_score;
}

RhythmRewardTier Rhythm_GetRewardTier()
{
    int score = g_score.totalScore;
    switch (g_difficulty)
    {
    case DIFFICULTY_EASY:
        if (score >= 20000) return REWARD_GOOD;
        if (score >= 8000)  return REWARD_AVERAGE;
        return REWARD_POOR;
    case DIFFICULTY_MEDIUM:
        if (score >= 45000) return REWARD_GOOD;
        if (score >= 15000) return REWARD_AVERAGE;
        return REWARD_POOR;
    case DIFFICULTY_HARD:
        if (score >= 80000) return REWARD_GOOD;
        if (score >= 25000) return REWARD_AVERAGE;
        return REWARD_POOR;
    }
    return REWARD_POOR;
}

void Rhythm_SetMusicEnabled(bool enabled)
{
    gMusicEnabled = enabled;

    if (!AEAudioIsValidGroup(g_musicGroup))
        return;

    if (enabled)
        AEAudioResumeGroup(g_musicGroup);
    else
        AEAudioPauseGroup(g_musicGroup);
}

void Rhythm_SetSeedType(int seedType)
{
    // Clamp to valid range  0=Apple  1=Pear  2=Banana
    if (seedType < 0 || seedType > 2) seedType = 0;
    g_pTexNormalNote = g_pTexFruitNotes[seedType];
    g_pTexPremiumNote = g_pTexSeedNotes[seedType];
    printf("Rhythm_SetSeedType: seedType=%d — normal=%s, premium=%s\n",
        seedType,
        g_pTexNormalNote ? "OK" : "NULL",
        g_pTexPremiumNote ? "OK" : "NULL");
}