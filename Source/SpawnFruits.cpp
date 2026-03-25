#include "SpawnFruits.h"
#include "AEEngine.h"
#include "Economy.h"
#include <vector>
#include <stdio.h>
#include <cstdlib>  // For random number generation
#include <time.h>    // For time-based randomness
#include "Inventory.h" // For inventory interaction when collecting fruits



// Ground Y in world coordinates (center = 0). Adjust if your ground is higher/lower.
float GROUND_Y = -350.0f;

// Physics Tunables
const float gravity = 80.0f;   // multiply falling speed to get pixels/sec
const float FRICTION = 0.88f;          // friction applied to horizontal speed each frame
const float ROTATION_FACTOR = 0.92f;    // degrees of rotation per horizont
const float ANGULAR_FRICTION = 0.92f;       // friction applied to angular velocity each frame
const float ANGVEL_FROM_SPEED = 0.5f;    // multiplier for how much horizontal speed affects angular velocity

std::vector<Fruit> fruits;  // A list to store all apples

// Animation for when a fruit is collected (scaling up and moving upwards)
//------------------------------------------------------------------------//
struct CollectAnim {
    bool active = false;
    bool anim_moveup = false; // if false, anim moves down (can be used for "bounce" effect if desired)
    float timer = 0.0f;
    float duration = 0.5f;      // seconds
    float startScale = 1.0f;
    float curScale = 1.0f;
    float velocityY = 300.0f;   // pixels/sec upward during collect
};

static std::vector<CollectAnim> collectAnims; // parallel to 'fruits'
//------------------------------------------------------------------------//

// Helper Function
//------------------------------------------------------------------------//

// helper function to generate random float between min and max
static float random_float(float min, float max) {
    if (max <= min) return min;
    return min + (max - min) * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
}

// percentage should be between 0 and 100 (e.g. 25 for 25% chance)
static bool rand_chance(float percentage) {
	float rand_num = random_float(0.0f, 100.0f);
    return (rand_num < percentage);
}


//------------------------------------------------------------------------//

// Spawn Timer
//------------------------------------------------------------------------//

float spawnTimer = 0.0f;  // Timer for spawning apples
f32 spawnInterval = 10.0f;  // Initial interval between apple spawns (in seconds)

//------------------------------------------------------------------------//

// Global variable 
AEGfxVertexList* pMeshApple = NULL;
static AEGfxTexture* gAppleTexture = nullptr;

void SpawnFruit_Init() {
    srand((unsigned int)time(NULL));

    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f, 0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f, 0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f, -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    pMeshApple = AEGfxMeshEnd();

    gAppleTexture = AEGfxTextureLoad("Assets/Fruit_Apple.png");

	spawnInterval = random_float(10.0f, 30.0f); // Randomize initial spawn interval between 10 and 30 seconds
	printf("Initial spawn interval: %.2f seconds\n", spawnInterval);
}

void SpawnFruit() {
    Fruit newApple;
    newApple.texture = gAppleTexture;  // Load the apple texture

    newApple.x = random_float(-200.0, 600.0);  // Random X position between 400 to 1000
    newApple.y = 1500;  // Start the apple at the top of the screen (y = 0)
    printf("Spawned apple at (%.2f, %.2f)\n", newApple.x, newApple.y);

    newApple.rotation = static_cast<f32>(rand() & 180);  // Start with no rotation

    newApple.speedY = static_cast<f32>((rand() % 4) + 3);  // Random falling speed between 2 and 6
    newApple.speedX = 0.0f;  // Start with no horizontal speed
    newApple.angularVelocity = 0.0f;  // Start with no rotation

    newApple.isFalling = true;  // Start it falling
    newApple.isCollected = false;  // It hasn?t been clicked yet
    newApple.rollDirection = (rand() % 2 == 0) ? 1.0f : -1.0f;  // Randomly decide left (-1) or right (+1) roll

    fruits.push_back(newApple);  // Add the new apple to the apple list
    collectAnims.push_back(CollectAnim()); // keep anim vector in sync
}

