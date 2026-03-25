#include "Economy.h"
#include <Windows.h>
#include "AEEngine.h"
#include "AEUtil.h"
#include <stdio.h>
#include <utility>
#include <AETypes.h>
#include <stdlib.h>
#include <time.h>
#include "Profile.h"
#include "Inventory.h"
#include "Crate.h"
<<<<<<< HEAD
=======



//MAIN FLOW FOR GAME
//Player harvests and then fruit goes into crate
/*
Timer triggers
System checks crate
Random amount sold
Money added 
Crate stock decreases *
*/

>>>>>>> Tiara

// Global variables - remove 'static' since they're extern in the header
u64 total_money = 0;
u64 max_money = 255; //depend on shop upgrades later
f32 money_multiplier = 1.0f;

static f32 timer = 0.0f;
static f32 next_sale_time = 0.0f; //seconds

bool timer_reset = true;

//placeholder for base price
<<<<<<< HEAD
extern u8 base_price_apple = 0;
=======
//extern u8 base_price_apple = 0;
u8 base_price_apple = 10;
>>>>>>> Tiara

//Helper functions
f32 random_time(f32 min, f32 max) {
	return min + (max - min) * AERandFloat();
}
u8 random_range(u8 min, u8 max) {
	return (u8)(min + (max - min) * AERandFloat());
}
std::pair<f32, f32> random_range_pair(f32 min1, f32 max1, f32 min2, f32 max2) {
	f32 rng = AERandFloat();
	if (rng < 0.3f) {
		return std::make_pair(min1, max1);
	}
	else {
		return std::make_pair(min2, max2);
	}
}

void static sell_fruit()
{
	int crateStock = Crate_GetFruitCount(0); // apple crate

<<<<<<< HEAD
	//determine sale price
	u64 total_price = static_cast<u64>(sale_amount) * static_cast<u64>(base_price_apple) * static_cast<u64>(money_multiplier);
	//add money to total
	Economy_AddMoney(static_cast<int>(total_price));
	//remove fruit from inventory
	Inventory_RemoveFruit(sale_amount);
=======
	if (crateStock <= 0)
		return;
>>>>>>> Tiara

	// determine sale amount (1–3 but not exceeding stock)
	u8 sale_amount = (u8)min(crateStock, random_range(1, 3));

	// calculate price
	u64 total_price = static_cast<u64>(sale_amount) *
		static_cast<u64>(base_price_apple) *
		static_cast<u64>(money_multiplier);

	// add money
	Economy_AddMoney((int)total_price);

	// REMOVE FROM CRATE (IMPORTANT FIX)
	Crate_RemoveFruitAmount(0, sale_amount);

	printf("Sold %d apples from crate.\n", sale_amount);
	printf("+%llu GOLD!\n", total_price);
}

// lifecycle
void Economy_Init() {
	//initialize random seed
	srand((unsigned int)time(NULL));

	total_money = 0; //to read from config file 
	money_multiplier = 1.0f; //to read from config file

	timer = 0.0f; //initialize timer

	//randomize next sale time
	std::pair<float, float> range_pair = random_range_pair(5.0f, 10.0f, 4.0f, 20.0f); //random time (fast, slow)between sales
	f32 first_sale_time = range_pair.first;
	f32 second_sale_time = range_pair.second;

	next_sale_time = random_time(first_sale_time, second_sale_time);


}
void Economy_Update(float dt) {
	timer += dt;

	if (timer >= next_sale_time && total_money <= max_money) {
<<<<<<< HEAD
		bool in_stock = Crate_GetFruitCount(0) > 0;
=======
		int crateStock = Crate_GetFruitCount(0);
		bool in_stock = (crateStock > 0);
>>>>>>> Tiara

		if (in_stock) {
			sell_fruit();
			printf("Sale! Money: %llu | Stock: %d\n", total_money, Inventory_GetFruitStock());
		}
		else {
			printf("No stock to sell!\n");
		}

		timer = 0.0f;
		std::pair<float, float> range_pair = random_range_pair(10.0f, 20.0f, 5.0f, 40.0f);
		next_sale_time = random_time(range_pair.first, range_pair.second);
		printf("Next sale in %.2f seconds.\n", next_sale_time);
	}

	/*if (total_money >= max_money) {
		total_money = max_money;
	}*/
}

//void Economy_Exit();
//
//// commands (change state)

void Economy_AddMoney(int amount) {
	total_money += static_cast<u64>(amount);
	Economy_SaveToProfile(Profile_GetActiveSlot());
<<<<<<< HEAD
=======

	total_money += static_cast<u64>(amount);

	if (total_money > max_money)
		total_money = max_money;
>>>>>>> Tiara
}

bool Economy_SpendMoney(int amount) {
	if (total_money >= static_cast<u64>(amount)) {
		total_money -= static_cast<u64>(amount);
		Economy_SaveToProfile(Profile_GetActiveSlot());
		return true;
	}
	else {
		return false;
	}
}
void Economy_SetMaxMoney(int amount) {
	max_money = static_cast<u64>(amount);
	Economy_SaveToProfile(Profile_GetActiveSlot());
}

void Economy_SetMultiplier(float mult) {
	money_multiplier = mult;
	Economy_SaveToProfile(Profile_GetActiveSlot());
}

void Economy_SetBasePriceApple(u8 price) {
	base_price_apple = price;
	Economy_SaveToProfile(Profile_GetActiveSlot());
}


// getters (read-only)
int Economy_GetTotalMoney() {
	return static_cast<int>(total_money);
}

int Economy_GetMaxMoney() {
	return static_cast<int>(max_money);
}
float Economy_GetMultiplier() {
	return money_multiplier;
}

