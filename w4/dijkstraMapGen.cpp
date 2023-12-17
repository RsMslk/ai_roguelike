#include "dijkstraMapGen.h"
#include "ecsTypes.h"
#include "dungeonUtils.h"

template<typename Callable>
static void query_dungeon_data(flecs::world& ecs, Callable c)
{
    static auto dungeonDataQuery = ecs.query<const DungeonData>();

    dungeonDataQuery.each(c);
}

template<typename Callable>
static void query_characters_positions(flecs::world& ecs, Callable c)
{
    static auto characterPositionQuery = ecs.query<const Position, const Team>();

    characterPositionQuery.each(c);
}

constexpr float invalid_tile_value = 1e5f;

static void init_tiles(std::vector<float>& map, const DungeonData& dd)
{
    map.resize(dd.width * dd.height);
    for (float& v : map)
        v = invalid_tile_value;
}

static float getMapAt(const std::vector<float>& map, const DungeonData& dd, size_t x,
    size_t y, float def) {
    if (x < dd.width && y < dd.width &&
        dd.tiles[y * dd.width + x] == dungeon::floor)
        return map[y * dd.width + x];
    return def;
};

// scan version, could be implemented as Dijkstra version as well
static void process_dmap(std::vector<float>& map, const DungeonData& dd)
{
    bool done = false;
    auto getMinNei = [&](size_t x, size_t y)
    {
        float val = map[y * dd.width + x];
        val = std::min(val, getMapAt(map, dd, x - 1, y + 0, val));
        val = std::min(val, getMapAt(map, dd, x + 1, y + 0, val));
        val = std::min(val, getMapAt(map, dd, x + 0, y - 1, val));
        val = std::min(val, getMapAt(map, dd, x + 0, y + 1, val));
        return val;
    };
    while (!done)
    {
        done = true;
        for (size_t y = 0; y < dd.height; ++y)
            for (size_t x = 0; x < dd.width; ++x)
            {
                const size_t i = y * dd.width + x;
                if (dd.tiles[i] != dungeon::floor)
                    continue;
                const float myVal = getMapAt(map, dd, x, y, invalid_tile_value);
                const float minVal = getMinNei(x, y);
                if (minVal < myVal - 1.f)
                {
                    map[i] = minVal + 1.f;
                    done = false;
                }
            }
    }
}

static bool isVisibleTileFromPos(const DungeonData& dd, int x, int y,
    int tile_x, int tile_y)
{
    // Traverse in the direction and check walls
    int curX = x;
    int curY = y;
    while ((curX != tile_x || curY != tile_y))
    {
        if (dd.tiles[curY * dd.width + curX] == dungeon::wall)
            return false;
        int deltaX = tile_x - curX;
        int deltaY = tile_y - curY;
        if (abs(deltaX) > abs(deltaY))
            curX += deltaX > 0 ? 1 : -1;
        else
            curY += deltaY > 0 ? 1 : -1;
    }
    return true;
}

static int getRange(int x, int y, int tx, int ty) { return abs(x - tx) + abs(y - ty); }

static bool isInRange(int x, int y, int tile_x, int tile_y, float range)
{
    if (getRange(x, y, tile_x, tile_y) <= range)
        return true;

    return false;
}

void dmaps::gen_player_approach_map(flecs::world& ecs, std::vector<float>& map, float range)
{
    query_dungeon_data(ecs, [&](const DungeonData& dd)
        {
            init_tiles(map, dd);
            query_characters_positions(ecs, [&](const Position& pos, const Team& t)
                {
                    if (t.team == 0) // player team hardcode
                    {
                        for (int dy = -range; dy <= range; dy++)
                            for (int dx = -range; dx <= range; dx++)
                            {
                                int x = pos.x + dx;
                                int y = pos.y + dy;
                                if (x < dd.width && y < dd.width && x >= 0 && y >= 0 &&
                                    dd.tiles[y * dd.width + x] == dungeon::floor &&
                                    isVisibleTileFromPos(dd, x, y, pos.x, pos.y))
                                {
                                    int tileRange = getRange(x, y, pos.x, pos.y);
                                    if (tileRange <= range)
                                        map[y * dd.width + x] = 0;
                                }
                            }
                    }
                });
            process_dmap(map, dd);
        });
}

void dmaps::gen_player_flee_map(flecs::world& ecs, std::vector<float>& map)
{
    gen_player_approach_map(ecs, map);
    for (float& v : map)
        if (v < invalid_tile_value)
            v *= -1.2f;
    query_dungeon_data(ecs, [&](const DungeonData& dd)
        {
            process_dmap(map, dd);
        });
}

void dmaps::gen_hive_pack_map(flecs::world& ecs, std::vector<float>& map)
{
    static auto hiveQuery = ecs.query<const Position, const Hive>();
    query_dungeon_data(ecs, [&](const DungeonData& dd)
        {
            init_tiles(map, dd);
            hiveQuery.each([&](const Position& pos, const Hive&)
                {
                    map[pos.y * dd.width + pos.x] = 0.f;
                });
            process_dmap(map, dd);
        });
}

void dmaps::gen_explore_map(flecs::world& ecs, std::vector<float>& map)
{
    static auto explorerQuery = ecs.query<const Position, Explorer>();
    query_dungeon_data(ecs, [&](const DungeonData& dd)
        {
            init_tiles(map, dd);
            explorerQuery.each([&](const Position& pos, Explorer& exp)
                {
                    exp.exploreMap.resize(dd.width * dd.height);
                    for (size_t y = 0; y < dd.height; ++y)
                        for (size_t x = 0; x < dd.width; ++x) {
                            const size_t tileI = y * dd.width + x;
                            if (dd.tiles[tileI] != dungeon::floor)
                                continue;
                            float tileVal = getMapAt(exp.exploreMap, dd, x, y, invalid_tile_value);
                            if (tileVal == exp.UnexploredTile &&
                                isInRange(pos.x, pos.y, x, y, exp.exploreRange) &&
                                isVisibleTileFromPos(dd, pos.x, pos.y, x, y))
                            {
                                exp.exploreMap[tileI] = exp.ExploredTile;
                                tileVal = exp.ExploredTile;
                            }
                            map[tileI] = tileVal;
                        }
                });
            process_dmap(map, dd);
        });
}

void dmaps::gen_ally_map(flecs::world& ecs, std::vector<float>& map,
    flecs::entity me) {
    static auto allyQuery = ecs.query<const Position, const Team>();
    query_dungeon_data(ecs, [&](const DungeonData& dd)
        {
            init_tiles(map, dd);
            me.get([&](const Team& my_team, const Hitpoints& hp)
                {
                    allyQuery.each([&](flecs::entity e, const Position& pos, const Team& t)
                        {
                            if (e != me && t.team == my_team.team && hp.hitpoints < 30.0f)
                                map[pos.y * dd.width + pos.x] = 0.0f;
                        });
                });
            process_dmap(map, dd);
        });
}
