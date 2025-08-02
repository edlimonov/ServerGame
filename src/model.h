#pragma once

#include "loot_generator.h"
#include "postgres.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>
#include "tagged.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <optional>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <atomic>

namespace model {

inline constexpr double ROAD_BOUNDARY_OFFSET = 0.4;
inline constexpr double DOG_WIDTH = 0.6;
inline constexpr double LOOT_WIDTH = 0.0;
inline constexpr double OFFICE_WIDTH = 0.5;
inline constexpr double MILLISECONDS_IN_SECOND = 1000.0;

using namespace loot_gen;
using Dimension = int;
using Coord = Dimension;

class Dog;
class Map;
class GameSession;
class Road;
class Loot;

using GameSessionShared = std::shared_ptr<GameSession>;
using DogShared = std::shared_ptr<Dog>;
using MapShared = std::shared_ptr<Map>;
using LootShared = std::shared_ptr<Loot>;
using LootGeneratorShared = std::shared_ptr<LootGenerator>;

using Dogs = std::vector<DogShared>;
using Roads = std::vector<Road>;
using Loots = std::vector<Loot>;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

struct Coords {
    double x = 0;
    double y = 0;
    bool operator==(const Coords& other) const {
        return (x == other.x) && (y == other.y);
    }
};

struct Speed {
    double x = 0;
    double y = 0;
};

enum class Direction {
    NORTH,
    SOUTH,
    WEST,
    EAST
};

class Loot {
public:
    Loot(int type, Coords position) :
    id_(counter_++), type_(type), position_(position) {}
    
    Loot(int id, int type, Coords position) :
    id_(id), type_(type), position_(position) {}
    
    int GetType() {
        return type_;
    }
    
    int GetType() const  {
        return type_;
    }
    
    Coords GetPosition() {
        return position_;
    }
    
    Coords GetPosition() const {
        return position_;
    }
    
    int GetId() {
        return id_;
    }
    
    int GetId() const {
        return id_;
    }
    
private:
    int id_;
    int type_;
    Coords position_;
    
    static std::atomic<int> counter_;
};

class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };

    struct VerticalTag {
        VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

    double GetLen() const noexcept {
        if (IsHorizontal()) {
            return std::fabs(start_.x - end_.x);
        }
        return std::fabs(start_.y - end_.y);
    }
    
    Coords GetRandomCords(double len);
    bool PointIsInside(Coords point);
    Coords GetLastPointOnRoad(Coords start, Coords end);

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }
 
    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }
    
    Offices& GetOffices() {
        return offices_;
    }
    
    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    void AddSpeed(double speed) {
        default_dog_speed_ = speed;
    }
    
    void AddCapacity(int capacity) {
        default_bag_capacity_ = capacity;
    }
    
    double GetSpeed() const noexcept {
        return default_dog_speed_;
    }
    
    double GetCapacity() const noexcept {
        return default_bag_capacity_;
    }
    
    void SetNumberOfLootOptions(int number) {
        number_of_loot_options_ = number;
    }
    
    void SetNamesOfLootsToNumbers(std::unordered_map<std::string, int> name_of_loot_to_number) {
        name_of_loot_to_number_ = name_of_loot_to_number;
    }
    
    void SetNumberOfLootsToValue(std::unordered_map<int, int> number_of_loot_to_value) {
        number_of_loot_to_value_ = number_of_loot_to_value;
    }
    
    int GetNumberOfLoot(const std::string& name) {
        return name_of_loot_to_number_[name];
    }
    
    int GetValueOfLoot(int type) {
        return number_of_loot_to_value_[type];
    }
    
    int GetNumberOfLootOptions() {
        return number_of_loot_options_;
    }
    
    Coords GetRandomCordsOnMap();
    Coords GetFirstCordsOnMap();
    
private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    
    double default_dog_speed_ = 1;
    int default_bag_capacity_ = 3;
    
    int number_of_loot_options_;
    std::unordered_map<std::string, int> name_of_loot_to_number_;
    std::unordered_map<int, int> number_of_loot_to_value_;
};

class Dog {
public:
    
    Dog(std::string name) : id_(counter_++), name_(name) {}
    Dog(int id, std::string name) : id_(id), name_(name) {}
    
    std::string& GetName() {
        return name_;
    }
    
    Speed GetSpeed() {
        return speed_;
    }
    
    Coords GetCords() {
        return cords_;
    }
    
    void SetCords(Coords cords) {
        cords_ = cords;
    }
    
    Direction GetDirection() {
        return direction_;
    }
    
    void SetDirection(Direction direction) {
        direction_ = direction;
    }
    
    void SetSpeed(Speed speed) {
        speed_ = speed;
    }
    
    void SetMapSpeed(double speed) {
        map_speed_ = speed;
    }
    
    void SetBagCapacity(int capacity) {
        bag_capacity_ = capacity;
    }
    
    bool BagIsFull() {
        return bag_.size() == bag_capacity_;
    }
    
    void TakeLoot(Loot loot) {
        bag_.push_back(loot);
    }
    
