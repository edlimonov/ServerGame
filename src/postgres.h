#include "connection_pool.h"

namespace postgres {

struct PlayerInfo {
    std::string name;
    int total_score;
    int full_game_period;
};

using PlayersInfo = std::vector<PlayerInfo>;

class DB {
public:
    static void Init(PoolShared pool);
    static PlayersInfo GetPlayersInfo(PoolShared pool, int start, int max_items);
    static void SaveRecord(PoolShared pool, PlayerInfo record);
};

}
