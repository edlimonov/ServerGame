#pragma once

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/unordered_map.hpp>

#include "model.h"
#include "application.h"
#include "geom.h"
#include <algorithm>

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar & point.x;
    ar & point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar & vec.x;
    ar & vec.y;
}

} // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, model::Point& point, [[maybe_unused]] const unsigned version) {
    ar & point.x;
    ar & point.y;
}

template <typename Archive>
void serialize(Archive& ar, model::Coords& coords, [[maybe_unused]] const unsigned version) {
    ar & coords.x;
    ar & coords.y;
}

template <typename Archive>
void serialize(Archive& ar, model::Speed& speed, [[maybe_unused]] const unsigned version) {
    ar & speed.x;
    ar & speed.y;
}

template <typename Archive>
void serialize(Archive& ar, model::Direction& dir, [[maybe_unused]] const unsigned version) {
    ar & dir;
}

template <typename Archive>
void serialize(Archive& ar, model::Size& size, [[maybe_unused]] const unsigned version) {
    ar & size.width;
    ar & size.height;
}

template <typename Archive>
void serialize(Archive& ar, model::Rectangle& rect, [[maybe_unused]] const unsigned version) {
    ar & rect.position;
    ar & rect.size;
}

template <typename Archive>
void serialize(Archive& ar, model::Offset& offset, [[maybe_unused]] const unsigned version) {
    ar & offset.dx;
    ar & offset.dy;
}

} // namespace model

namespace serialization {

using DogSharedPtr = std::shared_ptr<model::Dog>;
using GameSessionSharedPtr = std::shared_ptr<model::GameSession>;
using PlayerSharedPtr = std::shared_ptr<Player>;
using LootSharedPtr = std::shared_ptr<model::Loot>;
using MapSharedPtr = std::shared_ptr<model::Map>;

// LootRepr (LootsRepresentation) - сериализованное представление класса Loot
class LootRepr {
public:
    LootRepr() = default;
    
    explicit LootRepr(LootSharedPtr loot)
        : id_(loot->GetId())
        , loot_type_(loot->GetType())
        , position_(loot->GetPosition())
    {
    }
    
    [[nodiscard]] LootSharedPtr Restore() const {
        LootSharedPtr result = std::make_shared<model::Loot>(id_, loot_type_, position_);
        return result;
    }
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & id_;
        ar & loot_type_;
        ar & position_;
    }
    
private:
    int id_;
    int loot_type_;
    model::Coords position_;
};

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;
    
    explicit DogRepr(DogSharedPtr dog)
        : name_(dog->GetName())
        , cords_(dog->GetCords())
        , speed_(dog->GetSpeed())
        , direction_(dog->GetDirection())
        , map_speed_(dog->GetMapSpeed())
        , bag_capacity_(dog->GetCapacity())
        , score_(dog->GetScore())
        , full_time_(dog->GetFullTime())
        , retire_time_(dog->GetRetireTime())
    {
        for (auto loot : dog->GetBag()) {
            id_loots_in_bag_.push_back(loot.GetId());
        }
    }
    
    [[nodiscard]] model::Dog Restore(std::vector<model::LootShared> loots) const {
        model::Dog result(name_);
        result.SetCords(cords_);
        result.SetMapSpeed(map_speed_);
        result.SetSpeed(speed_);
        result.SetDirection(direction_);
        result.SetBagCapacity(bag_capacity_);
        result.SetScore(score_);
        
        model::Loots restored_bag;
        for (auto loot : loots) {
            if (std::find(id_loots_in_bag_.begin(), id_loots_in_bag_.end(), loot->GetId()) != id_loots_in_bag_.end()) {
                restored_bag.push_back(*loot);
            }
        }
        
        result.SetBag(restored_bag);
        result.SetFullTime(full_time_);
        result.SetRetireTime(retire_time_);
        
        return result;
    }
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & id_;
        ar & name_;
        ar & cords_;
        ar & speed_;
        ar & direction_;
        ar & map_speed_;
        ar & bag_capacity_;
        ar & score_;
        ar & id_loots_in_bag_;
        ar & full_time_;
        ar & retire_time_;
    }
    
private:
    int id_;
    std::string name_;
    model::Coords cords_;
    model::Speed speed_;
    model::Direction direction_;
    double map_speed_;
    int bag_capacity_;
    int score_;
    std::vector<int> id_loots_in_bag_;
    int full_time_;
    int retire_time_;
};

// GameSessionRepr (GameSessionRepresentation) - сериализованное представление класса GameSession
class GameSessionRepr {
public:
    GameSessionRepr() = default;
    
    explicit GameSessionRepr(GameSessionSharedPtr gameSession)
        : id_(gameSession->GetId()), id_map_(*(gameSession->GetMapId()))
    {
        for (auto& loot : gameSession->GetLoots()) {
            id_loots_.push_back(loot.GetId());
        }
        
        for (auto& dog : gameSession->GetDogs()) {
            id_dogs_.push_back(dog->GetId());
        }
    }
    
    [[nodiscard]] GameSessionSharedPtr Restore(const model::Game& game,
                                               std::vector<DogSharedPtr>& dogs_restored,
                                               std::vector<LootSharedPtr>& loots_restored) const {
        
        MapSharedPtr map_ptr = game.FindMap(model::Map::Id(id_map_));
        GameSessionSharedPtr session = std::make_shared<model::GameSession>(id_, map_ptr);
        
        model::Dogs dogs_to_add;
        for (auto dog : dogs_restored) {
            if (std::find(id_dogs_.begin(), id_dogs_.end(), dog->GetId()) != id_dogs_.end()) {
                dogs_to_add.push_back(dog);
            }
        }
        session->AddDogs(dogs_to_add);
        
        model::Loots loots_to_add;
        for (auto loot : loots_restored) {
            if (std::find(id_loots_.begin(), id_loots_.end(), loot->GetId()) != id_loots_.end()) {
                loots_to_add.push_back(*loot);
            }
        }
        session->AddLoots(loots_to_add);
        
        return session;
    }
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & id_;
        ar & id_map_;
        ar & id_loots_;
        ar & id_dogs_;
    }
    
private:
    int id_;
    std::string id_map_;
    std::vector<int> id_loots_;
    std::vector<int> id_dogs_;
};

// PlayerRepr (PlayerRepresentation) - сериализованное представление класса Player
class PlayerRepr {
public:
    PlayerRepr() = default;
    
    explicit PlayerRepr(PlayerSharedPtr player)
        : id_(player->Id())
        , token_(*(player->GetToken()))
        , dog_id_(player->GetDog()->GetId())
        , session_id_(player->GetSession()->GetId())
    {
    }
    
    [[nodiscard]] PlayerSharedPtr Restore(const model::Game& game,
                                         std::vector<DogSharedPtr>& dogs_restored,
                                         std::vector<GameSessionSharedPtr>& sessions_restored) const {
        
        DogSharedPtr dog_to_add;
        for (auto& dog : dogs_restored) {
            if (dog->GetId() == dog_id_) {
                dog_to_add = dog;
                break;
            }
        }
        
        GameSessionSharedPtr session_to_add;
        for (auto& session : sessions_restored) {
            if (session->GetId() == session_id_) {
                session_to_add = session;
                break;
            }
        }
        
        PlayerSharedPtr result = std::make_shared<Player>(id_, token_, dog_to_add, session_to_add);
        return result;
    }
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & id_;
        ar & token_;
        ar & dog_id_;
        ar & session_id_;
    }
    
private:
    uint64_t id_;
    std::string token_;
    int dog_id_;
    int session_id_;
};

} // namespace serialization
