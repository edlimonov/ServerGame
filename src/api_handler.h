#pragma once

#include "application.h"
#include "common_handler.h"
#include <string>
#include <boost/json.hpp>
#include <boost/beast/http.hpp>
#include <boost/url.hpp>

#include "extra_data.h"

namespace api_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
namespace urls = boost::urls;

boost::json::object MapToJson(const model::Map& map);
boost::json::object MapFullToJson(const model::Map& map);

bool isValidHex32(const std::string& str);

const std::string REQUEST_MAPS("/api/v1/maps");
const std::string REQUEST_STARTS_WITH_MAPS("/api/v1/maps/");
const std::string REQUEST_JOIN("/api/v1/game/join");
const std::string REQUEST_PLAYERS("/api/v1/game/players");
const std::string REQUEST_STATE("/api/v1/game/state");
const std::string REQUEST_ACTION("/api/v1/game/player/action");
const std::string REQUEST_TICK("/api/v1/game/tick");

const std::string KEY_ID = "id";
const std::string KEY_NAME = "name";

const std::string KEY_ROADS = "roads";
const std::string KEY_BUILDINGS = "buildings";
const std::string KEY_OFFICES = "offices";
const std::string KEY_LOOT_TYPES = "lootTypes";

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

template <typename Request>
StringResponse ProcessApi(Request&& req, model::Game& game, std::string url, std::chrono::time_point<std::chrono::high_resolution_clock> start_time) {
    
    const auto error_response = [&req](http::status status, std::string code, std::string message, std::string_view content_type, bool cache = false) {
        
        boost::json::object res;
        
        res["code"] = code;
        res["message"] = message;
        
        std::string text = boost::json::serialize(res);
        
        return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type, cache);
    };
    
    const auto text_response = [&req](http::status status, std::string_view text, std::string_view content_type, bool cache = false) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive(), content_type, cache);
    };
    
    if (url == REQUEST_MAPS) {
        auto maps = game.GetMaps();
        boost::json::array res;
        
        for (const auto& map : maps) {
            res.push_back(MapToJson(map));
        }
        
        std::string body = boost::json::serialize(res);
        
        return text_response(http::status::ok, body, ContentType::APPLICATION_JSON, true);
    }
    
    if (url.starts_with(REQUEST_STARTS_WITH_MAPS)) {
        
        // некорректный метод
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            auto response = error_response(http::status::method_not_allowed, "invalidMethod", "Invalid method", ContentType::APPLICATION_JSON, true);
            response.set(http::field::allow, "GET, HEAD");
            return response;
        }
        
        url = url.substr(13);
        
        auto maps = game.GetMaps();
        
        auto It = std::find_if(maps.begin(), maps.end(), [url](const auto& map){
            return *map.GetId() == url;
        });
        
        if (It == maps.end()) {
            return error_response(http::status::not_found, "mapNotFound", "Map not found", ContentType::APPLICATION_JSON, true);
        }
        
        auto map = *It;
        boost::json::object res = MapFullToJson(map);
        
        std::string body = boost::json::serialize(res);
        
        return text_response(http::status::ok, body, ContentType::APPLICATION_JSON, true);
    }
    
    // вход в игру
    if (url == REQUEST_JOIN) {
        
        if (req.method() != http::verb::post) {
            auto response = error_response(http::status::method_not_allowed, "invalidMethod", "Only POST method is expected", ContentType::APPLICATION_JSON, true);
            
            response.set(http::field::allow, "POST");
            return response;
        }
        
        boost::json::value value;
        
        try {
            value = boost::json::parse(req.body());
        } catch (const std::exception& e) {
            // не парсится json
            return error_response(http::status::bad_request, "invalidArgument", "Join game request parse error", ContentType::APPLICATION_JSON, true);
        }
        
        // не валидный
        if (!value.is_object()) {
            return error_response(http::status::bad_request, "invalidArgument", "Join game request parse error", ContentType::APPLICATION_JSON, true);
        }
        
        boost::json::object obj = value.as_object();
        
        // нет нужных свойств
        if (!obj.contains("userName") || !obj.contains("mapId")) {
            return error_response(http::status::bad_request, "invalidArgument", "Join game request parse error", ContentType::APPLICATION_JSON, true);
        }
        
        // пустое имя
        if (obj["userName"].as_string().empty()) {
            return error_response(http::status::bad_request, "invalidArgument", "Invalid name", ContentType::APPLICATION_JSON, true);
        }
        
        auto user_name = std::string(obj["userName"].as_string());
        auto map_id = std::string(obj["mapId"].as_string());
        
        auto new_player = Players::AddPlayer(game, user_name, map_id);
        
        // нет такой карты
        if (!new_player) {
            return error_response(http::status::not_found, "mapNotFound", "Map not found", ContentType::APPLICATION_JSON, true);
        }
        
        // формирование ответа
        boost::json::object object_response;
        object_response["authToken"] = *(new_player->GetToken());
        object_response["playerId"] = new_player->Id();
        
        std::string body = boost::json::serialize(object_response);
        
        return text_response(http::status::ok, body, ContentType::APPLICATION_JSON, true);
    }
    
    // список игроков в сессии с данным пользователем
    if (url == REQUEST_PLAYERS) {
        
        // некорректный метод
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            auto response = error_response(http::status::method_not_allowed, "invalidMethod", "Invalid method", ContentType::APPLICATION_JSON, true);
            response.set(http::field::allow, "GET, HEAD");
            return response;
        }
        
        // нет нужного заголовка
        if (!req.count(http::field::authorization)) {
            return error_response(http::status::unauthorized, "invalidToken", "Authorization header is missing", ContentType::APPLICATION_JSON, true);
        }
        
        const auto& auth_header = req[http::field::authorization];
        
        const std::string bearer_prefix = "Bearer ";
        
        // заголовок некорректный
        if (auth_header.size() <= bearer_prefix.size() ||
            !auth_header.starts_with(bearer_prefix)) {
            return error_response(http::status::unauthorized, "invalidToken", "Authorization header is missing", ContentType::APPLICATION_JSON, true);
        }
        
        std::string token(auth_header.substr(bearer_prefix.size()));
        
        // некорректный токен
        if (!isValidHex32(token)) {
            return error_response(http::status::unauthorized, "invalidToken", "Authorization header is missing", ContentType::APPLICATION_JSON, true);
        }
        
        auto player = Players::FindPlayerByToken(token);
        
        // пользователя не существует
        if (!player) {
            return error_response(http::status::unauthorized, "unknownToken", "Player token has not been found", ContentType::APPLICATION_JSON, true);
        }
        
        // обработка сессии пользователя
        auto dogs = player->GetSessionDogs();
        boost::json::object object_response;
        
        int i = 0;
        for (const auto& dog : dogs) {
            object_response[std::to_string(i)] = dog->GetName();
            i++;
        }
        
        std::string body = boost::json::serialize(object_response);
        
        return text_response(http::status::ok, body, ContentType::APPLICATION_JSON, true);
    }
    
    if (url == REQUEST_STATE) {
        
        // некорректный метод
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            auto response = error_response(http::status::method_not_allowed, "invalidMethod", "Invalid method", ContentType::APPLICATION_JSON, true);
            response.set(http::field::allow, "GET, HEAD");
            return response;
        }
        
        // нет нужного заголовка
        if (!req.count(http::field::authorization)) {
            return error_response(http::status::unauthorized, "invalidToken", "Authorization header is missing", ContentType::APPLICATION_JSON, true);
        }
        
        const auto& auth_header = req[http::field::authorization];
        
        const std::string bearer_prefix = "Bearer ";
        
        // заголовок некорректный
        if (auth_header.size() <= bearer_prefix.size() ||
            !auth_header.starts_with(bearer_prefix)) {
            return error_response(http::status::unauthorized, "invalidToken", "Authorization header is missing", ContentType::APPLICATION_JSON, true);
        }
        
        std::string token(auth_header.substr(bearer_prefix.size()));
        
        // некорректный токен
        if (!isValidHex32(token)) {
            return error_response(http::status::unauthorized, "invalidToken", "Authorization header is missing", ContentType::APPLICATION_JSON, true);
        }
        
        auto player = Players::FindPlayerByToken(token);
        
        // пользователя не существует
        if (!player) {
            return error_response(http::status::unauthorized, "unknownToken", "Player token has not been found", ContentType::APPLICATION_JSON, true);
        }
        
        // обработка сессии пользователя
        auto dogs = player->GetSessionDogs();
        boost::json::object object_response;
        boost::json::object object_players;
        
        for (const auto& dog : dogs) {
            boost::json::object object_player;
            
            auto speed = dog->GetSpeed();
            auto cords = dog->GetCords();
            auto direction = dog->GetDirection();
            
            object_player["pos"] = boost::json::array{cords.x, cords.y};
            object_player["speed"] = boost::json::array{speed.x, speed.y};
            
            if (direction == model::Direction::NORTH) {
                object_player["dir"] = "U";
            } else if (direction == model::Direction::SOUTH) {
                object_player["dir"] = "D";
            } else if (direction == model::Direction::EAST) {
                object_player["dir"] = "R";
            } else if (direction == model::Direction::WEST) {
                object_player["dir"] = "L";
            }
            
            boost::json::array bag;
            
            for (auto loot_in_bag : dog->GetBag()) {
                boost::json::object loot_in_bag_object;
                loot_in_bag_object["id"] = loot_in_bag.GetId();
                loot_in_bag_object["type"] = loot_in_bag.GetType();
                
                bag.push_back(loot_in_bag_object);
            }
            
            object_player["bag"] = bag;
            object_player["score"] = dog->GetScore();
            
            object_players[std::to_string(dog->GetId())] = object_player;
        }
        
        object_response["players"] = object_players;
        
        boost::json::object lost_objects;
        
        auto session = player->GetSession();
        auto loots = session->GetLoots();
        
        for (int i = 0; i < loots.size(); i++) {
            boost::json::object loot_object;
            
            loot_object["type"] = loots[i].GetType();
            
            boost::json::array position_arr;
            auto pos = loots[i].GetPosition();
            
            position_arr.push_back(pos.x);
            position_arr.push_back(pos.y);
            
            loot_object["pos"] = position_arr;
            
            lost_objects[std::to_string(loots[i].GetId())] = loot_object;
        }
        
        object_response["lostObjects"] = lost_objects;
        
        std::string body = boost::json::serialize(object_response);
        
        return text_response(http::status::ok, body, ContentType::APPLICATION_JSON, true);
        
    }
    
    if (url == REQUEST_ACTION) {
        
        // некорректный метод
        if (req.method() != http::verb::post) {
            auto response = error_response(http::status::method_not_allowed, "invalidMethod", "Invalid method", ContentType::APPLICATION_JSON, true);
            response.set(http::field::allow, "POST");
            return response;
        }
        
        // нет нужного заголовка content_type
        if (!req.count(http::field::content_type)) {
            return error_response(http::status::bad_request, "invalidArgument", "Invalid content type", ContentType::APPLICATION_JSON, true);
        }
        
        auto it = req.find(http::field::content_type);
        std::string content_type = std::string(it->value());
        
        if (content_type != "application/json") {
            return error_response(http::status::bad_request, "invalidArgument", "Invalid content type", ContentType::APPLICATION_JSON, true);
        }
        
        // нет нужного заголовка
        if (!req.count(http::field::authorization)) {
            return error_response(http::status::unauthorized, "invalidToken", "Authorization header is missing", ContentType::APPLICATION_JSON, true);
        }
        
        // проверка JSON
        boost::json::value value;
        
        try {
            value = boost::json::parse(req.body());
        } catch (const std::exception& e) {
            // не парсится json
            return error_response(http::status::bad_request, "invalidArgument", "Failed to parse action", ContentType::APPLICATION_JSON, true);
        }
        
        // не валидный
        if (!value.is_object()) {
            return error_response(http::status::bad_request, "invalidArgument", "Failed to parse action", ContentType::APPLICATION_JSON, true);
        }
        
        boost::json::object obj = value.as_object();
        
        // нет нужных свойств
        if (!obj.contains("move")) {
            return error_response(http::status::bad_request, "invalidArgument", "Failed to parse action", ContentType::APPLICATION_JSON, true);
        }
        
        std::string dir = std::string(obj["move"].as_string());
        
        // пустое имя
        if (!dir.empty() && dir != "L" && dir != "R" && dir != "U" && dir != "D") {
            return error_response(http::status::bad_request, "invalidArgument", "Failed to parse action", ContentType::APPLICATION_JSON, true);
        }
        
        const auto& auth_header = req[http::field::authorization];
        
        const std::string bearer_prefix = "Bearer ";
        
        // заголовок некорректный
        if (auth_header.size() <= bearer_prefix.size() ||
            !auth_header.starts_with(bearer_prefix)) {
            return error_response(http::status::unauthorized, "invalidToken", "Authorization header is missing", ContentType::APPLICATION_JSON, true);
        }
        
        std::string token(auth_header.substr(bearer_prefix.size()));
        
        // некорректный токен
        if (!isValidHex32(token)) {
            return error_response(http::status::unauthorized, "invalidToken", "Authorization header is missing", ContentType::APPLICATION_JSON, true);
        }
        
        auto player = Players::FindPlayerByToken(token);
        
        // пользователя не существует
        if (!player) {
            return error_response(http::status::unauthorized, "unknownToken", "Player token has not been found", ContentType::APPLICATION_JSON, true);
        }
        
        // обработка сессии пользователя
        auto dog = Players::FindDogByToken(token);
        dog->SetDirection(dir);
        
        boost::json::object object_response;
        std::string body = boost::json::serialize(object_response);
        
        return text_response(http::status::ok, body, ContentType::APPLICATION_JSON, true);
    }
    
    if (url == REQUEST_TICK) {
        
        // некорректный метод
        if (req.method() != http::verb::post) {
            auto response = error_response(http::status::method_not_allowed, "invalidMethod", "Invalid method", ContentType::APPLICATION_JSON, true);
            response.set(http::field::allow, "POST");
            return response;
        }
        
        // нет нужного заголовка content_type
        if (!req.count(http::field::content_type)) {
            return error_response(http::status::bad_request, "invalidArgument", "Invalid content type", ContentType::APPLICATION_JSON, true);
        }
        
        auto it = req.find(http::field::content_type);
        std::string content_type = std::string(it->value());
        
        if (content_type != "application/json") {
            return error_response(http::status::bad_request, "invalidArgument", "Invalid content type", ContentType::APPLICATION_JSON, true);
        }
        
        // проверка JSON
        boost::json::value value;
        value = boost::json::parse(req.body());
        
        // не валидный
        if (!value.is_object()) {
            return error_response(http::status::bad_request, "invalidArgument", "Failed to parse tick request JSON", ContentType::APPLICATION_JSON, true);
        }
        
        boost::json::object obj = value.as_object();
        
        // нет нужных свойств
        if (!obj.contains("timeDelta")) {
            return error_response(http::status::bad_request, "invalidArgument", "Failed to parse tick request JSON", ContentType::APPLICATION_JSON, true);
        }
        
        // нет нужных свойств
        if (obj["timeDelta"].kind() != boost::json::kind::int64) {
            return error_response(http::status::bad_request, "invalidArgument", "Failed to parse tick request JSON", ContentType::APPLICATION_JSON, true);
        }
        
        if (!game.IsInTestState()) {
            return error_response(http::status::bad_request, "invalidArgument", "Invalid endpoint", ContentType::APPLICATION_JSON, true);
        }
        
        int tick = static_cast<int>(obj["timeDelta"].get_int64());
        
        // логика запроса
        try {
            game.UpdateTickState(tick);
        } catch (const std::exception& e) {
            return error_response(http::status::bad_request, "invalidArgument", "Invalid endpoint", ContentType::APPLICATION_JSON, true);
        }
        
        boost::json::object object_response;
        std::string body = boost::json::serialize(object_response);
        
        return text_response(http::status::ok, body, ContentType::APPLICATION_JSON, true);
    }
    
    if (url.starts_with("/api/v1/game/records")) {
        
        // некорректный метод
        if (req.method() != http::verb::get) {
            auto response = error_response(http::status::method_not_allowed, "invalidMethod", "Invalid method", ContentType::APPLICATION_JSON, true);
            response.set(http::field::allow, "GET");
            return response;
        }
        
        // использую boost.url для парсинга строки
        urls::url_view url_view;
        
        try {
            url_view = urls::parse_uri_reference(url).value();
        } catch (const std::exception& e) {
            return error_response(http::status::bad_request, "invalidArgument", "Failed to parse tick request JSON", ContentType::APPLICATION_JSON, true);
        }
        
        // получаю query-параметры
        auto params = url_view.params();
        
        // ищу нужные параметры
        int start = 0;
        int maxItems = 100;

        for (auto param : params) {
            auto key = param.key;
            auto value = param.value;

            if (key == "start") {
                start = std::stoi(std::string(value));
            } else if (key == "maxItems") {
                maxItems = std::stoi(std::string(value));
            }
        }

        if (maxItems > 100) {
            return error_response(http::status::bad_request, "badRequest", "Too many items requested", ContentType::APPLICATION_JSON);
        }
        
        std::string body;
        
        auto records = postgres::DB::GetPlayersInfo(game.GetPool(), start, maxItems);
        boost::json::array res;
        
        for (const auto& record : records) {
            boost::json::object record_obj;
            
            record_obj[std::string("name")] = record.name;
            record_obj[std::string("score")] = record.total_score;
            record_obj[std::string("playTime")] =
            static_cast<double>(record.full_game_period) / 1000;
            
            res.push_back(record_obj);
        }
        
        body = boost::json::serialize(res);
        
        return text_response(http::status::ok, body, ContentType::APPLICATION_JSON, true);
    }
    
    return error_response(http::status::bad_request, "badRequest", "Bad request", ContentType::APPLICATION_JSON);
}

} // namespace api_handler
