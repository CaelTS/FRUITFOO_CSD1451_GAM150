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
#include "Main.h"
#include "UI.h"
#include <algorithm> // for std::min



//MAIN FLOW FOR GAME
//Player harvests and then fruit goes into crate
/*
Timer triggers
System checks crate
Random amount sold
Money added
Crate stock decreases *
*/


// Global variables - remove 'static' since they're extern in the header
u64 total_money = 0;
u64 max_money = 255; //depend on shop upgrades later
f32 money_multiplier = 1.0f;

static f32 timer = 0.0f;
static f32 next_sale_time = 0.0f; //seconds

bool timer_reset = true;

//placeholder for base price
//extern u8 base_price_apple = 0;
//u8 base_price_apple = 10;
u8 base_price_apple = 5;


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


// Sell from a specific crate index (crateIndex) for a given fruit type.
// Removes stock from the crate and credits money.
static void sell_fruit(int crateIndex, int fruitType) {
	(void)fruitType; // reserved for per-fruit pricing; currently uses base_price_apple
	if (crateIndex < 0) return;

	int stock = Crate_GetFruitCount(crateIndex);
	if (stock <= 0) return;

	// determine sale amount: 1..3 but no more than available stock
	u8 sale_amount = static_cast<u8>(min(stock, static_cast<int>(random_range(1, 3))));

	// try to remove from crate first
	bool removed = Crate_RemoveFruitAmount(crateIndex, sale_amount);
	if (!removed) {
		// removal failed (race or insufficient), bail
		return;
	}

	// determine sale price
	// NOTE: currently using base_price_apple as generic price. Replace with a lookup per fruitType later.
	u64 total_price = static_cast<u64>(sale_amount) * static_cast<u64>(base_price_apple);

	// add money to total
	Economy_AddMoney(static_cast<int>(total_price));
	// If you also keep Inventory in sync with crates, update Inventory here if needed.
	// Inventory_RemoveFruit(sale_amount); // no longer used for crate-based sales

}

// lifecycle
void Economy_Init() {
	srand((unsigned int)time(NULL));

	timer = 0.0f;

	//  LOAD instead of reset
	int slot = Profile_GetActiveSlot();
	if (slot >= 0)
	{
		Economy_LoadFromProfile(slot);
	}

	// randomize next sale time
	std::pair<float, float> range_pair = random_range_pair(5.0f, 10.0f, 4.0f, 20.0f);
	next_sale_time = random_time(range_pair.first, range_pair.second);

	printf("base price apple: %d\n", base_price_apple);
}
void Economy_Update(float dt) {
	timer += dt;

	if (!(timer >= next_sale_time) || total_money >= max_money)
		return;

	// Trigger sale window only when timer passes and we haven't hit max money
	if (timer >= next_sale_time && total_money < max_money) {
		const auto& baskets = GetFruitBaskets();
		bool anySale = false;


		for (const auto& b : baskets) {
			// check stock (b.stock is assumed to be the crate/stock id)
			bool in_stock = Crate_GetFruitCount(b.stock) > 0;        // <<--------------------------- Assuming b.stock corresponds to crate index; adjust if needed

			if (in_stock) {
				// sell from this crate/basket
				sell_fruit(b.stock, b.fruitType);
				anySale = true;

				// Log using the stock id to report remaining stock
				printf("Sale! Money: %llu | Stock remaining (id %d): %d\n",
					total_money, b.stock, Crate_GetFruitCount(b.stock));

				// If we've reached max, clamp and break out
				if (total_money >= max_money) {
					total_money = max_money;
					break;
				}
			}
			else {
				// Optional: keep minimal logging
				//printf("No stock to sell for stock id %d\n", b.stock);
			}

			// Schedule next sale once per window (not per-basket)
			if (anySale) {
				timer = 0.0f;
				auto range_pair = random_range_pair(10.0f, 20.0f, 5.0f, 40.0f);
				next_sale_time = random_time(range_pair.first, range_pair.second);
				printf("Next sale in %.2f seconds.\n", next_sale_time);
			}
			else {
				// If no stock anywhere, try again sooner (or pick whatever policy you want)
				timer = 0.0f;
				next_sale_time = 1.0f; // try again after 1 second
			}
		}
	}
}

//void Economy_Exit();
//
//// commands (change state)

void Economy_AddMoney(int amount) {
	total_money += static_cast<u64>(amount);

	if (total_money > max_money)
		total_money = max_money;

	Economy_SaveToProfile(Profile_GetActiveSlot());
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