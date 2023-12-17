#include "aiLibrary.h"
#include <flecs.h>
#include "ecsTypes.h"
#include "raylib.h"
#include <cfloat>
#include <cmath>

class AttackEnemyState : public State
{
public:
    void enter(flecs::world& ecs, flecs::entity entity) override {}
    void exit(flecs::world& ecs, flecs::entity entity) override {}
    void act(float/* dt*/, flecs::world&/*ecs*/, flecs::entity /*entity*/) override {}
};

template<typename T>
T sqr(T a) { return a * a; }

template<typename T, typename U>
static float dist_sq(const T& lhs, const U& rhs) { return float(sqr(lhs.x - rhs.x) + sqr(lhs.y - rhs.y)); }

template<typename T, typename U>
static float dist(const T& lhs, const U& rhs) { return sqrtf(dist_sq(lhs, rhs)); }

template<typename T, typename U>
static int move_towards(const T& from, const U& to)
{
    int deltaX = to.x - from.x;
    int deltaY = to.y - from.y;
    if (abs(deltaX) > abs(deltaY))
        return deltaX > 0 ? EA_MOVE_RIGHT : EA_MOVE_LEFT;
    return deltaY < 0 ? EA_MOVE_UP : EA_MOVE_DOWN;
}

static int inverse_move(int move)
{
    return move == EA_MOVE_LEFT ? EA_MOVE_RIGHT :
        move == EA_MOVE_RIGHT ? EA_MOVE_LEFT :
        move == EA_MOVE_UP ? EA_MOVE_DOWN :
        move == EA_MOVE_DOWN ? EA_MOVE_UP : move;
}

static bool not_targetable(Team team)
{
    return false;
}

template<typename TargetCompare, typename Callable>
static void on_closest_target_pos(flecs::world& ecs, flecs::entity entity, TargetCompare condition, Callable c)
{
    static auto targetsQuery = ecs.query<const Position>();
    entity.get([&](const Position& pos)
        {
            flecs::entity closestTarget;
            float closestDist = FLT_MAX;
            Position closestPos;
            targetsQuery.each([&](flecs::entity target, const Position& tpos)
                {
                    if (entity == target || !condition(target))
                        return;
                    float curDist = dist(tpos, pos);
                    if (curDist < closestDist)
                    {
                        closestDist = curDist;
                        closestPos = tpos;
                        closestTarget = target;
                    }
                });
            if (ecs.is_valid(closestTarget))
                entity.set([&](Action& a)
                    {
                        c(a, pos, closestPos);
                    });
        });
}

class MoveToEnemyState : public State
{
public:
    void enter(flecs::world& ecs, flecs::entity entity) override {}
    void exit(flecs::world& ecs, flecs::entity entity) override {}
    void act(float/* dt*/, flecs::world& ecs, flecs::entity entity) override
    {
        on_closest_target_pos(ecs, entity,
            [entity](auto target)
            {
                bool isTargetable = true;
                entity.get([&](const Team& team)
                    {
                        target.get([&](const Team& target)
                            {
                                if (team.tag == target.tag)
                                    isTargetable = false;
                            });
                    });
                return isTargetable;
            },
            [&](Action& a, const Position& pos, const Position& enemy_pos)
            {
                a.action = move_towards(pos, enemy_pos);
            });
    }
};

class FleeFromEnemyState : public State
{
public:
    FleeFromEnemyState() {}
    void enter(flecs::world& ecs, flecs::entity entity) override {}
    void exit(flecs::world& ecs, flecs::entity entity) override {}
    void act(float/* dt*/, flecs::world& ecs, flecs::entity entity) override
    {
        on_closest_target_pos(ecs, entity,
            [entity](auto target)
            {
                bool isTargetable = true;
                entity.get([&](const Team& team)
                    {
                        target.get([&](const Team& target)
                            {
                                if (team.tag == target.tag)
                                    isTargetable = false;
                            });
                    });
                return isTargetable;
            },
            [&](Action& a, const Position& pos, const Position& enemy_pos)
            {
                a.action = inverse_move(move_towards(pos, enemy_pos));
            });
    }
};

template <typename T>
class MoveToTagState : public State
{
public:
    MoveToTagState() {}
    void enter(flecs::world& ecs, flecs::entity entity) override {}
    void exit(flecs::world& ecs, flecs::entity entity) override {}
    void act(float/* dt*/, flecs::world& ecs, flecs::entity entity) override
    {
        on_closest_target_pos(ecs, entity,
            [](auto target)
            {
                return target.has<T>();
            },
            [&](Action& a, const Position& pos, const Position& target_pos)
            {
                a.action = move_towards(pos, target_pos);
            });
    }
};

