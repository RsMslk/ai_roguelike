#include "raylib.h"
#include <functional>
#include <vector>
#include <limits>
#include <float.h>
#include <cmath>
#include <cassert>
#include <algorithm>
#include "math.h"
#include "dungeonGen.h"
#include "dungeonUtils.h"
#include "path.h"

template<typename T>
static size_t coord_to_idx(T x, T y, size_t w)
{
    return size_t(y) * w + size_t(x);
}

static void draw_nav_grid(const char* input, size_t width, size_t height)
{
    for (size_t y = 0; y < height; ++y)
        for (size_t x = 0; x < width; ++x)
        {
            char symb = input[coord_to_idx(x, y, width)];
            Color color = GetColor(symb == ' ' ? 0xeeeeeeff : symb == 'o' ? 0x7777ffff : 0x222222ff);
            const Rectangle rect = { float(x), float(y), 1.f, 1.f };
            DrawRectangleRec(rect, color);
            //DrawPixel(int(x), int(y), color);
        }
}

static void draw_path(std::vector<Position> path)
{
    for (const Position& p : path)
    {
        const Rectangle rect = { float(p.x), float(p.y), 1.f, 1.f };
        DrawRectangleRec(rect, GetColor(0x44000088));
    }
}

static std::vector<Position> reconstruct_path(std::vector<Position> prev, Position to, size_t width)
{
    Position curPos = to;
    std::vector<Position> res = { curPos };
    while (prev[coord_to_idx(curPos.x, curPos.y, width)] != Position{ -1, -1 })
    {
        curPos = prev[coord_to_idx(curPos.x, curPos.y, width)];
        res.insert(res.begin(), curPos);
    }
    return res;
}

float heuristic(Position lhs, Position rhs)
{
    return sqrtf(square(float(lhs.x - rhs.x)) + square(float(lhs.y - rhs.y)));
};

static std::vector<Position> find_ida_star_path(const char* input, size_t width, size_t height, Position from, Position to)
{
    const auto heuristicCb = [&](Position from, Position to) -> float { return heuristic(from, to); };
    const auto validCb = [&](Position p) -> bool
    {
        if (p.x < 0 || p.y < 0 || p.x >= int(width) || p.y >= int(height))
            return false;
        size_t idx = coord_to_idx(p.x, p.y, width);
        if (input[idx] == '#')
            return false;
        return true;
    };
    const auto weightCb = [&](Position p) -> float {
        size_t idx = coord_to_idx(p.x, p.y, width);
        return input[idx] == 'o' ? 10.f : 1.f;
    };
    const auto neighboursCb = [&](Position p) {
        return std::array<Position, 4> { {
            {p.x + 1, p.y + 0},
            { p.x - 1, p.y + 0 },
            { p.x + 0, p.y + 1 },
            { p.x + 0, p.y - 1 }
            }};
    };
    float bound = heuristic(from, to);
    std::vector<Position> path = { from };
    while (true)
    {
        const float t = ida_star_search(path, 0.f, bound, to, validCb, heuristicCb, weightCb, neighboursCb);
        // const float t = ida_star_search(input, width, height, path, 0.0f, bound, to);
        if (t < 0.f)
            return path;
        if (t == FLT_MAX)
            return {};
        bound = t;
        printf("new bound %0.1f\n", bound);
    }
    return {};
}

template <typename F>
static std::pair<size_t, float>
get_best_score(const std::vector<Position>& open_list, const F& get_score)
{
    if (open_list.size() == 0)
        return { -1, FLT_MAX };
    size_t bestIdx = 0;
    float bestScore = get_score(open_list[0]);
    for (size_t i = 1; i < open_list.size(); ++i) {
        float score = get_score(open_list[i]);
        if (score < bestScore) {
            bestIdx = i;
            bestScore = score;
        }
    }
    return { bestIdx, bestScore };
}

bool is_valid(const char* input, Position p, size_t width, size_t height)
{
    if (p.x < 0 || p.y < 0 || p.x >= int(width) || p.y >= int(height))
        return false;
    size_t idx = coord_to_idx(p.x, p.y, width);
    if (input[idx] == '#')
        return false;

    return true;
}

static bool expand_state(Position p, Position cur, Position to, float e,
    const char* input, size_t width, std::vector<float>& g,
    std::vector<Position>& prev)
{
    auto getG = [&](Position p) -> float { return g[coord_to_idx(p.x, p.y, width)]; };

    size_t idx = coord_to_idx(p.x, p.y, width);
    float edgeWeight = input[idx] == 'o' ? 10.f : 1.f;
    float gScore = getG(cur) + edgeWeight;
    if (gScore < getG(p))
    {
        prev[idx] = cur;
        g[idx] = gScore;
        return true;
    }
    return false;
}

