
#include "SpawnFruits.h"
#include "AEEngine.h"
#include "Economy.h"
#include <vector>
#include <stdio.h>
#include <cstdlib>  // For random number generation
#include <ctime>    // For time-based randomness

std::vector<Fruit> fruits;  // A list to store all apples

float spawnTimer = 0.0f;  // Timer for spawning apples
f32 spawnInterval = 5.0f;  // Initial interval between apple spawns (in seconds)

// Global variable declaration

AEGfxVertexList* pMeshApple = NULL;

void SpawnFruit_Init() {
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    pMeshApple = AEGfxMeshEnd();
}

void SpawnFruit() {
    Fruit newApple;
    newApple.texture = AEGfxTextureLoad("Assets/apple.png");  // Load the apple texture
    newApple.x = rand() % 1600;  // Random X position between 0 and screen width (1600px)
    newApple.y = 0;  // Start the apple at the top of the screen (y = 0)
	newApple.rotation = 0.0f;  // Start with no rotation
    newApple.speedY = rand() % 5 + 2;  // Random falling speed between 2 and 6
    newApple.isFalling = true;  // Start it falling
    newApple.isCollected = false;  // It hasn’t been clicked yet
    newApple.rollDirection = (rand() % 2 == 0) ? 1 : -1;  // Randomly decide left (-1) or right (+1) roll

    fruits.push_back(newApple);  // Add the new apple to the apple list
}

// Update function: Updates the position of all apples
void UpdateSpawnFruits(float dt) {
    for (auto& apple : fruits) {
        if (apple.isFalling) {
            apple.y += apple.speedY * dt;  // Move apple down (fall)
            if (apple.y >= 800) {  // Check if apple hits the ground (assuming ground is at y=800)
                apple.isFalling = false;  // Stop falling
                apple.speedY = 0.0f;  // Stop vertical movement
                apple.x += apple.rollDirection * rand() % 5 + 2;  // Roll it left or right
				apple.x -= apple.rollDirection * 0.1f;  // Gradually slow down horizontal movement [FRICTION]
				apple.rotation += apple.rollDirection * 5.0f;  // Rotate it for rolling effect
				apple.rotation -= apple.rollDirection * 0.1f;  // Gradually slow down rotation [FRICTION]
            }
        }
    }
}

// Render function: Renders all apples on the screen
void RenderFruits() {
    for (const auto& apple : fruits) {
        if (!apple.isCollected) {// if havent collect yet

            // Render apple at its current position
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);  // Use texture rendering mode
            AEGfxTextureSet(apple.texture, 0, 0);  // Set the texture

            AEMtx33 scale, trans, rotation, transform, rotscale;
			AEMtx33Scale(&scale, 47.0f, 47.0f);  // Scale the apple to 64x64 pixels
			AEMtx33Rot(&rotation, apple.rotation);  // Create rotation matrix based on current rotation
            AEMtx33Trans(&trans, apple.x, apple.y);  // Apply position transformation

            AEMtx33Concat(&rotscale, &rotation, &scale);
            AEMtx33Concat(&transform, &trans, &rotscale);

            AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple

            AEGfxMeshDraw(pMeshApple, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
        }
    }
}

// Timer-based spawning logic (update once per frame)
void UpdateFruitSpawner(float dt) {
    spawnTimer += dt;

    if (spawnTimer >= spawnInterval) {
		SpawnFruit();  // Spawn fruit when timer exceeds the interval
        spawnTimer = 0.0f;  // Reset the spawn timer
        spawnInterval = random_time(10,20);  // Random spawn interval (between 3 and 5 seconds)
		printf("Next fruit in %.2f seconds.\n", spawnInterval);
    }
}