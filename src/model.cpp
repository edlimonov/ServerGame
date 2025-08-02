#include "model.h"
#include "application.h"
#include <stdexcept>

#include "collision_detector.h"
#include "model_serialization.h"

namespace model {
using namespace std::literals;

std::atomic<int> Loot::counter_{0};
std::atomic<int> Dog::counter_{0};
std::atomic<int> GameSession::counter_{0};

Coords Road::GetRandomCords(double len) {
    
    len = std::min(len, GetLen());
    
    if (IsHorizontal()) {
        if (start_.x < end_.x) {
            return Coords{start_.x + len, static_cast<double>(start_.y)};
        }
        return Coords{end_.x + len, static_cast<double>(start_.y)};
    }
    
    if (start_.y < end_.y) {
        return Coords{static_cast<double>(start_.x), start_.y + len};
    }
    return Coords{static_cast<double>(start_.x), end_.y + len};
}

bool Road::PointIsInside(Coords point) {
    if (IsHorizontal()) {
        
        double up = start_.y - ROAD_BOUNDARY_OFFSET;
        double down = start_.y + ROAD_BOUNDARY_OFFSET;

        double right = std::max(start_.x, end_.x) + ROAD_BOUNDARY_OFFSET;
        double left = std::min(start_.x, end_.x) - ROAD_BOUNDARY_OFFSET;
        
        return point.x <= right && point.x >= left && point.y >= up && point.y <= down;
    }
    
    double up = std::min(start_.y, end_.y);
    double down = std::max(start_.y, end_.y);
    
    double right = start_.x + ROAD_BOUNDARY_OFFSET;
    double left = start_.x - ROAD_BOUNDARY_OFFSET;
    
    return point.x <= right && point.x >= left && point.y >= up && point.y <= down;
}

Coords Road::GetLastPointOnRoad(Coords start, Coords end) {
    if (start == end) return end;
    
    double up;
    double down;
    double right;
    double left;
    
    if (IsHorizontal()) {
        up = start_.y - ROAD_BOUNDARY_OFFSET;
        down = start_.y + ROAD_BOUNDARY_OFFSET;
        right = std::max(start_.x, end_.x) + ROAD_BOUNDARY_OFFSET;
        left = std::min(start_.x, end_.x) - ROAD_BOUNDARY_OFFSET;
    }
    if (IsVertical()) {
        up = std::min(start_.y, end_.y);
        down = std::max(start_.y, end_.y);
        right = start_.x + ROAD_BOUNDARY_OFFSET;
        left = start_.x - ROAD_BOUNDARY_OFFSET;
    }
    
    // вертикальный путь
    if (start.x == end.x) {
        // вниз
        if (start.y < end.y) {
            return {start.x, std::min(end.y, down)};
        }
        // вверх
        return {start.x, std::max(end.y, up)};
    }
    
    // вправо
    if (start.x < end.x) {
        return {std::min(right, end.x), start.y};
    }
    // влево
    return {std::max(left, end.x), start.y};
}

Coords Map::GetRandomCordsOnMap() {
    int num = static_cast<int>(roads_.size());
    
    std::random_device rd; // Для получения настоящего случайного числа из устройства
    std::mt19937 gen(rd()); // Стандартный механизм генерации случайных чисел
    std::uniform_int_distribution<> distrib_1(0, num - 1); // Определяем диапазон

    int i = distrib_1(gen);
    
    auto road = roads_[i];
    double len = road.GetLen();
    
    std::uniform_real_distribution<> distrib_2(0.0, len);

    double j = distrib_2(gen);
    
    return road.GetRandomCords(j);
}

Coords Map::GetFirstCordsOnMap() {
    auto road = roads_[0];
    Point start = road.GetStart();
    return {static_cast<double>(start.x), static_cast<double>(start.y)};
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (const std::exception& e) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Dog::SetDirection(const std::string& dir) {
    if (dir == "L") {
        direction_ = Direction::WEST;
        speed_ = {- map_speed_, 0};
    } else if (dir == "R") {
        direction_ = Direction::EAST;
        speed_ = {map_speed_, 0};
    } else if (dir == "U") {
        direction_ = Direction::NORTH;
        speed_ = {0, - map_speed_};
    } else if (dir == "D") {
        direction_ = Direction::SOUTH;
        speed_ = {0, map_speed_};
    } else if (dir.empty()) {
        speed_ = {0, 0};
    }
}

Coords Dog::GetNewCords(int tick) {
    
    double tick_double = static_cast<double>(tick) / MILLISECONDS_IN_SECOND;
    
    return {cords_.x + speed_.x * tick_double, cords_.y + speed_.y * tick_double};
}

Coords Dog::FindRouteRec(Coords start, Coords end, Roads& roads) {
    Coords last_point_on_this_road = start; // Инициализируем, чтобы вернуть start, если ничего не найдено
    bool found_route = false;

    for (size_t i = 0; i < roads.size(); ++i) {
        auto& road = roads[i]; // Получаем ссылку на текущую дорогу

        if (road.PointIsInside(start)) {
            last_point_on_this_road = road.GetLastPointOnRoad(start, end);

            if (last_point_on_this_road == end) {
                return end; // Базовый случай: нашли конечную точку
            }

            // Создаем новый вектор roads, исключая текущую дорогу
            Roads next_roads;
            for (size_t j = 0; j < roads.size(); ++j) {
                if (i != j) {
                    next_roads.push_back(roads[j]);
                }
            }

            // Рекурсивный вызов с уменьшенным набором дорог
            Coords recursive_result = FindRouteRec(last_point_on_this_road, end, next_roads);

            if (recursive_result != start) {
                // Если рекурсивный вызов нашел путь
                std::cout << "WE ARE HERE" << std::endl;
                return recursive_result;
            }
        }
    }

    // Если не нашли путь ни по одной дороге, возвращаем start
    return start;
}

void Dog::UpdateTickState(int tick, MapShared map) {
    
    full_time_ += tick;
    
    if (speed_.x == 0 && speed_.y == 0) {
        retire_time_ += tick;
    } else {
        retire_time_ = 0;
    }
    
    // новые координаты точки
    Coords new_cords = GetNewCords(tick);
    
    // если начальные и конечные точки внутри одной дороги - можно двигаться до конечной
    auto roads = map->GetRoads();
    for (auto road : roads) {
        if (road.PointIsInside(cords_) && road.PointIsInside(new_cords)) {
            cords_ = new_cords;
            return;
        }
    }

    cords_ = FindRouteRec(cords_, new_cords, roads);
    
    if (cords_ != new_cords) {
        speed_ = {0, 0};
    }
}

void Dog::UnloadBag(MapShared map) {
    
    for (auto loot : bag_) {
        score_ += map->GetValueOfLoot(loot.GetType());
    }
    
    bag_.clear();
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (const std::exception& e) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

GameSessionShared Game::AddDogToSession(DogShared dog, const std::string map) {
    
    auto map_id = Map::Id(map);
    
    // проверка, есть ли вообще нужная карта
    if (!map_id_to_index_.count(map_id)) {
        return nullptr;
    }
    
    auto map_in_which_add = maps_[map_id_to_index_[map_id]];
    
    // добавление скорости
    if (map_in_which_add.GetSpeed() != 1) {
        dog->SetMapSpeed(map_in_which_add.GetSpeed());
    } else if (default_dog_speed_ != 1) {
        dog->SetMapSpeed(default_dog_speed_);
    }
    
    // добавление вместимости
    if (map_in_which_add.GetCapacity() != 3) {
        dog->SetBagCapacity(map_in_which_add.GetCapacity());
    } else if (default_bag_capacity_ != 3) {
        dog->SetBagCapacity(default_bag_capacity_);
    }
    
    // добавление координат
    if (randomize_spawn_point_) {
        dog->SetCords(map_in_which_add.GetRandomCordsOnMap());
    } else {
        dog->SetCords(map_in_which_add.GetFirstCordsOnMap());
    }
    
    // проверка, есть ли сессия с нужной картой
    auto it = std::find_if(sessions_.begin(), sessions_.end(),
                          [map_id](GameSessionShared session) {
        return session->GetMapId() == map_id;
    });
    
    // если сессия уже есть
    if (it != sessions_.end()) {
        (*it)->AddDog(dog);
        return *it;
    }
    
    GameSessionShared new_session(new GameSession(
                                                  std::make_shared<Map>(maps_[map_id_to_index_[map_id]])
                                                  ));
    
    new_session->AddDog(dog);
    sessions_.push_back(new_session);
    
    return sessions_.back();
}

void GameSession::UpdateTickState(int tick, LootGeneratorShared loot_generator, PoolShared pool, int retire_trashhold) {
    using namespace collision_detector;
    
    ItemGatherer item_gatherer;
    
    std::vector<DogShared> dogs_in_concrete_order;
    
    for (auto dog : dogs_) {
        
        Coords previous_coords = dog->GetCords();
        dog->UpdateTickState(tick, map_);
        Coords actual_coords = dog->GetCords();
        
        item_gatherer.AddGatherer(Gatherer{
            {actual_coords.x, actual_coords.y},
            {previous_coords.x, previous_coords.y},
            DOG_WIDTH
        });
        
        dogs_in_concrete_order.push_back(dog);
    }
    
    for (auto loot : loots_) {
        item_gatherer.AddItem(Item{
            {loot.GetPosition().x, loot.GetPosition().y},
            LOOT_WIDTH
        });
    }
    
    for (auto& office : map_->GetOffices()) {
        item_gatherer.AddItem(Item{
            {static_cast<double>(office.GetPosition().x), static_cast<double>(office.GetPosition().y)},
            OFFICE_WIDTH
        });
    }
    
    auto gathering_events = FindGatherEvents(item_gatherer);
    
    for (auto& event : gathering_events) {
        
        auto dog = dogs_in_concrete_order[event.gatherer_id];

        // собака подобрала предмет
        if (event.item_id < loots_.size() && !dog->BagIsFull()) {
            
            dog->TakeLoot(loots_[event.item_id]);
            loots_.erase(loots_.begin() + event.item_id);
            
        } else {
            dog->UnloadBag(map_);
        }
    }
    
    // количество новых предметов
    auto number_of_loots = loot_generator->Generate(
        std::chrono::milliseconds(tick),
        static_cast<unsigned>(loots_.size()),
        static_cast<unsigned>(dogs_.size())
    );
    
    // генератор
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distrib(0, map_->GetNumberOfLootOptions() - 1);
    
    // генерирование новых предметов
    while (number_of_loots > 0) {
        loots_.emplace_back(distrib(gen), map_->GetRandomCordsOnMap());
        // увеличивается счетчик всех предметов, которые были/есть на карте
        --number_of_loots;
    }
    
    // отправка собачек на покой
    for (auto it = dogs_.begin(); it != dogs_.end();) {
        auto dog = *it;
        
        if (dog->IsGoingToRetire(retire_trashhold)) {
            
            postgres::DB::SaveRecord(pool, {
                dog->GetName(),
                dog->GetScore(),
                dog->GetFullTime(),
            });
            
            Players::RemovePlayerByDogId(dog->GetId());
            
            it = dogs_.erase(it);
        } else {
            ++it;
        }
    }
    
} // UpdateTickState

void Game::SaveState() {
    namespace fs = std::filesystem;
    
    if (save_file_.empty()) {
        return;
    }
    
    // код, который отвечает за открытие файла
    using OutputArchive = boost::archive::text_oarchive;
    
    fs::path temp_path(save_file_);
    temp_path += ".tmp";
    
    std::ofstream ofs(temp_path);
    
    if (!ofs) throw std::runtime_error("Cannot open file for saving state");
    OutputArchive oa(ofs);
    
    // Сериализуем весь лут
    std::vector<serialization::LootRepr> loots_to_serialize;
    for (auto session : sessions_) {
        for (auto loot : session->GetLoots()) {
            loots_to_serialize.push_back(serialization::LootRepr(std::make_shared<Loot>(loot)));
        }
        for (auto dog : session->GetDogs()) {
            for (auto loot : dog->GetBag()) {
                loots_to_serialize.push_back(serialization::LootRepr(std::make_shared<Loot>(loot)));
            }
        }
    }
    oa << loots_to_serialize;
    
    // Сериализуем всех собак
    std::vector<serialization::DogRepr> dogs_to_serialize;
    for (auto session : sessions_) {
        for (auto dog : session->GetDogs()) {
            dogs_to_serialize.push_back(serialization::DogRepr(dog));
        }
    }
    oa << dogs_to_serialize;

    // Сериализуем все сессии
    std::vector<serialization::GameSessionRepr> sessions_to_serialize;
    for (auto session : sessions_) {
        sessions_to_serialize.push_back(serialization::GameSessionRepr(session));
    }
    oa << sessions_to_serialize;
    
    // Сериализуем всех пользователей
    std::vector<serialization::PlayerRepr> players_to_serialize;
    for (const auto& player : Players::GetAllPlayers()) {
        players_to_serialize.push_back(serialization::PlayerRepr(player));
    }
    oa << players_to_serialize;
    
    fs::rename(temp_path, fs::path(save_file_));
}

void Game::LoadState() {
    using InputArchive = boost::archive::text_iarchive;
    std::ifstream ifs(save_file_);
    if (!ifs) throw std::runtime_error("Cannot open file for loading state");
    InputArchive ia(ifs);
    
    // Десериализуем лут
    std::vector<serialization::LootRepr> loots_repr;
    ia >> loots_repr;
    std::vector<LootShared> loots_restored;
    for (const auto& repr : loots_repr) {
        loots_restored.push_back(repr.Restore());
    }
    
    // Десериализуем собак
    std::vector<serialization::DogRepr> dogs_repr;
    ia >> dogs_repr;
    std::vector<DogShared> dogs_restored;
    for (const auto& repr : dogs_repr) {
        dogs_restored.push_back(std::make_shared<model::Dog>(repr.Restore(loots_restored)));
    }

    // Десериализуем сессии
    std::vector<serialization::GameSessionRepr> sessions_repr;
    ia >> sessions_repr;
    sessions_.clear();
    std::vector<GameSessionShared> sessions_restored;
    for (const auto& repr : sessions_repr) {
        sessions_restored.push_back(repr.Restore(*this, dogs_restored, loots_restored));
    }
    
    sessions_ = sessions_restored;

     // Десериализуем игроков
     std::vector<serialization::PlayerRepr> players_repr;
     ia >> players_repr;
     std::vector<PlayerShared> players_restored;
     for (const auto& repr : players_repr) {
         players_restored.push_back(repr.Restore(*this, dogs_restored, sessions_restored));
     }
     Players::SetAllPlayers(players_restored);
}

}  // namespace model
