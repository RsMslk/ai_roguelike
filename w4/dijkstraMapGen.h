#pragma once
#include <vector>
#include <flecs.h>

namespace dmaps
{
    void gen_player_approach_map(flecs::world& ecs, std::vector<float>& map, float range = 0.0f);
    void gen_player_flee_map(flecs::world& ecs, std::vector<float>& map);
    void gen_hive_pack_map(flecs::world& ecs, std::vector<float>& map);
    void gen_explore_map(flecs::world& ecs, std::vector<float>& map);
    void gen_ally_map(flecs::world& ecs, std::vector<float>& map,
        flecs::entity me);
};

