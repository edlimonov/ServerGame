#include "api_handler.h"

namespace api_handler {

bool isValidHex32(const std::string& str) {
    return str.size() == 32 &&
           std::all_of(str.begin(), str.end(), [](unsigned char c) {
               return std::isxdigit(c);
           });
}

boost::json::object MapToJson(const model::Map& map) {
    boost::json::object obj;
    
    obj[KEY_ID] = *map.GetId();
    obj[KEY_NAME] = map.GetName();
    
    return obj;
}

boost::json::object RoadToJson(const model::Road& road) {
    boost::json::object new_road;
    
    auto start_point = road.GetStart();
    auto end_point = road.GetEnd();
    
    new_road[KEY_X0] = start_point.x;
    new_road[KEY_Y0] = start_point.y;
    
    if (road.IsVertical()){
        new_road[KEY_Y1] = end_point.y;
    } else {
        new_road[KEY_X1] = end_point.x;
    }
    
    return new_road;
}

boost::json::object BuildingToJson(const model::Building& building) {
    boost::json::object new_building;
    
    auto rectangle = building.GetBounds();
    
    new_building[KEY_X] = rectangle.position.x;
    new_building[KEY_Y] = rectangle.position.y;
    
    new_building[KEY_W] = rectangle.size.width;
    new_building[KEY_H] = rectangle.size.height;
    
    return new_building;
}

boost::json::object OfficeToJson(const model::Office& office) {
    boost::json::object new_office;
    
    new_office[KEY_ID] = *office.GetId();
    
    auto pos = office.GetPosition();
    auto offset = office.GetOffset();
    
    new_office[KEY_X] = pos.x;
    new_office[KEY_Y] = pos.y;
    
    new_office[KEY_OFFSET_X] = offset.dx;
    new_office[KEY_OFFSET_Y] = offset.dy;
    
    return new_office;
}

boost::json::object MapFullToJson(const model::Map& map) {
    boost::json::object res;
    
    res[KEY_ID] = *map.GetId();
    res[KEY_NAME] = map.GetName();
    
    boost::json::array roads;
    boost::json::array buildings;
    boost::json::array offices;
    boost::json::array loots;
    
    auto map_roads = map.GetRoads();
    auto map_buildings = map.GetBuildings();
    auto map_offices = map.GetOffices();
    
    for (const auto& road : map_roads) {
        roads.push_back(RoadToJson(road));
    }
    
    for (const auto& building : map_buildings) {
        buildings.push_back(BuildingToJson(building));
    }
    
    for (const auto& office : map_offices) {
        offices.push_back(OfficeToJson(office));
    }
    
    for (const auto& loot : ExtraData::ReadAll(ExtraData::NameHelper({*map.GetId() , KEY_LOOT_TYPES}))) {
        loots.push_back(boost::json::parse(loot));
    }
    
    res[KEY_ROADS] = roads;
    res[KEY_BUILDINGS] = buildings;
    res[KEY_OFFICES] = offices;
    res[KEY_LOOT_TYPES] = loots;
    
    return res;
}

} // namespace api_handler