static void SpawnFruit(float x) {
    Fruit newApple;
    newApple.texture = AEGfxTextureLoad("Assets/Fruit_Apple.png");  // Load the apple texture

    newApple.x = x;  // Random X position between 400 to 1000
    newApple.y = 100;  // Start the apple at the top of the screen (y = 0)
    printf("Spawned apple at (%.2f, %.2f)\n", newApple.x, newApple.y);

    newApple.rotation = static_cast<f32>(rand() & 180);  // Start with no rotation

    newApple.speedY = static_cast<f32>((rand() % 4) + 3);  // Random falling speed between 2 and 6
    newApple.speedX = 0.0f;  // Start with no horizontal speed
    newApple.angularVelocity = 0.0f;  // Start with no rotation

    newApple.isFalling = true;  // Start it falling
    newApple.isCollected = false;  // It hasn?t been clicked yet
    newApple.rollDirection = (rand() % 2 == 0) ? 1.0f : -1.0f;  // Randomly decide left (-1) or right (+1) roll

    fruits.push_back(newApple);  // Add the new apple to the apple list
    collectAnims.push_back(CollectAnim()); // keep anim vector in syn
}

void SpawnMultipleFruits(int count) {
    for (int i = 0; i < count; ++i) {
        SpawnFruit(random_float(-200.0, 200.0));
    }
}

// multiple spawns for testing, can be triggered by rhythm game reward later
//--------------------------------------------------------------------------//

f32 currentTimer = 0.0f;  // This variable is not used in the current implementation, but can be used for timing-based logic if needed
f32 currentDelay = 1.5f;  // 5 seconds delay for testing purposes, can be adjusted or randomized as needed

bool spawnMultiple = false;  // Flag to control whether to spawn multiple fruits at once
const float targetY = -240.0f; // how high the apple should move up during collect animation

//--------------------------------------------------------------------------//

