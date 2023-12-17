#include "roguelike.h"
#include "ecsTypes.h"
#include "raylib.h"
#include "stateMachine.h"
#include "aiLibrary.h"


static enum MYCOLOR : uint32_t
{
    RED_COLOR = 0xff'00'00'ff,
    MAGENTA_COLOR = 0xee'00'ee'ff,
    BLACK_COLOR = 0x11'11'11'ff,
    DARK_RED_COLOR = 0x88'00'00'ff,
    WATER_LEAF_COLOR = 0xae'f4'dd'ff,
    MARINER_COLOR = 0x46'5b'9e'ff,
    GOLD_COLOR = 0xff'd7'00'ff,
    DARK_KHAKI_COLOR = 0xbd'b7'6b'ff,
    LIGHT_SALMON_COLOR = 0xff'a9'7a'ff,
};

static void add_patrol_attack_flee_sm(flecs::entity entity)
{
    entity.get([](StateMachine& sm)
        {
            int patrol = sm.addState(create_patrol_state(3.f));
            int moveToEnemy = sm.addState(create_move_to_enemy_state());
            int fleeFromEnemy = sm.addState(create_flee_from_enemy_state());

            sm.addTransition(create_enemy_available_transition(3.f), patrol, moveToEnemy);
            sm.addTransition(create_negate_transition(create_enemy_available_transition(5.f)), moveToEnemy, patrol);

            sm.addTransition(create_and_transition(create_hitpoints_less_than_transition(60.f), create_enemy_available_transition(5.f)),
                moveToEnemy, fleeFromEnemy);
            sm.addTransition(create_and_transition(create_hitpoints_less_than_transition(60.f), create_enemy_available_transition(3.f)),
                patrol, fleeFromEnemy);

            sm.addTransition(create_negate_transition(create_enemy_available_transition(7.f)), fleeFromEnemy, patrol);
        });
}

static void add_patrol_attack_berserk_sm(flecs::entity entity)
{
    entity.get([](StateMachine& sm)
        {
            const auto patrolState = sm.addState(create_patrol_state(3.f));
            const auto moveToEnemyState = sm.addState(create_move_to_enemy_state());

            sm.addTransition(create_hitpoints_less_than_transition(60.f), patrolState, moveToEnemyState);
        });
}

static void add_patrol_self_heal_sm(flecs::entity entity)
{
    entity.get([](StateMachine& sm)
        {
            const auto patrolState = sm.addState(create_patrol_state(3.f));
            const auto selfHealState = sm.addState(create_self_heal_state({ 30.0f }));

            sm.addTransition(create_hitpoints_less_than_transition(60.f), patrolState, selfHealState);
            sm.addTransition(create_negate_transition(create_hitpoints_less_than_transition(60.0f)), selfHealState, patrolState);
        });
}

static void add_guard_attack_player_heal_sm(flecs::entity entity)
{
    entity.get([](StateMachine& sm)
        {
            const auto guardState = sm.addState(create_guard_state(2.f));
            const auto playerHealState = sm.addState(create_player_heal_state({ 30.0f }, 10));
            const auto moveToEnemyState = sm.addState(create_move_to_enemy_state());

            sm.addTransition(create_player_hitpoints_less_than_transition(60.f), guardState, playerHealState);
            sm.addTransition(create_negate_transition(create_player_hitpoints_less_than_transition(60.0f)), playerHealState, guardState);
            sm.addTransition(create_enemy_available_transition(5.f), guardState, moveToEnemyState);
            sm.addTransition(create_negate_transition(create_enemy_available_transition(5.f)), moveToEnemyState, guardState);
        });
}

static void add_patrol_flee_sm(flecs::entity entity)
{
    entity.get([](StateMachine& sm)
        {
            int patrol = sm.addState(create_patrol_state(3.f));
            int fleeFromEnemy = sm.addState(create_flee_from_enemy_state());

            sm.addTransition(create_enemy_available_transition(3.f), patrol, fleeFromEnemy);
            sm.addTransition(create_negate_transition(create_enemy_available_transition(5.f)), fleeFromEnemy, patrol);
        });
}

static void add_attack_sm(flecs::entity entity)
{
    entity.get([](StateMachine& sm)
        {
            sm.addState(create_move_to_enemy_state());
        });
}


static StateMachine* create_craft_sm()
{
    const auto sm = new StateMachine();
    const auto craftState = sm->addState(create_craft_state());
    const auto moveToCraftState = sm->addState(create_move_to_craft_state());
    const auto shopState = sm->addState(create_shop_state(3));
    const auto moveToShopState = sm->addState(create_move_to_shop_state());

    sm->addTransition(create_craft_transition(), moveToCraftState, craftState);
    sm->addTransition(create_negate_transition(create_craft_transition()), craftState, moveToCraftState);
    sm->addTransition(create_move_to_shop_transition(3), craftState, moveToShopState);
    sm->addTransition(create_shop_transition(), moveToShopState, shopState);
    sm->addTransition(create_shop_timer_transition(), shopState, moveToCraftState);
    return sm;
}