class ARASTAR
{
public:
    ARASTAR(size_t w, size_t h) : width(w), height(h) { reset(); };
    std::vector<Position> iteration(const char* input, Position from, Position to,
        float epsilon)
    {
        isExpanded = false;
        for (auto p : inconsList)
            openList.push_back(p);
        inconsList.clear();
        if (openList.empty())
        {
            openList.push_back(from);
            g[coord_to_idx(from.x, from.y, width)] = 0;
        }
        if (!is_valid(input, from, width, height))
            return std::vector<Position>();

        const auto getF = [&](Position p) -> float {
            return g[coord_to_idx(p.x, p.y, width)] + epsilon * heuristic(p, to);
        };

        std::vector<Position> closedList{};
        std::sort(openList.begin(), openList.end(),
            [&](Position a, Position b) { return getF(a) < getF(b); });
        auto [bestIdx, bestScore] = get_best_score(openList, getF);
        while (!openList.empty() && getF(to) > bestScore)
        {
            auto [bestIdx, score] = get_best_score(openList, getF);
            bestScore = score;
            Position curPos = openList[bestIdx];
            openList.erase(openList.begin() + bestIdx);
            if (std::find(visList.begin(), visList.end(), curPos) == visList.end())
                visList.emplace_back(curPos);
            closedList.emplace_back(curPos);
            const auto checkNeighbour = [&](Position p) {
                if (!is_valid(input, p, width, height))
                    return;
                if (expand_state(p, curPos, to, epsilon, input, width, g, prev))
                {
                    isExpanded = true;
                    if (std::find(closedList.begin(), closedList.end(), p) == closedList.end())
                    {
                        if (std::find(openList.begin(), openList.end(), p) == openList.end())
                            openList.emplace_back(p);
                    }
                    else if (std::find(inconsList.begin(), inconsList.end(), p) == inconsList.end())
                        inconsList.emplace_back(p);
                }
            };
            checkNeighbour({ curPos.x + 1, curPos.y + 0 });
            checkNeighbour({ curPos.x - 1, curPos.y + 0 });
            checkNeighbour({ curPos.x + 0, curPos.y + 1 });
            checkNeighbour({ curPos.x + 0, curPos.y - 1 });
        }
        return reconstruct_path(prev, to, width);
    }
    void reset()
    {
        g = std::vector<float>(width * height, std::numeric_limits<float>::max());
        prev = std::vector<Position>(width * height, { -1, -1 });
        openList.clear();
        inconsList.clear();
        visList.clear();
    }
    void draw()
    {
        for (auto p : visList)
        {
            size_t idx = coord_to_idx(p.x, p.y, width);
            const Rectangle rect = { float(p.x), float(p.y), 1.f, 1.f };
            DrawRectangleRec(rect, Color{ uint8_t(g[idx]), uint8_t(g[idx]), 0, 100 });
        }
    }

    bool isExpanded = false;
private:
    size_t width = 0;
    size_t height = 0;
    std::vector<float> g{};
    std::vector<float> f{};
    std::vector<Position> openList{};
    std::vector<Position> inconsList{};
    std::vector<Position> visList{};
    std::vector<Position> prev{};
};

static std::vector<Position> find_path_a_star(const char* input, size_t width, size_t height, Position from, Position to, float weight)
{
    if (from.x < 0 || from.y < 0 || from.x >= int(width) || from.y >= int(height))
        return std::vector<Position>();
    size_t inpSize = width * height;

    std::vector<float> g(inpSize, std::numeric_limits<float>::max());
    std::vector<Position> prev(inpSize, { -1,-1 });

    auto getG = [&](Position p) -> float { return g[coord_to_idx(p.x, p.y, width)]; };
    const auto getF = [&](Position p) -> float {
        return g[coord_to_idx(p.x, p.y, width)] + weight * heuristic(p, to);
    };

    g[coord_to_idx(from.x, from.y, width)] = 0;

    std::vector<Position> openList = { from };
    std::vector<Position> closedList;

    while (!openList.empty())
    {
        auto [bestIdx, _] = get_best_score(openList, getF);
        if (openList[bestIdx] == to)
            return reconstruct_path(prev, to, width);
        Position curPos = openList[bestIdx];
        openList.erase(openList.begin() + bestIdx);
        if (std::find(closedList.begin(), closedList.end(), curPos) != closedList.end())
            continue;
        size_t idx = coord_to_idx(curPos.x, curPos.y, width);
        const Rectangle rect = { float(curPos.x), float(curPos.y), 1.f, 1.f };
        DrawRectangleRec(rect, Color{ uint8_t(g[idx]), uint8_t(g[idx]), 0, 100 });
        closedList.emplace_back(curPos);
        auto checkNeighbour = [&](Position p)
        {
            if (!is_valid(input, p, width, height))
                return;
            expand_state(p, curPos, to, weight, input, width, g, prev);
            bool found = std::find(openList.begin(), openList.end(), p) != openList.end();
            if (!found)
                openList.emplace_back(p);
        };
        checkNeighbour({ curPos.x + 1, curPos.y + 0 });
        checkNeighbour({ curPos.x - 1, curPos.y + 0 });
        checkNeighbour({ curPos.x + 0, curPos.y + 1 });
        checkNeighbour({ curPos.x + 0, curPos.y - 1 });
    }
    // empty path
    return std::vector<Position>();
}