    void UnloadBag(MapShared map);
    
    Loots GetBag() {
        return bag_;
    }
    
    int GetScore() {
        return score_;
    }
    
    void SetScore(int score) {
        score_ = score;
    }
    
    double GetMapSpeed() {
        return map_speed_;
    }
    
    int GetCapacity() {
        return bag_capacity_;
    }
    
    int GetId() {
        return id_;
    }
    
    void SetBag(Loots new_bag) {
        bag_ = new_bag;
    }
    
    void SetDirection(const std::string& dir);
    Coords GetNewCords(int tick);
    Coords FindRouteRec(Coords start, Coords end, Roads& roads);
    
    // изменение координат с течением времени
    void UpdateTickState(int tick, MapShared map);
    
    void SetFullTime(int full_time) {
        full_time_ = full_time;
    }

    void SetRetireTime(int retire_time) {
        retire_time_ = retire_time;
    }

    int GetFullTime() {
        return full_time_;
    }

    int GetRetireTime() {
        return retire_time_;
    }
    
    bool IsGoingToRetire(int retire_time) {
        return retire_time_ >= retire_time;
    }
    
private:
    int id_;
    std::string name_;
    Coords cords_;
    Speed speed_;
    Direction direction_ = Direction::NORTH;
    
    double map_speed_ = 1;
    int bag_capacity_ = 3;
    int score_ = 0;
    Loots bag_;
    
    int full_time_ = 0;
    int retire_time_ = 0;
    
    static std::atomic<int> counter_;
};

class GameSession {
public:
    
    GameSession(MapShared map) : id_(counter_++), map_(map) {}
    GameSession(int id, MapShared map) : id_(id), map_(map) {}
    
    Map::Id GetMapId() const noexcept {
        return map_->GetId();
    }
    
    void AddDog(DogShared dog) {
        dogs_.push_back(dog);
    }
    
    Dogs& GetDogs() noexcept {
        return dogs_;
    }
    
    const Loots& GetLoots() {
        return loots_;
    }
    
    int GetNumberOfLoot(const std::string& name) {
        return map_->GetNumberOfLoot(name);
    }
    
    void UpdateTickState(int tick, LootGeneratorShared loot_generator, PoolShared pool, int retire_trashhold);
    
    int GetId() {
        return id_;
    }
    
    void AddDogs(Dogs dogs) {
        dogs_ = dogs;
    }
    
    void AddLoots(Loots loots) {
        loots_ = loots;
    }
    
private:
    int id_;
    MapShared map_;
    Dogs dogs_;
    Loots loots_;
    
    static std::atomic<int> counter_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);
    
    void AddSpeed(double speed) {
        default_dog_speed_ = speed;
    }

    void AddCapacity(int capacity) {
        default_bag_capacity_ = capacity;
    }
    
    double GetSpeed() const noexcept {
        return default_dog_speed_;
    }
    
    const Maps& GetMaps() const noexcept {
        return maps_;
    }
    
    void UpdateTickState(int tick) {
        
        for (auto session : sessions_) {
            session->UpdateTickState(tick, loot_generator_, connection_pool_, retire_time_);
        }
        
        if (need_to_save_manually) {
            SaveState();
        }
    }
    
    void UpdateTickState(std::chrono::milliseconds delta) {
        UpdateTickState(static_cast<int>(delta.count()));
    }
    
    void SetRandomizeSpawnPoint(bool rand) {
        randomize_spawn_point_ = rand;
    }
    
    void SetTickPeriod(std::optional<int> tick_period) {
        tick_period_ = tick_period;
    }
    
    bool IsInTestState() {
        if (tick_period_) {
            return false;
        }
        return true;
    }
    
    void SetGenerator(LootGeneratorShared generator) {
        loot_generator_ = generator;
    }
    
    void LoadState();
    void SaveState();
    
    void SetSaveFile(std::string file) {
        save_file_ = file;
    }
    
    void SetManualSerialization(bool need_to_save) {
        need_to_save_manually = need_to_save;
    }
    
    MapShared FindMap(Map::Id id) const {
        return std::make_shared<Map>(maps_[map_id_to_index_.at(id)]);
    }
    
    void SetPool(PoolShared pool) {
        connection_pool_ = pool;
    }
    
    PoolShared GetPool() {
        return connection_pool_;
    }
    
    void SetRetireTime(int retire_time) {
        retire_time_ = retire_time;
    }
    
    int GetRetireTime() {
        return retire_time_;
    }
    
    GameSessionShared AddDogToSession(DogShared dog, const std::string map);
    
private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    
    std::vector<GameSessionShared> sessions_;
    
    double default_dog_speed_ = 1;
    int default_bag_capacity_ = 3;
    
    bool randomize_spawn_point_;
    std::optional<int> tick_period_;
    
    LootGeneratorShared loot_generator_;
    
    std::string save_file_;
    bool need_to_save_manually;
    
    int retire_time_;
    
    PoolShared connection_pool_;
};

}  // namespace model