static StateMachine* create_wander_sm()
{
    const auto sm = new StateMachine();
    sm->addState(create_wander_state(6));
    return sm;
}

static StateMachine* create_communicate_sm()
{
    const auto sm = new StateMachine();
    const auto communicateState = sm->addState(create_communicate_state(3));
    const auto moveToCommunicateState = sm->addState(create_move_to_communicate_state());

    sm->addTransition(create_communicate_transition(), moveToCommunicateState, communicateState);
    sm->addTransition(create_negate_transition(create_communicate_transition()), communicateState, moveToCommunicateState);
    return sm;
}

static StateMachine* create_sleep_sm()
{
    const auto sm = new StateMachine();
    sm->addState(create_sleep_state(3));
    return sm;
}

static void add_craft_wander_sleep_communicate_hsm(flecs::entity entity)
{
    entity.get([](HierarchyStateMachine& hsm)
        {
            const auto craftSm = hsm.addState(create_craft_sm());
            const auto wanderSm = hsm.addState(create_wander_sm());
            const auto communicateSm = hsm.addState(create_communicate_sm());
            const auto sleepSm = hsm.addState(create_sleep_sm());

            hsm.addTransition(create_craft_threshold_transition(5), craftSm, wanderSm);
            hsm.addTransition(create_wander_timer_transition(), wanderSm, sleepSm);
            hsm.addTransition(create_sleep_timer_transition(), sleepSm, communicateSm);
            hsm.addTransition(create_communicate_timer_transition(), communicateSm, craftSm);
        });
}

static flecs::entity create_monster(flecs::world& ecs, int x, int y, Color color, Tag team)
{
    return ecs.entity()
        .set(Position{ x, y })
        .set(MovePos{ x, y })
        .set(PatrolPos{ x, y })
        .set(Hitpoints{ 100.f })
        .set(Action{ EA_NOP })
        .set(Color{ color })
        .set(StateMachine{})
        .set(Team{ team })
        .set(NumActions{ 1, 0 })
        .set(MeleeDamage{ 20.f })
        .set(HealCooldown{});
}

static flecs::entity create_crafter(flecs::world& ecs, int x, int y, flecs::entity table)
{
    return ecs.entity()
        .set(Position{ x, y })
        .set(MovePos{ x, y })
        .set(PatrolPos{ x, y })
        .set(Hitpoints{ 500.f })
        .set(Action{ EA_NOP })
        .set(GetColor(LIGHT_SALMON_COLOR))
        .set(HierarchyStateMachine{})
        .set(Team{ Tag::Player })
        .set(NumActions{ 1, 0 })
        .set(MeleeDamage{ 20.f })
        .set(WanderTimer{})
        .set(SleepTimer{})
        .set(CommunicateTimer{})
        .set(Craft{})
        .add<IsCrafter>()
        .set(CraftTable{ table });
}

static void create_player(flecs::world& ecs, int x, int y)
{
    ecs.entity("player")
        .set(Position{ x, y })
        .set(MovePos{ x, y })
        .set(Hitpoints{ 100.f })
        .set(GetColor(0xeeeeeeff))
        .set(Action{ EA_NOP })
        .add<IsPlayer>()
        .set(Team{ Tag::Player })
        .set(PlayerInput{})
        .set(NumActions{ 2, 0 })
        .set(MeleeDamage{ 50.f });
}

static void create_heal(flecs::world& ecs, int x, int y, float amount)
{
    ecs.entity()
        .set(Position{ x, y })
        .set(HealAmount{ amount })
        .set(GetColor(0x44ff44ff));
}

static void create_powerup(flecs::world& ecs, int x, int y, float amount)
{
    ecs.entity()
        .set(Position{ x, y })
        .set(PowerupAmount{ amount })
        .set(Color{ 255, 255, 0, 255 });
}

static void create_shop(flecs::world& ecs, int x, int y)
{
    ecs.entity()
        .set(Position{ x, y })
        .set(GetColor(GOLD_COLOR))
        .add<IsShop>();
}

static auto create_craft_table(flecs::world& ecs, int x, int y)
{
    return ecs.entity()
        .set(Position{ x, y })
        .set(GetColor(DARK_KHAKI_COLOR))
        .add<IsCraftTable>();
}

