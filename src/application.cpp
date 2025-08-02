#include "application.h"

std::vector<PlayerShared> Players::players_;
uint64_t Player::counter_ = 0;

std::string TokenGenerator::GetToken() {
    // Инициализация генераторов при первом вызове
    static thread_local std::mt19937_64 generator1 = init_generator();
    static thread_local std::mt19937_64 generator2 = init_generator();
    
    // Генерация двух 64-битных чисел
    uint64_t num1 = generator1();
    uint64_t num2 = generator2();
    
    // Преобразование в hex-строку
    std::stringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(16) << num1
       << std::setw(16) << num2;
    
    return ss.str();
}

std::mt19937_64 TokenGenerator::init_generator() {
    std::random_device rd;
    std::uniform_int_distribution<uint64_t> dist;
    return std::mt19937_64(dist(rd));
}

PlayerShared Players::AddPlayer(model::Game& game, std::string user_name, std::string map_id) {
    
    auto new_dog = std::make_shared<Dog>(user_name);
    auto session = game.AddDogToSession(new_dog, map_id);
    
    // нет такой карты
    if (!session) {
        return nullptr;
    }
    
    players_.push_back(PlayerShared(new Player(new_dog, session)));
    return players_.back();
}

PlayerShared Players::FindPlayerByToken(const std::string& token) {
    
    Token player_token(token);
    
    for (auto player : players_) {
        if (player->GetToken() == player_token) {
            return player;
        }
    }
    
    return nullptr;
}

Dogs& Player::GetSessionDogs() const noexcept {
    return session_->GetDogs();
}

DogShared Players::FindDogByToken(const std::string& token) {
    auto player = FindPlayerByToken(token);
    return player->GetDog();
}

void Players::RemovePlayerByDogId(int id) {
    for (auto it = players_.begin(); it != players_.end(); ) {
        auto player = *it;
        
        if (player->GetDog()->GetId() == id) {
            it = players_.erase(it);
        } else {
            ++it;
        }
        
    }
}