class MoveToCraftState : public State
{
public:
    MoveToCraftState() {}
    void enter(flecs::world& ecs, flecs::entity entity) override {}
    void exit(flecs::world& ecs, flecs::entity entity) override {}
    void act(float/* dt*/, flecs::world& ecs, flecs::entity entity) override
    {
        on_closest_target_pos(ecs, entity,
            [entity](auto target)
            {
                bool targetable = false;
                entity.get([&](const CraftTable& table)
                    {
                        targetable = target == table.entity;
                    });
                return targetable;
            },
            [&](Action& a, const Position& pos, const Position& target_pos)
            {
                a.action = move_towards(pos, target_pos);
            });
    }
};

using MoveToShopState = MoveToTagState<IsShop>;
using MoveToCommunicateState = MoveToTagState<IsCrafter>;

class PatrolState : public State
{
    float patrolDist;
public:
    PatrolState(float dist) : patrolDist(dist) {}
    void enter(flecs::world& ecs, flecs::entity entity) override {}
    void exit(flecs::world& ecs, flecs::entity entity) override {}
    void act(float/* dt*/, flecs::world& ecs, flecs::entity entity) override
    {
        entity.set([&](const Position& pos, const PatrolPos& ppos, Action& a)
            {
                if (dist(pos, ppos) > patrolDist)
                    a.action = move_towards(pos, ppos); // do a recovery walk
                else
                {
                    // do a random walk
                    a.action = GetRandomValue(EA_MOVE_START, EA_MOVE_END - 1);
                }
            });
    }
};

class SelfHealState : public State
{
public:
    SelfHealState(HealAmount heal) : heal(heal) {}
    void enter(flecs::world& ecs, flecs::entity entity) override {}
    void exit(flecs::world& ecs, flecs::entity entity) override {}
    void act(float/* dt*/, flecs::world& ecs, flecs::entity entity) override
    {
        entity.set([&](Hitpoints& hp)
            {
                hp.hitpoints += heal.amount;
            });
    }
private:
    HealAmount heal;
};

class GuardState : public State
{
public:
    GuardState(float guard_dist) : guardDist(guard_dist) {}
    void enter(flecs::world& ecs, flecs::entity entity) override {}
    void exit(flecs::world& ecs, flecs::entity entity) override {}
    void act(float/* dt*/, flecs::world& ecs, flecs::entity entity) override
    {
        static auto playerQuery = ecs.query<IsPlayer, Position>();
        playerQuery.each([&](const IsPlayer&, const Position& playerPos)
            {
                entity.set([&](const Position& pos, Action& a)
                    {
                        if (dist(pos, playerPos) > guardDist)
                            a.action = move_towards(pos, playerPos); // do a recovery walk
                    });
            });
    }
private:
    float guardDist;
};

class PlayerHealState : public State
{
public:
    PlayerHealState(HealAmount heal, int cooldown) : heal(heal), cooldown(cooldown) {}
    void enter(flecs::world& ecs, flecs::entity entity) override {}
    void exit(flecs::world& ecs, flecs::entity entity) override {}
    void act(float/* dt*/, flecs::world& ecs, flecs::entity entity) override
    {
        static auto playerQuery = ecs.query<IsPlayer, Position, Hitpoints>();
        playerQuery.each([&](const IsPlayer&, const Position& playerPos, Hitpoints& playerHp)
            {
                entity.set([&](const Position& pos, Action& a, HealCooldown& healCooldown)
                    {
                        if (dist(pos, playerPos) <= 1.)
                        {
                            playerHp.hitpoints += heal.amount;
                            healCooldown.timer = cooldown;
                        }
                        else
                        {
                            a.action = move_towards(pos, playerPos);
                        }
                    });
            });
    }
private:
    HealAmount heal;
    int cooldown;
};

template <typename T, int delta, int min = 0, int max = std::numeric_limits<int>::max()>
class CountDeltaState : public State
{
public:
    CountDeltaState(int init = 0) : init(init), count(init) {}
    void enter(flecs::world& ecs, flecs::entity entity) override
    {
        count = init;
        entity.set([this](T& component)
            {
                component.count = count;
            });
    }
    void exit(flecs::world& ecs, flecs::entity entity) override {}
    void act(float/* dt*/, flecs::world& ecs, flecs::entity entity) override
    {
        if (count >= max)
            count = min;
        if (count > min && delta < 0 || delta > 0)
        {
            count += delta;
            entity.set([this](T& component)
                {
                    component.count = count;
                });
        }
    }
protected:
    int init;
    int count;
};

