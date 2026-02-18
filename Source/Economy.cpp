#include "Economy.h"
#include <Windows.h>
#include "AEEngine.h"
#include "AEUtil.h"
#include <stdio.h>
#include <utility>
#include <AETypes.h>
#include <stdlib.h>
#include <time.h>

// Global variables - remove 'static' since they're extern in the header
u64 total_money = 0;
u64 max_money = 255; //depend on shop upgrades later
f32 money_multiplier = 1.0f;

static f32 timer = 0.0f;
static f32 next_sale_time = 0.0f; //seconds

static u8 total_fruits = 10;
bool timer_reset = true;

//placeholder inventory stock function
u8 static Inventory_GetFruitStock() {
	return total_fruits; //assume always have fruit for now
}
//placeholder function to remove fruit from inventory function
void Inventory_RemoveFruit(u8 amount) {
	total_fruits -= amount;
	printf("Removed %d fruits from inventory.\n", amount);
	printf("Fruits left in inventory: %d\n", total_fruits);
}

//placeholder for base price
u8 base_fruit_price = 5;

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

void static sell_fruit() {
	u8 stock = Inventory_GetFruitStock();
	//determine sale amount
	u8 sale_amount = min(stock, random_range(1, 3)); //sell between 1 and 3 fruits (use int literals)

	//determine sale price
	u64 total_price = static_cast<u64>(sale_amount) * static_cast<u64>(base_fruit_price) * static_cast<u64>(money_multiplier);
	//add money to total
	Economy_AddMoney(static_cast<int>(total_price));
	//remove fruit from inventory
	Inventory_RemoveFruit(sale_amount);

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
	static int last_second = -1;
	int current_second = (int)timer;

	if (current_second != last_second) {
		printf("Economy Timer: %d seconds.\n", current_second);
		last_second = current_second;
	}

	if (timer_reset) {
		printf("Next sale in %.2f seconds.\n", next_sale_time);
		printf("Money: %llu | Stock: %d\n", total_money, Inventory_GetFruitStock());
		timer_reset = false;
	}

	if (timer >= next_sale_time && total_money <= max_money) { //time to sell fruit	

		bool in_stock = Inventory_GetFruitStock() > 0;
		//check for fruit in stock
		if (in_stock) {
			sell_fruit();

		}
		//reset timer
		timer = 0.0f;

		//randomize next sale time
		std::pair<float, float> range_pair = random_range_pair(10.0f, 20.0f, 5.0f, 40.0f); //random time (fast, slow)between sales
		f32 first_sale_time = range_pair.first;
		f32 second_sale_time = range_pair.second;

		next_sale_time = random_time(first_sale_time, second_sale_time);

		printf("Money: %llu | Stock: %d\n", total_money, Inventory_GetFruitStock());
	}

	if (total_money >= max_money) {
		total_money = max_money; //cap money at max
		timer = 0.0f; //reset timer to try again later
		timer_reset = true;
	}

	if (Inventory_GetFruitStock() == 0) {
		//no stock, reset timer to try again later
		timer = 0.0f;
		timer_reset = true;
	}



}
//void Economy_Exit();
//
//// commands (change state)

void Economy_AddMoney(int amount) {
	total_money += static_cast<u64>(amount);
}

bool Economy_SpendMoney(int amount) {
	if (total_money >= static_cast<u64>(amount)) {
		total_money -= static_cast<u64>(amount);
		return true;
	}
	else {
		return false;
	}
}

// getters (read-only)
int Economy_GetTotalMoney() {
	return static_cast<int>(total_money);
}
float Economy_GetMultiplier() {
	return money_multiplier;
}