static void register_roguelike_systems(flecs::world& ecs)
{
    ecs.system<PlayerInput, Action, const IsPlayer>()
        .each([&](PlayerInput& inp, Action& a, const IsPlayer)
            {
                bool left = IsKeyDown(KEY_LEFT);
                bool right = IsKeyDown(KEY_RIGHT);
                bool up = IsKeyDown(KEY_UP);
                bool down = IsKeyDown(KEY_DOWN);
                if (left && !inp.left)
                    a.action = EA_MOVE_LEFT;
                if (right && !inp.right)
                    a.action = EA_MOVE_RIGHT;
                if (up && !inp.up)
                    a.action = EA_MOVE_UP;
                if (down && !inp.down)
                    a.action = EA_MOVE_DOWN;
                inp.left = left;
                inp.right = right;
                inp.up = up;
                inp.down = down;
            });
    ecs.system<const Position, const Color>()
        .term<TextureSource>(flecs::Wildcard).not_()
        .each([&](const Position& pos, const Color color)
            {
                const Rectangle rect = { float(pos.x), float(pos.y), 1, 1 };
                DrawRectangleRec(rect, color);
            });
    ecs.system<const Position, const Color>()
        .term<TextureSource>(flecs::Wildcard)
        .each([&](flecs::entity e, const Position& pos, const Color color)
            {
                const auto textureSrc = e.target<TextureSource>();
                DrawTextureQuad(*textureSrc.get<Texture2D>(),
                    Vector2{ 1, 1 }, Vector2{ 0, 0 },
                    Rectangle{ float(pos.x), float(pos.y), 1, 1 }, color);
            });
    ecs.system<HealCooldown>()
        .each([](HealCooldown& heal)
            {
                heal.timer = (heal.timer > 0) ? heal.timer - 1 : 0;
            });
}


void init_roguelike(flecs::world& ecs)
{
    register_roguelike_systems(ecs);

    const auto offset = 5;
    add_patrol_attack_flee_sm(create_monster(ecs, 5, 5 + offset, GetColor(MAGENTA_COLOR), Tag::Enemy));
    add_patrol_attack_flee_sm(create_monster(ecs, 10, -5 + offset, GetColor(MAGENTA_COLOR), Tag::Enemy));
    add_patrol_attack_berserk_sm(create_monster(ecs, 1, 1 + offset, GetColor(RED_COLOR), Tag::Enemy));
    add_patrol_attack_berserk_sm(create_monster(ecs, 3, 9 + offset, GetColor(RED_COLOR), Tag::Enemy));
    add_patrol_flee_sm(create_monster(ecs, -5, -5 + offset, GetColor(BLACK_COLOR), Tag::Enemy));
    add_attack_sm(create_monster(ecs, -5, 5 + offset, GetColor(DARK_RED_COLOR), Tag::Enemy));
    add_patrol_self_heal_sm(create_monster(ecs, -3, -5 + offset, GetColor(WATER_LEAF_COLOR), Tag::Enemy));
    add_patrol_self_heal_sm(create_monster(ecs, -8, 9 + offset, GetColor(WATER_LEAF_COLOR), Tag::Enemy));
    add_guard_attack_player_heal_sm(create_monster(ecs, 1, 1, GetColor(MARINER_COLOR), Tag::Player));

    create_player(ecs, 0, 0);

    const auto table1 = create_craft_table(ecs, -5, -8);
    const auto table2 = create_craft_table(ecs, 5, -8);

    add_craft_wander_sleep_communicate_hsm(create_crafter(ecs, -5, -10, table1));
    add_craft_wander_sleep_communicate_hsm(create_crafter(ecs, 5, -10, table2));

    create_shop(ecs, 0, -10);

    create_powerup(ecs, 7, 7 + offset, 10.f);
    create_powerup(ecs, 3, 9 + offset, 10.f);
    create_powerup(ecs, 10, -6 + offset, 10.f);
    create_powerup(ecs, 10, -4 + offset, 10.f);

    create_heal(ecs, -5, -5 + offset, 50.f);
    create_heal(ecs, -5, 5 + offset, 50.f);
    reset_sm(ecs);
}

static bool is_player_acted(flecs::world& ecs)
{
    static auto processPlayer = ecs.query<const IsPlayer, const Action>();
    bool playerActed = false;
    processPlayer.each([&](const IsPlayer, const Action& a)
        {
            playerActed = a.action != EA_NOP;
        });
    return playerActed;
}

static bool upd_player_actions_count(flecs::world& ecs)
{
    static auto updPlayerActions = ecs.query<const IsPlayer, NumActions>();
    bool actionsReached = false;
    updPlayerActions.each([&](const IsPlayer, NumActions& na)
        {
            na.curActions = (na.curActions + 1) % na.numActions;
            actionsReached |= na.curActions == 0;
        });
    return actionsReached;
}

