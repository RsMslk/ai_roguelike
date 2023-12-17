#pragma once
#include <flecs.h>

using Followers = flecs::query<const Position, Action, const DmapWeights>;
void process_dmap_followers(flecs::world& ecs, Followers& query);

