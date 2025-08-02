#pragma once

#include <filesystem>

#include "model.h"
#include "extra_data.h"

namespace json_loader {

const std::string KEY_DEFAULT_DOG_SPEED = "defaultDogSpeed";
const std::string KEY_DEFAULT_BAG_CAPACITY = "defaultBagCapacity";
const std::string KEY_DOG_RETIREMENT_TIME = "dogRetirementTime";
const std::string KEY_MAPS = "maps";
const std::string KEY_ID = "id";
const std::string KEY_NAME = "name";
const std::string KEY_DOG_SPEED = "dogSpeed";
const std::string KEY_BAG_CAPACITY = "bagCapacity";
const std::string KEY_ROADS = "roads";
const std::string KEY_BUILDINGS = "buildings";
const std::string KEY_OFFICES = "offices";
const std::string KEY_LOOT_TYPES = "lootTypes";
const std::string KEY_LOOT_GENERATOR_CONFIG = "lootGeneratorConfig";
const std::string KEY_PERIOD = "period";
const std::string KEY_PROBABILITY = "probability";
const std::string KEY_X0 = "x0";
const std::string KEY_Y0 = "y0";
const std::string KEY_X1 = "x1";
const std::string KEY_Y1 = "y1";
const std::string KEY_X = "x";
const std::string KEY_Y = "y";
const std::string KEY_W = "w";
const std::string KEY_H = "h";
const std::string KEY_OFFSET_X = "offsetX";
const std::string KEY_OFFSET_Y = "offsetY";
const std::string KEY_LOOT_NAME = "name";
const std::string KEY_LOOT_VALUE = "value";
const std::string TAG_LOOT_TYPES = "lootTypes";

model::Game LoadGame(const std::filesystem::path& json_path);

}
