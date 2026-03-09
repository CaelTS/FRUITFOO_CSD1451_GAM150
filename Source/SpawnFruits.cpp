
#include "SpawnFruits.h"
#include "AEEngine.h"
#include "Economy.h"
#include <vector>
#include <stdio.h>
#include <cstdlib>  // For random number generation
#include <time.h>    // For time-based randomness

//helper function to generate random float between min and max
static float random_float(float min, float max) {
    if (max <= min) return min;
    return min + (max - min) * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
}

// Ground Y in world coordinates (center = 0). Adjust if your ground is higher/lower.
float GROUND_Y = -350.0f;

// Tunables
const float gravity = 80.0f;   // multiply falling speed to get pixels/sec
const float FRICTION = 0.88;          // friction applied to horizontal speed each frame
const float ROTATION_FACTOR = 0.92f;    // degrees of rotation per horizont
const float ANGULAR_FRICTION = 0.92f;       // friction applied to angular velocity each frame
const float ANGVEL_FROM_SPEED = 0.5f;    // multiplier for how much horizontal speed affects angular velocity

std::vector<Fruit> fruits;  // A list to store all apples

float spawnTimer = 0.0f;  // Timer for spawning apples
f32 spawnInterval = 5.0f;  // Initial interval between apple spawns (in seconds)

// Global variable declaration

AEGfxVertexList* pMeshApple = NULL;

void SpawnFruit_Init() {
    srand((unsigned int)time(NULL));

    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    pMeshApple = AEGfxMeshEnd();
}

void SpawnFruit() {
    Fruit newApple;
    newApple.texture = AEGfxTextureLoad("Assets/Fruit_Apple.png");  // Load the apple texture

    newApple.x = random_float(-200.0,600.0);  // Random X position between 400 to 1000
    newApple.y = 1500;  // Start the apple at the top of the screen (y = 0)
	printf("Spawned apple at (%.2f, %.2f)\n", newApple.x, newApple.y);

	newApple.rotation = static_cast<f32>(rand() & 180);  // Start with no rotation

    newApple.speedY = static_cast<f32>((rand() % 4) + 3);  // Random falling speed between 2 and 6
	newApple.speedX = 0.0f;  // Start with no horizontal speed
	newApple.angularVelocity = 0.0f;  // Start with no rotation

    newApple.isFalling = true;  // Start it falling
    newApple.isCollected = false;  // It hasn’t been clicked yet
    newApple.rollDirection = (rand() % 2 == 0) ? 1.0f : -1.0f;  // Randomly decide left (-1) or right (+1) roll

    fruits.push_back(newApple);  // Add the new apple to the apple list
}

// Update function: Updates the position of all apples
void UpdateSpawnFruits(float dt) {
    for (auto& apple : fruits) {
        if (apple.isFalling) {
            apple.y -= apple.speedY * dt * gravity;  // Move apple down (fall)

            if (apple.y  <= GROUND_Y) {  // Check if apple hits the ground 
				apple.y = GROUND_Y;  // Snap to ground level
				GROUND_Y = random_float(-400.0f, -350.0f);  // Randomize next ground level for visual variety
				printf("Apple hit the ground at (%.2f, %.2f)\n", apple.x, apple.y);

                apple.isFalling = false;  // Stop falling
                apple.speedY = 0.0f;  // Stop vertical movement

                // Give an immediate horizontal impulse on contact
                apple.speedX = 100 * apple.rollDirection;  // px/sec

                // Set angular velocity based on horizontal speed so rotation is driven by motion
                apple.angularVelocity = apple.speedX * ANGVEL_FROM_SPEED;

                // Small visual offset so it doesn't look stuck
                apple.x += apple.rollDirection * static_cast<f32>(rand() % 6 + 2);

            }
        }

        else {

            apple.x += apple.speedX * dt;
			apple.speedX *= FRICTION;  // Apply friction to slow down horizontal speed

			if (apple.rollDirection <= -1.0f ) { // If rolling left
                apple.angularVelocity *= ANGULAR_FRICTION; 
                apple.rotation += apple.angularVelocity * dt;  // Rotate left
				/*printf("Rotation: %.2f degrees\n", apple.rotation);*/
            }
			else { // If rolling right
                apple.angularVelocity *= ANGULAR_FRICTION;
                apple.rotation -= apple.angularVelocity * dt;  // Rotate right
			}
        }

        // keep rotation in a reasonable range to avoid large numbers
        if (apple.rotation > 360.0f) apple.rotation -= 360.0f;
        else if (apple.rotation < -360.0f) apple.rotation += 360.0f;
    }
}

// Render function: Renders all apples on the screen
void RenderSpawnFruits() {
    for (const auto& apple : fruits) {
        if (!apple.isCollected) {// if havent collect yet

            // Render apple at its current position
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);  // Use texture rendering mode
            AEGfxTextureSet(apple.texture, 0, 0);  // Set the texture

            AEMtx33 scale, trans, rotation, transform, rotscale;
			AEMtx33Scale(&scale, 47.0f, 47.0f);  

            f32 rotationInRadians = apple.rotation * (3.14159265358979323846f / 180.0f);  // Convert degrees to radian
			AEMtx33Rot(&rotation, rotationInRadians);  // Create rotation matrix based on current rotation

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
        spawnInterval = random_time(1,5);  // Random spawn interval (between 3 and 5 seconds)
		printf("Next fruit in %.2f seconds.\n", spawnInterval);
    }
}

void CheckForFruitClicks(s32 mouseX, s32 mouseY) {
	    
    if (!AEInputCheckCurr(AEVK_LBUTTON)) {  // If not clicked, exit the function
		return;  
    }

        // Convert screen coordinates to world coordinates
        float worldX = (float)mouseX - 800.0f;
        float worldY = 450.0f - (float)mouseY;

    for (auto apple = fruits.begin(); apple != fruits.end(); )
    {
        if (!apple->isCollected && !apple->isFalling)
        {
            float halfSize = 47.0f * 0.5f;

            if (worldX > apple->x - halfSize &&
                worldX < apple->x + halfSize &&
                worldY > apple->y - halfSize &&
				worldY < apple->y + halfSize) // Check if click is within apple bounds
            {
                apple->isCollected = true;
                printf("Apple clicked!\n");

            }
        }
        if (apple->isCollected){
            //AddToInventory();  // Add to inventory when clicked
				
			apple = fruits.erase(apple);  // Remove apple from the world
			printf("Apple removed from the world.\n");

			//visual feedback for collecting apple could be added here ( + 1 floating text, sound effect, etc.)

            //+1 floating text

			//smoke sprite to show the apple being collected or a quick sparkle effect
			//or maybe apple moves up quickly and shrinks down to show it being collected with sparkle effect

            //sound effect
            
		}
        else {
            ++apple;  // Move to next apple
        }
    }
}

// function to make when rhythm game gives you rewards