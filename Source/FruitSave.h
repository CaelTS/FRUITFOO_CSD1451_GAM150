#ifndef FRUIT_SAVE_H
#define FRUIT_SAVE_H

#include <string>
#include <vector>
#include <ctime>
#include "../../rapidjson/document.h"

struct Fruit {
    std::string id;
    std::string fruitType;
    std::time_t plantedAt;
    std::time_t readyAt;
    bool harvested = false;
};

struct Profile {
    std::string playerId;
    std::string playerName;
};

class FruitSaveManager {

};



#endif