#pragma once

#include <vector>
#include <array>

template <typename T, typename isValidT, typename HeuristicT, typename WeightT,
    typename NeighboursT>
float ida_star_search(std::vector<T>& path, const float g, const float bound,
    T to, const isValidT& is_valid,
    const HeuristicT& heuristic, const WeightT& get_weight,
    const NeighboursT& get_neighbours)
{
    const auto& p = path.back();
    const float f = g + heuristic(p, to);
    if (f > bound)
        return f;
    if (p == to)
        return -f;

    float min = FLT_MAX;
    auto checkNeighbour = [&](T n) -> float {
        // out of bounds
        if (!is_valid(n))
            return 0.0f;
        if (std::find(path.begin(), path.end(), n) != path.end())
            return 0.f;
        path.push_back(n);
        float gScore = g + get_weight(n);
        const auto t = ida_star_search(path, gScore, bound, to, is_valid, heuristic,
            get_weight, get_neighbours);
        if (t < 0.f)
            return t;
        if (t < min)
            min = t;
        path.pop_back();
        return t;
    };
    for (const auto& n : get_neighbours(p)) {
        const auto t = checkNeighbour(n);
        if (t < 0.f)
            return t;
    }
    return min;
}