void draw_nav_data(ARASTAR& araStar, const char* input, size_t width, size_t height,
    Position from, Position to, float weight) {
    static const auto framePerARAUpdate = 32;
    static const auto epsStep = 0.05;
    static const auto epsStart = 10.f;
    static auto eps = epsStart;
    draw_nav_grid(input, width, height);
    //std::vector<Position> path = find_path_a_star(input, width, height, from, to, weight);
    //std::vector<Position> path = find_ida_star_path(input, width, height, from, to);
    static size_t frame = 0;
    static std::vector<Position> path{};
    if (frame++ % framePerARAUpdate == 0)
    {
        araStar.isExpanded = false;
        while (!araStar.isExpanded)
        {
            if (eps < 1.f)
            {
                araStar.reset();
                eps = epsStart;
            }
            path = araStar.iteration(input, from, to, eps);
            eps -= epsStep;
        }
    }
    araStar.draw();
    draw_path(path);
}

int main(int /*argc*/, const char** /*argv*/)
{
    int width = 900;
    int height = 900;
    InitWindow(width, height, "w3 AI MIPT");

    const int scrWidth = GetMonitorWidth(0);
    const int scrHeight = GetMonitorHeight(0);
    if (scrWidth < width || scrHeight < height)
    {
        width = std::min(scrWidth, width);
        height = std::min(scrHeight - 150, height);
        SetWindowSize(width, height);
    }

    constexpr size_t dungWidth = 100;
    constexpr size_t dungHeight = 100;
    char* navGrid = new char[dungWidth * dungHeight];
    gen_drunk_dungeon(navGrid, dungWidth, dungHeight, 24, 100);
    spill_drunk_water(navGrid, dungWidth, dungHeight, 8, 10);
    float weight = 1.f;

    ARASTAR araStar(dungWidth, dungHeight);
    Position from = dungeon::find_walkable_tile(navGrid, dungWidth, dungHeight);
    Position to = dungeon::find_walkable_tile(navGrid, dungWidth, dungHeight);

    Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
    //camera.offset = Vector2{ width * 0.5f, height * 0.5f };
    camera.zoom = float(height) / float(dungHeight);

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    while (!WindowShouldClose())
    {
        // pick pos
        Vector2 mousePosition = GetScreenToWorld2D(GetMousePosition(), camera);
        Position p{ int(mousePosition.x), int(mousePosition.y) };
        if (IsMouseButtonPressed(2) || IsKeyPressed(KEY_Q))
        {
            araStar.reset();
            size_t idx = coord_to_idx(p.x, p.y, dungWidth);
            if (idx < dungWidth * dungHeight)
                navGrid[idx] = navGrid[idx] == ' ' ? '#' : navGrid[idx] == '#' ? 'o' : ' ';
        }
        else if (IsMouseButtonPressed(0))
        {
            Position& target = from;
            target = p;
            araStar.reset();
        }
        else if (IsMouseButtonPressed(1))
        {
            Position& target = to;
            target = p;
            araStar.reset();
        }
        if (IsKeyPressed(KEY_SPACE))
        {
            gen_drunk_dungeon(navGrid, dungWidth, dungHeight, 24, 100);
            spill_drunk_water(navGrid, dungWidth, dungHeight, 8, 10);
            from = dungeon::find_walkable_tile(navGrid, dungWidth, dungHeight);
            to = dungeon::find_walkable_tile(navGrid, dungWidth, dungHeight);
            araStar.reset();
        }
        if (IsKeyPressed(KEY_UP))
        {
            weight += 0.1f;
            printf("new weight %f\n", weight);
        }
        if (IsKeyPressed(KEY_DOWN))
        {
            weight = std::max(1.f, weight - 0.1f);
            printf("new weight %f\n", weight);
        }
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);
        draw_nav_data(araStar, navGrid, dungWidth, dungHeight, from, to, weight);
        EndMode2D();
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
