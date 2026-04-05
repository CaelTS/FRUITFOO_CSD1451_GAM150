#include "Upgrades.h"
#include "Profile.h"
#include "AEEngine.h"
#include "Utilities.h"
#include "GameStateManager.h"
#include <fstream>
#include "Farm.h"
#include "Economy.h"
#include "Inventory.h"
#include "Crate.h"




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
                    SetInventoryLimit(GetInventoryLimit() + 10);
                    break;

                case UPGRADE_FRUIT_I:
                    Economy_SetBasePriceApple(7);
                    break;

                case UPGRADE_UNLOCK_PLOT2:
                    Farm_SetPlotUnlocked(1, true);
                    break;

                case UPGRADE_CRATE_I:
                    Crate_SetMaxStock(Crate_GetMaxStock() + 10);
                    break;

                case UPGRADE_INVENTORY_II:
                    SetInventoryLimit(GetInventoryLimit() + 20);
                    break;

                case UPGRADE_UNLOCK_CRATE2:
                    Crate_SetUnlocked(1, true);
                    break;

                case UPGRADE_FRUIT_II:
                    Economy_SetBasePriceApple(9);
                    break;

                case UPGRADE_CRATE_II:
                    Crate_SetMaxStock(Crate_GetMaxStock() + 20);
                    break;

                case UPGRADE_UNLOCK_PLOT3:
                    Farm_SetPlotUnlocked(2, true);
                    break;

                case UPGRADE_INVENTORY_III:
                    SetInventoryLimit(GetInventoryLimit() + 30);
                    break;

                case UPGRADE_FRUIT_III:
                    Economy_SetBasePriceApple(11);
                    break;
                case UPGRADE_UNLOCK_CRATE3:
                    Crate_SetUnlocked(2, true);
                    break;

                case UPGRADE_CRATE_III:
                    Crate_SetMaxStock(Crate_GetMaxStock() + 30);
                    break;

                case UPGRADE_UNLOCK_PLOT4:
                    Farm_SetPlotUnlocked(3, true);
                    break;

                case UPGRADE_LEVEL_UP:
                    // Placeholder for a big upgrade, e.g. unlock all plots, max inventory, and max crate capacity
                    break;
                }
                break; // Exit loop after finding the upgrade
            }
        }
    }
}

bool Upgrades_CanPurchase(const Upgrade& u, int currentMoney) {
    // Can't buy something already purchased
    if (u.purchased) return false;
    // Need enough money
    if (currentMoney >= u.cost) {
        Economy_SpendMoney(u.cost);
    }
    return (currentMoney >= u.cost);
}

void Upgrades_Unload()
{
    for (auto& u : upgrades)
    {
        if (u.texture)
            AEGfxTextureUnload(u.texture);
    }
}