#include "json_loader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>

#include <boost/json.hpp>

using namespace boost::json;

namespace json_loader {

void AddRoad(model::Map& to_add, boost::json::object& road_obj) {
    // horizontal
    if (road_obj.contains(KEY_X1)) {
        model::Point start{
            static_cast<int>(road_obj[KEY_X0].as_int64()),
            static_cast<int>(road_obj[KEY_Y0].as_int64())
        };
        model::Road road_to_add(
            model::Road::HORIZONTAL,
            start,
            static_cast<int>(road_obj[KEY_X1].as_int64())
        );
        to_add.AddRoad(road_to_add);
        return;
    }
    
    // vertical
    if (road_obj.contains(KEY_Y1)) {
        model::Point start{
            static_cast<int>(road_obj[KEY_X0].as_int64()),
            static_cast<int>(road_obj[KEY_Y0].as_int64())
        };
        model::Road road_to_add(
            model::Road::VERTICAL,
            start,
            static_cast<int>(road_obj[KEY_Y1].as_int64())
        );
        to_add.AddRoad(road_to_add);
        return;
    }
}

void AddBuilding(model::Map& to_add, boost::json::object& building_obj) {
    model::Rectangle rectangle;
    rectangle.position = model::Point{
        static_cast<int>(building_obj[KEY_X].as_int64()),
        static_cast<int>(building_obj[KEY_Y].as_int64())
    };
    rectangle.size = model::Size{
        static_cast<int>(building_obj[KEY_W].as_int64()),
        static_cast<int>(building_obj[KEY_H].as_int64())
    };
    model::Building build(rectangle);
    to_add.AddBuilding(build);
}

void AddOffice(model::Map& to_add, boost::json::object& office_obj) {
    model::Office::Id id{
        model::Office::Id(std::string(office_obj[KEY_ID].as_string()))
    };
    model::Point point{
        static_cast<int>(office_obj[KEY_X].as_int64()),
        static_cast<int>(office_obj[KEY_Y].as_int64())
    };
    model::Offset offset{
        static_cast<int>(office_obj[KEY_OFFSET_X].as_int64()),
        static_cast<int>(office_obj[KEY_OFFSET_Y].as_int64())
    };
    model::Office off(id, point, offset);
    to_add.AddOffice(off);
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    model::Game game;

    std::ifstream in(json_path);
    
    if (!in) {
        throw std::runtime_error("Failed to open file");
    }
    
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string str = buffer.str();
    
    boost::json::value val = boost::json::parse(str);
    boost::json::object json = val.as_object();
    
    if (json.count(KEY_DEFAULT_DOG_SPEED)) {
        double speed = json[KEY_DEFAULT_DOG_SPEED].as_double();
        game.AddSpeed(speed);
    }
    
    if (json.count(KEY_DEFAULT_BAG_CAPACITY)) {
        int capacity = static_cast<int>(json[KEY_DEFAULT_BAG_CAPACITY].as_double());
        game.AddCapacity(capacity);
    }
    
    if (json.count(KEY_DOG_RETIREMENT_TIME)) {
        int retire_time = static_cast<int>(json[KEY_DOG_RETIREMENT_TIME].as_double()*1000);
        game.SetRetireTime(retire_time);
    } else {
        game.SetRetireTime(60000);
    }
    
    for (const auto& map : json[KEY_MAPS].as_array()) {
        boost::json::object map_obj = map.as_object();
        
        model::Map to_add(
            model::Map::Id(std::string(map_obj[KEY_ID].as_string())),
            std::string(map_obj[KEY_NAME].as_string())
        );
            
        if (map_obj.count(KEY_DOG_SPEED)) {
            double speed = map_obj[KEY_DOG_SPEED].as_double();
            to_add.AddSpeed(speed);
        }
        
        if (map_obj.count(KEY_BAG_CAPACITY)) {
            int capacity = static_cast<int>(map_obj[KEY_BAG_CAPACITY].as_double());
            to_add.AddCapacity(capacity);
        }
        
        for (const auto& road : map_obj[KEY_ROADS].as_array()) {
            boost::json::object road_obj = road.as_object();
            AddRoad(to_add, road_obj);
        }
        
        for (const auto& building : map_obj[KEY_BUILDINGS].as_array()) {
            boost::json::object building_obj = building.as_object();
            AddBuilding(to_add, building_obj);
        }
            
        for (const auto& office : map_obj[KEY_OFFICES].as_array()) {
            boost::json::object office_obj = office.as_object();
            AddOffice(to_add, office_obj);
        }
        
        boost::json::array loots = map_obj[KEY_LOOT_TYPES].as_array();
        to_add.SetNumberOfLootOptions(static_cast<int>(loots.size()));
        
        std::unordered_map<std::string, int> name_of_loot_to_number;
        std::unordered_map<int, int> number_of_loot_to_value;
        int counter = 0;
        
        std::string tag = ExtraData::NameHelper({std::string(map_obj[KEY_ID].as_string()), TAG_LOOT_TYPES});
        
        for (const auto& loot : loots) {
            boost::json::object loot_obj = loot.as_object();
            
            ExtraData::Store(tag, boost::json::serialize(loot));
            name_of_loot_to_number[std::string(loot_obj[KEY_LOOT_NAME].as_string())] = counter;
            number_of_loot_to_value[counter] = static_cast<int>(loot_obj[KEY_LOOT_VALUE].as_int64());
            ++counter;
        }
        
        to_add.SetNamesOfLootsToNumbers(name_of_loot_to_number);
        to_add.SetNumberOfLootsToValue(number_of_loot_to_value);
        
        game.AddMap(to_add);
    }
    
    boost::json::object config_obj = json[KEY_LOOT_GENERATOR_CONFIG].as_object();
    
    double base_interval = config_obj[KEY_PERIOD].as_double();
    double probability = config_obj[KEY_PROBABILITY].as_double();
    
    auto generator_shared = std::make_shared<loot_gen::LootGenerator>(
        std::chrono::milliseconds(static_cast<int>(base_interval)),
        probability
    );
        
    game.SetGenerator(generator_shared);
        
    return game;
}

}  // namespace json_loader
