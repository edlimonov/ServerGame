#pragma once

#include <random>
#include <sstream>
#include <iomanip>
#include <string>

#include "model.h"
#include "tagged.h"

using namespace model;
using namespace util;

class Player;
using PlayerShared = std::shared_ptr<Player>;

class TokenGenerator {
public:
    static std::string GetToken();
private:
    static std::mt19937_64 init_generator();
};

class Player {
public:
    
    Player(DogShared dog, GameSessionShared session) : dog_(dog), session_(session){
        id_ = counter_++;
        token_ = Token(TokenGenerator::GetToken());
    }
    
    Player(uint64_t id, std::string token, DogShared dog, GameSessionShared session) : id_(id), token_(Token(token)), dog_(dog), session_(session) {}
    
    Token& GetToken() noexcept {
        return token_;
    }
    
    uint64_t Id() const noexcept {
        return id_;
    }
    
    Dogs& GetSessionDogs() const noexcept ;
    
    DogShared GetDog() const noexcept {
        return dog_;
    }
    
    GameSessionShared GetSession() {
        return session_;
    }
    
    const GameSessionShared GetSession() const {
        return session_;
    }
    
private:
    uint64_t id_;
    Token token_{"default"};
    DogShared dog_;
    GameSessionShared session_;
    
    static uint64_t counter_;
};

class Players {
public:
    
    static PlayerShared AddPlayer(model::Game& game, std::string map_id, std::string user_name);
    static PlayerShared FindPlayerByToken(const std::string& token);
    static DogShared FindDogByToken(const std::string& token);
    static void RemovePlayerByDogId(int id);
    
    static std::vector<PlayerShared> GetAllPlayers() {
        return players_;
    }
    
    static void SetAllPlayers(const std::vector<PlayerShared>& new_players) {
        players_ = new_players;
    }
    
private:
    static std::vector<PlayerShared> players_;
};