// Update function: Updates the position of all apples
void UpdateSpawnFruits(float dt) {

    //if (AEInputCheckReleased(AEVK_SPACE)) { // Simulate reward after rhythm game by pressing spacebar, can be removed later
    //    spawnMultiple = true;
    //}

    if (spawnMultiple) {
        currentTimer += dt;
        if (currentTimer >= currentDelay) {
            currentTimer = 0.0f;  // Reset timer
            spawnMultiple = false;  // Reset flag
            SpawnMultipleFruits(10);  // Spawn 5 apples at once for testing
        }
        if (currentTimer == 0.0f) {
            printf("Spawned 10 apples at once!\n");
        }
    }

    //animation
    //-------------------------------------------------------------------------//
    // iterate by index so erases keep anim vector in sync
    for (size_t i = 0; i < fruits.size(); )
    {
        Fruit& apple = fruits[i];
        CollectAnim& anim = collectAnims[i];

        if (apple.isCollected && anim.active)
        {
            // advance animation timer (per-phase)
            anim.timer += dt;

            // move up phase
            if (!anim.anim_moveup)
            {
                apple.y += anim.velocityY * dt;

                // when we reach or pass the target, flip to shrink phase and reset timer
                if (apple.y >= targetY)
                {
                    anim.anim_moveup = true;
                    anim.timer = 0.0f;           // <-- reset so shrink has its full duration
                }
            }

            // shrink phase uses timer normalized to [0,1]
            if (anim.anim_moveup)
            {
                float t = anim.duration > 0.0f ? (anim.timer / anim.duration) : 1.0f;
                if (t > 1.0f) t = 1.0f;

                // exponential easing (ease-in) ? slower at start, faster near the end
                const float expo = 5.0f; // increase for steeper falloff, decrease for gentler
                anim.curScale = anim.startScale * powf(1.0f - t, expo);

                // clamp very small values to zero to avoid tiny draws
                if (anim.curScale < 0.001f) anim.curScale = 0.0f;
            }

            // finish
            if (anim.timer >= anim.duration)
            {
                // animation finished -> remove fruit and anim entry
                // AddToInventory() here if needed
                fruits.erase(fruits.begin() + i);
                collectAnims.erase(collectAnims.begin() + i);
                printf("Apple collected & removed after animation.\n");
                continue; // don't increment i (we removed this index)
            }
            else
            {
                ++i;
            }
        }
        else
        {
            ++i;
        }
    }
    //-------------------------------------------------------------------------//

    //physics
    for (auto& apple : fruits) {
        if (apple.isFalling) {
            apple.y -= apple.speedY * dt * gravity;  // Move apple down (fall)

            if (apple.y <= GROUND_Y) {  // Check if apple hits the ground 
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

            if (apple.rollDirection <= -1.0f) { // If rolling left
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

//// Render function: Renders all apples on the screen
//void RenderSpawnFruits() {
//    for (const auto& apple : fruits) {
//        if (!apple.isCollected) {// if havent collect yet
//
//            // Render apple at its current position
//            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);  // Use texture rendering mode
//            AEGfxTextureSet(apple.texture, 0, 0);  // Set the texture
//
//            AEMtx33 scale, trans, rotation, transform, rotscale;
//			AEMtx33Scale(&scale, 47.0f, 47.0f);  
//
//            f32 rotationInRadians = apple.rotation * (3.14159265358979323846f / 180.0f);  // Convert degrees to radian
//			AEMtx33Rot(&rotation, rotationInRadians);  // Create rotation matrix based on current rotation
//
//            AEMtx33Trans(&trans, apple.x, apple.y);  // Apply position transformation
//
//            AEMtx33Concat(&rotscale, &rotation, &scale);
//            AEMtx33Concat(&transform, &trans, &rotscale);
//
//            AEGfxSetTransform(transform.m);  // Set the transformation matrix for the apple
//
//            AEGfxMeshDraw(pMeshApple, AE_GFX_MDM_TRIANGLES);  // Draw the apple as a quad
//        }
//    }
//}

// --- RenderSpawnFruits: use per-fruit scale when drawing ---
void RenderSpawnFruits() {
    for (size_t i = 0; i < fruits.size(); ++i) {
        const Fruit& apple = fruits[i];
        const CollectAnim& anim = collectAnims[i];

        if (!apple.isCollected || anim.active) { // allow rendering during collect animation
            // Render apple at its current position
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);  // Use texture rendering mode
            AEGfxTextureSet(apple.texture, 0, 0);  // Set the texture

            AEMtx33 scale, trans, rotation, transform, rotscale;

            float baseSize = 47.0f;
            float drawSize = baseSize * (anim.active ? anim.curScale : 1.0f);
            AEMtx33Scale(&scale, drawSize, drawSize);

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
//--------------------------------------------------------------------------//
void UpdateFruitSpawner(float dt) {
    spawnTimer += dt;
    
    if (spawnTimer >= spawnInterval) {
        SpawnFruit();  // Spawn fruit when timer exceeds the interval
        spawnTimer = 0.0f;  // Reset the spawn timer
        spawnInterval = random_time(1, 400);  // Random spawn interval (between 1 and 400 seconds / 6.67 mins)
        printf("Next fruit in %.2f seconds.\n", spawnInterval);
    }
}
//--------------------------------------------------------------------------//

//void CheckForFruitClicks(s32 mouseX, s32 mouseY) {
//	    
//    if (!AEInputCheckCurr(AEVK_LBUTTON)) {  // If not clicked, exit the function
//		return;  
//    }
//
//    // Convert screen coordinates to world coordinates
//    float worldX = (float)mouseX - 800.0f;
//    float worldY = 450.0f - (float)mouseY;
//
//    for (auto apple = fruits.begin(); apple != fruits.end(); )
//    {
//        if (!apple->isCollected && !apple->isFalling)
//        {
//            float halfSize = 47.0f * 0.5f;
//
//            if (worldX > apple->x - halfSize &&
//                worldX < apple->x + halfSize &&
//                worldY > apple->y - halfSize &&
//				worldY < apple->y + halfSize) // Check if click is within apple bounds
//            {
//                apple->isCollected = true;
//                printf("Apple clicked!\n");
//
//            }
//        }
//        if (apple->isCollected){
//            //AddToInventory();  // Add to inventory when clicked
//			
//            //animation
//			//apple moves up quickly and shrinks down to show it being collected
//
//
//			apple = fruits.erase(apple);  // Remove apple from the world
//			printf("Apple removed from the world.\n");
//
//			//visual feedback for collecting apple could be added here ( + 1 floating text, sound effect, etc.)
//
//            //+1 floating text
//
//			//smoke sprite to show the apple being collected or a quick sparkle effect
//
//            //sound effect
//            
//		}
//        else {
//            ++apple;  // Move to next apple
//        }
//    }
//}

void CheckForFruitClicks(s32 mouseX, s32 mouseY) {
    if (!AEInputCheckCurr(AEVK_LBUTTON)) {  // If not clicked, exit the function
        return;
    }
	bool clickOnce = false; // Flag to track if we've handled a click on an apple
    // Convert screen coordinates to world coordinates
    float worldX = (float)mouseX - 800.0f;
    float worldY = 450.0f - (float)mouseY;

    for (size_t i = 0; i < fruits.size(); ++i)
    {
        Fruit& apple = fruits[i];
        CollectAnim& anim = collectAnims[i];

        if (!apple.isCollected && !apple.isFalling)
        {
            float halfSize = 47.0f * 0.5f;

            if (worldX > apple.x - halfSize &&
                worldX < apple.x + halfSize &&
                worldY > apple.y - halfSize &&
                worldY < apple.y + halfSize) // Check if click is within apple bounds
            {
                apple.isCollected = true;
                // start collect animation
                anim.active = true;
                anim.timer = 0.0f;
                anim.curScale = anim.startScale = 1.0f;
                anim.duration = 0.5f;
                anim.velocityY = 300.0f;
                printf("Apple clicked! starting collection animation at (%.2f, %.2f)\n", apple.x, apple.y);
				clickOnce = true; // Set flag to indicate we've handled a click
            }
        }

        else if (clickOnce)
        {
            if (GetInventoryCount() < GetInventoryLimit()) {
                // Add to inventory when clicked (can be moved to animation finish if you want it to add after animation instead of at start)
                Inventory_AddFruit(1, 0);
                printf("Added 1 apple to inventory. Total fruits: %d\n", Inventory_GetFruitStock());

                //20% chance to also give an apple seed when collecting an apple, can be adjusted as needed
                if (rand_chance(20)) {
                    Inventory_AddSeed(1, 0);
                }
            }

            else {
				printf("Inventory full! Cannot add more fruits.\n");
                int add_money = random_range(1, 5);
                Economy_AddMoney(add_money);
				printf("Get money $%d instead!\n", add_money);
            }

			clickOnce = false; // Reset flag after handling the click

        }
    }
}

// Collect fruits by proximity to a moving object (center in world coords, size in pixels).
// Rectangular AABB test (default). Set useCircle=true to use radius-based test.
void Proximity_CheckForFruitClicks(f32 objX, f32 objY, f32 objWidth, f32 objHeight, bool useCircle /*= false*/)
{
    const float fruitHalf = 47.0f * 0.5f; // same half-size used for clicks
    const float fruitRadius = 47.0f * 0.5f;

    for (size_t idx = 0; idx < fruits.size(); ++idx)
    {
        Fruit& f = fruits[idx];
        CollectAnim& anim = collectAnims[idx];

        if (f.isCollected || f.isFalling)
            continue;

        bool collect = false;

        if (!useCircle)
        {
            float objHalfW = objWidth * 0.5f;
            float objHalfH = objHeight * 0.5f;
            bool overlapX = (objX + objHalfW) >= (f.x - fruitHalf) &&
                (objX - objHalfW) <= (f.x + fruitHalf);
            bool overlapY = (objY + objHalfH) >= (f.y - fruitHalf) &&
                (objY - objHalfH) <= (f.y + fruitHalf);
            collect = overlapX && overlapY;
        }
        else
        {
            float dx = objX - f.x;
            float dy = objY - f.y;
            float distSq = dx * dx + dy * dy;
            float radius = (f32)((fmaxf(objWidth, objHeight) * 0.5f) + fruitRadius);
            collect = distSq <= (radius * radius);
        }

        if (collect)
        {
            // start collect animation instead of immediate erase so user sees feedback
            f.isCollected = true;
            anim.active = true;
            anim.timer = 0.0f;
            anim.curScale = anim.startScale = 1.0f;
            anim.duration = 0.5f;
            anim.velocityY = 300.0f;
            anim.anim_moveup = false;
            printf("Apple collected by proximity at (%.2f, %.2f) -> starting animation\n", f.x, f.y);
        }
    }
}