using ShopState = CountDeltaState<ShopTimer, -1>;
using SleepState = CountDeltaState<SleepTimer, -1>;
using CommunicateState = CountDeltaState<CommunicateTimer, -1>;

class CraftState : public CountDeltaState<Craft, 1, 0, 8>
{
public:
    CraftState(int init = 0) : CountDeltaState(init) {}
    void exit(flecs::world& ecs, flecs::entity entity) override
    {
        init = count + 1;
    }
};

class WanderState : public CountDeltaState<WanderTimer, -1>
{
public:
    WanderState(int init = 0) : CountDeltaState(init) {}
    void act(float dt, flecs::world& ecs, flecs::entity entity) override
    {
        CountDeltaState::act(dt, ecs, entity);
        entity.set([&](Action& a)
            {
                a.action = GetRandomValue(EA_MOVE_START, EA_MOVE_END - 1);
            });
    }
};


class NopState : public State
{
public:
    void enter(flecs::world& ecs, flecs::entity entity) override {}
    void exit(flecs::world& ecs, flecs::entity entity) override {}
    void act(float/* dt*/, flecs::world& ecs, flecs::entity entity) override {}
};

class EnemyAvailableTransition : public StateTransition
{
    float triggerDist;
public:
    EnemyAvailableTransition(float in_dist) : triggerDist(in_dist) {}
    bool isAvailable(flecs::world& ecs, flecs::entity entity) const override
    {
        static auto enemiesQuery = ecs.query<const Position, const Team>();
        bool enemiesFound = false;
        entity.get([&](const Position& pos, const Team& t)
            {
                enemiesQuery.each([&](flecs::entity enemy, const Position& epos, const Team& et)
                    {
                        if (t.tag == et.tag || not_targetable(et))
                            return;
                        float curDist = dist(epos, pos);
                        enemiesFound |= curDist <= triggerDist;
                    });
            });
        return enemiesFound;
    }
};

template <typename Tag>
class TargetTagAvailableTransition : public StateTransition
{
    float triggerDist;
public:
    TargetTagAvailableTransition(float in_dist) : triggerDist(in_dist) {}
    bool isAvailable(flecs::world& ecs, flecs::entity entity) const override
    {
        static auto targetsQuery = ecs.query<const Position, const Tag>();
        bool targetFound = false;
        entity.get([&](const Position& pos)
            {
                targetsQuery.each([&](flecs::entity target, const Position& epos, const Tag& t)
                    {
                        if (entity == target)
                            return;
                        float curDist = dist(epos, pos);
                        targetFound |= curDist <= triggerDist;
                    });
            });
        return targetFound;
    }
};

using CommunicateTransition = TargetTagAvailableTransition<IsCrafter>;
using ShopTransition = TargetTagAvailableTransition<IsShop>;
using CraftTransition = TargetTagAvailableTransition<IsCraftTable>;

class HitpointsLessThanTransition : public StateTransition
{
    float threshold;
public:
    HitpointsLessThanTransition(float in_thres) : threshold(in_thres) {}
    bool isAvailable(flecs::world& ecs, flecs::entity entity) const override
    {
        bool hitpointsThresholdReached = false;
        entity.get([&](const Hitpoints& hp)
            {
                hitpointsThresholdReached |= hp.hitpoints < threshold;
            });
        return hitpointsThresholdReached;
    }
};

class PlayerHitpointsLessThanTransition : public StateTransition
{
public:
    PlayerHitpointsLessThanTransition(float in_thres) : threshold(in_thres) {}
    bool isAvailable(flecs::world& ecs, flecs::entity entity) const override
    {
        bool hitpointsThresholdReached = false;
        static auto playerQuery = ecs.query<IsPlayer, Hitpoints>();
        entity.get([&](const HealCooldown& healCooldown)
            {
                if (healCooldown.timer == 0.)
                    playerQuery.each([&](const IsPlayer&, Hitpoints& playerHp)
                        {
                            hitpointsThresholdReached |= playerHp.hitpoints < threshold;
                        });
            });
        return hitpointsThresholdReached;
    }
private:
    float threshold;
};

