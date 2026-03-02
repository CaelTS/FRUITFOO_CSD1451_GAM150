#pragma once

struct GameProfile
{
    char name[32];
    int gold;
    int energy;
    int inventory[3];  // Apple, Pear, Banana
    bool exists;
    time_t lastPlayed;
};

// ProfileScreen lifecycle functions
void ProfileScreen_Load();
void ProfileScreen_Initialize();
void ProfileScreen_Update();
void ProfileScreen_Render();
void ProfileScreen_Free();
void ProfileScreen_Unload();

// Profile management functions
void Profile_Save(int slot, const GameProfile& profile);
bool Profile_Load(int slot, GameProfile& profile);
void Profile_Delete(int slot);
int Profile_GetCount();
void Profile_Select(int slot);
int Profile_GetSelected();