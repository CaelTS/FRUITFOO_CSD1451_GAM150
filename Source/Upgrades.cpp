#include "Upgrades.h"
#include "Profile.h"
#include "AEEngine.h"
#include "Utilities.h"
#include "GameStateManager.h"
#include <fstream>
#include "Farm.h"
#include "Economy.h"
#include "Inventory.h"

//placeholder upgrade effects
static void Inventory_UpgradeCapacity(int qty) {
	printf("Inventory capacity increased by %d!\n", qty);
}
static void Economy_UpgradeFruitPrice(f32 multiplier) {
    printf("Fruit price increased by %.2f%%!\n", multiplier * 100);
}
static void Crate_CapacityUpgrade(int qty) {
    printf("Crate capacity increased by %d!\n", qty);
}
static void Farm_UnlockPlot(int plotNumber) {
    printf("Plot #%d unlocked!\n", plotNumber);
}
static void Crate_UnlockCrate(int crateLevel) {
    printf("Crate #%d unlocked!\n", crateLevel);
}



static std::vector<Upgrade> upgrades;

void Upgrades_Init()
{
    
    upgrades.clear();

    upgrades.push_back({
        UPGRADE_INVENTORY_I, // ID
		80, // Cost
		false, // Purchased 
		AEGfxTextureLoad("Assets/Upgrades/Upgrades_Inventory_I.png") // Texture
        });

    upgrades.push_back({
		UPGRADE_FRUIT_I,
		100,
		false,
        AEGfxTextureLoad("Assets/Upgrades/Upgrades_Fruits_I.png")
        });

	upgrades.push_back({
        UPGRADE_UNLOCK_PLOT2,
        250,
        false,
        AEGfxTextureLoad("Assets/Upgrades/Upgrades_Plot Unlock_2.png")
		});

    upgrades.push_back({
        UPGRADE_CRATE_I,
        120,
        false,
        AEGfxTextureLoad("Assets/Upgrades/Upgrades_Crates_I.png")
		});

    upgrades.push_back({
        UPGRADE_INVENTORY_II,
        260,
        false,
		AEGfxTextureLoad("Assets/Upgrades/Upgrades_Inventory_II.png")
        });

    upgrades.push_back({
        UPGRADE_UNLOCK_CRATE2,
        250,
        false,
        AEGfxTextureLoad("Assets/Upgrades/Upgrades_Crate Unlock_2.png")
        });

    upgrades.push_back({
        UPGRADE_FRUIT_II,
        350,
        false,
        AEGfxTextureLoad("Assets/Upgrades/Upgrades_Fruits_II.png")
        });

    upgrades.push_back({
        UPGRADE_CRATE_II,
        380,
        false,
        AEGfxTextureLoad("Assets/Upgrades/Upgrades_Crates_II.png")
        });

    upgrades.push_back({
        UPGRADE_UNLOCK_PLOT3,
        500,
        false,
        AEGfxTextureLoad("Assets/Upgrades/Upgrades_Plot Unlock_3.png")
        });

    upgrades.push_back({
        UPGRADE_INVENTORY_III,
        500,
		false,
        AEGfxTextureLoad("Assets/Upgrades/Upgrades_Inventory_III.png")
		});

    upgrades.push_back({
        UPGRADE_FRUIT_III,
        800,
        false,
        AEGfxTextureLoad("Assets/Upgrades/Upgrades_Fruits_III.png")
        });

    upgrades.push_back({
        UPGRADE_UNLOCK_CRATE3,
        680,
        false,
        AEGfxTextureLoad("Assets/Upgrades/Upgrades_Crate Unlock_3.png")
        });

    upgrades.push_back({
        UPGRADE_CRATE_III,
        800,
        false,
        AEGfxTextureLoad("Assets/Upgrades/Upgrades_Crates_III.png")
        });

    upgrades.push_back({
        UPGRADE_UNLOCK_PLOT4,
        1200,
        false,
        AEGfxTextureLoad("Assets/Upgrades/Upgrades_Plot Unlock_4.png")
		});

    upgrades.push_back({
        UPGRADE_LEVEL_UP,
        7000,
        false,
        AEGfxTextureLoad("Assets/Upgrades/Upgrades_LevelUP.png")
		});
}

std::vector<Upgrade>& Upgrades_GetList()
{
    return upgrades;
}

void Upgrades_Purchase(UpgradeID id) {
    for (Upgrade& u : upgrades) {
        if (u.id == id) {
            if (!u.purchased) {
                u.purchased = true;
                // Apply upgrade effects
                switch (id) {
                case UPGRADE_INVENTORY_I:
                    Inventory_UpgradeCapacity(10);
                    break;

                case UPGRADE_FRUIT_I:
                    Economy_UpgradeFruitPrice(10);
                    break;

                case UPGRADE_UNLOCK_PLOT2:
                    Farm_UnlockPlot(2);
                    break;

                case UPGRADE_CRATE_I:
                    Crate_CapacityUpgrade(10);
                    break;

                case UPGRADE_INVENTORY_II:
                    Inventory_UpgradeCapacity(20);
                    break;

                case UPGRADE_UNLOCK_CRATE2:
                    Crate_UnlockCrate(2);
                    break;

                case UPGRADE_FRUIT_II:
                    Economy_UpgradeFruitPrice(20);
                    break;

                case UPGRADE_CRATE_II:
                    Crate_CapacityUpgrade(20);
                    break;

                case UPGRADE_UNLOCK_PLOT3:
                    Farm_UnlockPlot(3);
                    break;

                case UPGRADE_INVENTORY_III:
                    Inventory_UpgradeCapacity(30);
                    break;

                case UPGRADE_FRUIT_III:
                    Economy_UpgradeFruitPrice(30);
                    break;
                case UPGRADE_UNLOCK_CRATE3:
                    Crate_UnlockCrate(3);
                    break;

                case UPGRADE_CRATE_III:
                    Crate_CapacityUpgrade(30);
                    break;

                case UPGRADE_UNLOCK_PLOT4:
                    Farm_UnlockPlot(4);
                    break;

                case UPGRADE_LEVEL_UP:
                    Inventory_UpgradeCapacity(50);
                    Economy_UpgradeFruitPrice(50);
                    Crate_CapacityUpgrade(50);
                    Farm_UnlockPlot(5);
                    Crate_UnlockCrate(4);
                    break;
                }
                break; // Exit loop after finding the upgrade
            }
        }
    }
}

bool Upgrades_CanPurchase(Upgrade id, int currentMoney) {
	if (currentMoney < id.cost) return false; // Not enough money
    else if (currentMoney >= id.cost) { // Can afford
		id.purchased = true;
		return true;
    }
}

void Upgrades_Unload()
{
    for (auto& u : upgrades)
    {
        if (u.texture)
            AEGfxTextureUnload(u.texture);
    }
}