class EnemyReachableTransition : public StateTransition
{
public:
    bool isAvailable(flecs::world& ecs, flecs::entity entity) const override
    {
        return false;
    }
};

class NegateTransition : public StateTransition
{
    const StateTransition* transition; // we own it
public:
    NegateTransition(const StateTransition* in_trans) : transition(in_trans) {}
    ~NegateTransition() override { delete transition; }

    bool isAvailable(flecs::world& ecs, flecs::entity entity) const override
    {
        return !transition->isAvailable(ecs, entity);
    }
};

class AndTransition : public StateTransition
{
    const StateTransition* lhs; // we own it
    const StateTransition* rhs; // we own it
public:
    AndTransition(const StateTransition* in_lhs, const StateTransition* in_rhs) : lhs(in_lhs), rhs(in_rhs) {}
    ~AndTransition() override
    {
        delete lhs;
        delete rhs;
    }

    bool isAvailable(flecs::world& ecs, flecs::entity entity) const override
    {
        return lhs->isAvailable(ecs, entity) && rhs->isAvailable(ecs, entity);
    }
};

template <typename T, int init_count = 0>
class CountTransition : public StateTransition
{
public:
    CountTransition(int count = init_count) : count(count) {}
    bool isAvailable(flecs::world& ecs, flecs::entity entity) const override
    {
        bool isAvailable = false;
        entity.get([&](const T& component)
            {
                isAvailable = component.count == count;
            });
        return isAvailable;
    }
private:
    int count;
};

using WanderTimerTransition = CountTransition<WanderTimer>;
using SleepTimerTransition = CountTransition<SleepTimer>;
using CommunicateTimerTransition = CountTransition<CommunicateTimer>;
using ShopTimerTransition = CountTransition<ShopTimer>;
using MoveToShopTransition = CountTransition<Craft>;
using CraftThresholdTransition = CountTransition<Craft>;

// states
State* create_attack_enemy_state() { return new AttackEnemyState(); }
State* create_move_to_enemy_state() { return new MoveToEnemyState(); }
State* create_flee_from_enemy_state() { return new FleeFromEnemyState(); }
State* create_patrol_state(float patrol_dist) { return new PatrolState(patrol_dist); }
State* create_self_heal_state(HealAmount heal) { return new SelfHealState(heal); }
State* create_guard_state(float guard_dist) { return new GuardState(guard_dist); }
State* create_player_heal_state(HealAmount heal, int cooldown) { return new PlayerHealState(heal, cooldown); }
State* create_wander_state(int time) { return new WanderState(time); }
State* create_craft_state() { return new CraftState(); }
State* create_shop_state(int time) { return new ShopState(time); }
State* create_sleep_state(int time) { return new SleepState(time); }
State* create_nop_state() { return new NopState(); }
State* create_move_to_craft_state() { return new MoveToCraftState(); }
State* create_move_to_shop_state() { return new MoveToShopState(); }
State* create_move_to_communicate_state() { return new MoveToCommunicateState(); }
State* create_communicate_state(int time) { return new CommunicateState(time); }

// transitions
StateTransition* create_enemy_available_transition(float dist) { return new EnemyAvailableTransition(dist); }
StateTransition* create_enemy_reachable_transition() { return new EnemyReachableTransition(); }
StateTransition* create_hitpoints_less_than_transition(float thres) { return new HitpointsLessThanTransition(thres); }
StateTransition* create_player_hitpoints_less_than_transition(float thres) { return new PlayerHitpointsLessThanTransition(thres); }
StateTransition* create_negate_transition(StateTransition* in) { return new NegateTransition(in); }
StateTransition* create_and_transition(StateTransition* lhs, StateTransition* rhs) { return new AndTransition(lhs, rhs); }
StateTransition* create_wander_timer_transition() { return new WanderTimerTransition(); }
StateTransition* create_sleep_timer_transition() { return new SleepTimerTransition(); }
StateTransition* create_communicate_timer_transition() { return new CommunicateTimerTransition(); }
StateTransition* create_shop_timer_transition() { return new ShopTimerTransition(); }
StateTransition* create_communicate_transition() { return new CommunicateTransition(1.5f); }
StateTransition* create_shop_transition() { return new ShopTransition(1.0f); }
StateTransition* create_craft_transition() { return new CraftTransition(1.0f); }
StateTransition* create_move_to_shop_transition(int thres) { return new MoveToShopTransition(thres); }
StateTransition* create_craft_threshold_transition(int thres) { return new CraftThresholdTransition(thres); }
