#pragma once
#ifndef SPAWNFRUITS_H
#define SPAWNFRUITS_H

#include <vector>
#include "AEEngine.h"

enum FruitType {
    APPLE = 0,
    PEAR = 1,
    BANANA = 2,
    NILL = -1
};

struct Fruit {
    FruitType type;
    AEGfxTexture* texture;  // The texture of the fruit (image)
    f32 x, y = 1500.0f;             // Position of the apple on the screen
	f32 scaleX = 72.0f , scaleY = 70.0f ; // Scale for rendering (can be used for animation)
    f32 rotation;          // Current rotation angle for rolling effect
    f32 speedY;           // The speed at which the apple falls
    f32 speedX = 0.0f;;           // The speed at which the apple rolls horizontally
    bool isFalling = true;         // Whether the apple is still falling
    bool isCollected = false;        // Whether the apple has been clicked and picked up
    f32 rollDirection;    // Direction of the roll (1 for right, -1 for left)
    f32 angularVelocity = 0.0f;   // Speed of rotation for rolling effect
};

extern std::vector<Fruit> fruits;

void SpawnFruit_Init();                    // Initialize resources for spawning fruits
void SpawnFruit();                         // Create a new fruit and add it to the list
bool SpawnMultipleFruits(int count, FruitType fruit);              // Spawn multiple fruits at once
void UpdateSpawnFruits(float dt);          // Update the position and state of all fruits
void UpdateFruitSpawner(float dt);        // Update all falling apples
void CheckForFruitClicks(s32 mouseX, s32 mouseY);  // Check for clicks on apples

// Check for proximity-based collection (e.g., player character collecting fruits by moving close to them)
void Proximity_CheckForFruitClicks(f32 objX, f32 objY, f32 objWidth, f32 objHeight, bool useCircle);

//void AddToInventory();                    // Add +1 to inventory when clicked

void RenderSpawnFruits();                      // Render all apples on screen
bool CollectFruit(int index);   // Collect fruit at index: starts animation, adds inventory/money, returns true if accepted

#endif // SPAWNFRUITS_H