#include "postgres.h"

#include <pqxx/result.hxx>

namespace postgres {

void DB::Init(PoolShared pool) {
    auto connection_ = pool->GetConnection();
    pqxx::work work{*connection_};
    
    // Включаем расширение pgcrypto (если ещё не включено)
    work.exec("CREATE EXTENSION IF NOT EXISTS pgcrypto");
    
    work.exec(pqxx::zview(
        "CREATE TABLE IF NOT EXISTS retired_players ("
        "    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),"
        "    name VARCHAR(100) NOT NULL,"
        "    score INTEGER NOT NULL,"
        "    play_time_ms BIGINT NOT NULL"
        ")"
    ));
    
    work.commit();
}

std::vector<PlayerInfo> DB::GetPlayersInfo(PoolShared pool, int start, int max_items) {
    // Валидация параметров
    start = std::max(0, start);          // Оффсет не может быть отрицательным
    max_items = std::max(1, max_items);  // Лимит не может быть меньше 1

    std::vector<PlayerInfo> result;
    auto connection = pool->GetConnection();
    pqxx::work work{*connection};

    try {
        auto result_set = work.exec_params(
            "SELECT name, score, play_time_ms "
            "FROM retired_players "
            "ORDER BY score DESC, play_time_ms, name "
            "OFFSET $1 LIMIT $2",
            start,
            max_items  // Ограничение максимального лимита
        );

        for (auto row : result_set) {
            PlayerInfo info;
            info.name = row["name"].as<std::string>();
            info.total_score = row["score"].as<int>();
            info.full_game_period = row["play_time_ms"].as<int>();
            result.push_back(info);
        }

    } catch (const std::exception& e) {
        // Логирование ошибки
        throw std::runtime_error("Database query failed: " + std::string(e.what()));
    }

    return result;
}

void DB::SaveRecord(PoolShared pool, PlayerInfo record) {

    auto connection = pool->GetConnection();
    pqxx::work work{*connection};

    try {
        work.exec_params(
            "INSERT INTO retired_players (name, score, play_time_ms) "
            "VALUES ($1, $2, $3)",
            record.name,
            record.total_score,
            record.full_game_period
        );
        work.commit();
    } catch (const pqxx::sql_error& e) {
        throw std::runtime_error("Database error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Unexpected error: " + std::string(e.what()));
    }
}

};
