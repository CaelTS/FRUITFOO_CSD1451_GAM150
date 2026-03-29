#pragma once
#ifndef UPGRADES_H
#define UPGRADES_H

#include "AEEngine.h"
#include <vector>

enum UpgradeID
{
    UPGRADE_INVENTORY_I,
    UPGRADE_FRUIT_I,
    UPGRADE_UNLOCK_PLOT2,
    UPGRADE_CRATE_I,
    UPGRADE_INVENTORY_II,
    UPGRADE_UNLOCK_CRATE2,
    UPGRADE_FRUIT_II,
    UPGRADE_CRATE_II,
    UPGRADE_UNLOCK_PLOT3,
    UPGRADE_INVENTORY_III,
    UPGRADE_FRUIT_III,
    UPGRADE_UNLOCK_CRATE3,
    UPGRADE_CRATE_III,
    UPGRADE_UNLOCK_PLOT4,
    UPGRADE_LEVEL_UP
};

struct Upgrade
{
    UpgradeID id;
    int cost;
    bool purchased;

	AEGfxTexture* texture; 
};

void Upgrades_Init();
void Upgrades_Unload();
void Upgrades_Purchase(UpgradeID id);

// Use const reference, don't mutate inside the check
bool Upgrades_CanPurchase(const Upgrade& u, int currentMoney);
std::vector<Upgrade>& Upgrades_GetList();

#endif // UPGRADES_H