static Position move_pos(Position pos, int action)
{
    if (action == EA_MOVE_LEFT)
        pos.x--;
    else if (action == EA_MOVE_RIGHT)
        pos.x++;
    else if (action == EA_MOVE_UP)
        pos.y--;
    else if (action == EA_MOVE_DOWN)
        pos.y++;
    return pos;
}

static void process_actions(flecs::world& ecs)
{
    static auto processActions = ecs.query<Action, Position, MovePos, const MeleeDamage, const Team>();
    static auto checkAttacks = ecs.query<const MovePos, Hitpoints, const Team>();
    // Process all actions
    ecs.defer([&]
        {
            processActions.each([&](flecs::entity entity, Action& a, Position& pos, MovePos& mpos, const MeleeDamage& dmg, const Team& team)
                {
                    Position nextPos = move_pos(pos, a.action);
                    bool blocked = false;
                    checkAttacks.each([&](flecs::entity enemy, const MovePos& epos, Hitpoints& hp, const Team& enemy_team)
                        {
                            if (entity != enemy && epos == nextPos)
                            {
                                blocked = true;
                                if (team.tag != enemy_team.tag)
                                    hp.hitpoints -= dmg.damage;
                            }
                        });
                    if (blocked)
                        a.action = EA_NOP;
                    else
                        mpos = nextPos;
                });
            // now move
            processActions.each([&](flecs::entity entity, Action& a, Position& pos, MovePos& mpos, const MeleeDamage&, const Team&)
                {
                    pos = mpos;
                    a.action = EA_NOP;
                });
        });

    static auto deleteAllDead = ecs.query<const Hitpoints>();
    ecs.defer([&]
        {
            deleteAllDead.each([&](flecs::entity entity, const Hitpoints& hp)
                {
                    if (hp.hitpoints <= 0.f)
                        entity.destruct();
                });
        });

    static auto playerPickup = ecs.query<const IsPlayer, const Position, Hitpoints, MeleeDamage>();
    static auto healPickup = ecs.query<const Position, const HealAmount>();
    static auto powerupPickup = ecs.query<const Position, const PowerupAmount>();
    ecs.defer([&]
        {
            playerPickup.each([&](const IsPlayer&, const Position& pos, Hitpoints& hp, MeleeDamage& dmg)
                {
                    healPickup.each([&](flecs::entity entity, const Position& ppos, const HealAmount& amt)
                        {
                            if (pos == ppos)
                            {
                                hp.hitpoints += amt.amount;
                                entity.destruct();
                            }
                        });
                    powerupPickup.each([&](flecs::entity entity, const Position& ppos, const PowerupAmount& amt)
                        {
                            if (pos == ppos)
                            {
                                dmg.damage += amt.amount;
                                entity.destruct();
                            }
                        });
                });
        });
    static auto timers = ecs.query<HealCooldown>();
}

void reset_sm(flecs::world& ecs)
{
    static auto stateMachineAct = ecs.query<StateMachine>();
    static auto hStateMachineAct = ecs.query<HierarchyStateMachine>();
    ecs.defer([&]
        {
            stateMachineAct.each([&](flecs::entity e, StateMachine& sm)
                {
                    sm.reset(ecs, e);
                });
            hStateMachineAct.each([&](flecs::entity e, HierarchyStateMachine& hsm)
                {
                    hsm.reset(ecs, e);
                });
        });
}

void process_turn(flecs::world& ecs)
{
    static auto stateMachineAct = ecs.query<StateMachine>();
    static auto hStateMachineAct = ecs.query<HierarchyStateMachine>();
    if (is_player_acted(ecs))
    {
        if (upd_player_actions_count(ecs))
        {
            // Plan action for NPCs
            ecs.defer([&]
                {
                    stateMachineAct.each([&](flecs::entity e, StateMachine& sm)
                        {
                            sm.act(0.f, ecs, e);
                        });
                    hStateMachineAct.each([&](flecs::entity e, HierarchyStateMachine& hsm)
                        {
                            hsm.act(0.f, ecs, e);
                        });
                });
        }
        process_actions(ecs);
    }
}

void print_stats(flecs::world& ecs)
{
    static auto playerStatsQuery = ecs.query<const IsPlayer, const Hitpoints, const MeleeDamage>();
    playerStatsQuery.each([&](const IsPlayer&, const Hitpoints& hp, const MeleeDamage& dmg)
        {
            DrawText(TextFormat("hp: %d", int(hp.hitpoints)), 20, 20, 20, WHITE);
            DrawText(TextFormat("power: %d", int(dmg.damage)), 20, 40, 20, WHITE);
        });
}

