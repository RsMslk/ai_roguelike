#pragma once

struct Position;
struct MovePos;

struct MovePos
{
	int x = 0;
	int y = 0;

	MovePos& operator=(const Position& rhs);
};

struct Position
{
	int x = 0;
	int y = 0;

	Position& operator=(const MovePos& rhs);
};

inline Position& Position::operator=(const MovePos& rhs)
{
	x = rhs.x;
	y = rhs.y;
	return *this;
}

inline MovePos& MovePos::operator=(const Position& rhs)
{
	x = rhs.x;
	y = rhs.y;
	return *this;
}

inline bool operator==(const Position& lhs, const Position& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
inline bool operator==(const Position& lhs, const MovePos& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
inline bool operator==(const MovePos& lhs, const MovePos& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
inline bool operator==(const MovePos& lhs, const Position& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }

inline constexpr int INVALID = -1;

struct PatrolPos
{
	int x = 0;
	int y = 0;
};

struct Hitpoints
{
	float hitpoints = 10.f;
};

enum class Tag
{
	Player = 0,
	Enemy = 1,
};

enum Actions
{
	EA_NOP = 0,
	EA_MOVE_START,
	EA_MOVE_LEFT = EA_MOVE_START,
	EA_MOVE_RIGHT,
	EA_MOVE_DOWN,
	EA_MOVE_UP,
	EA_MOVE_END,
	EA_ATTACK = EA_MOVE_END,
	EA_NUM
};

struct Action
{
	int action = 0;
};

struct NumActions
{
	int numActions = 1;
	int curActions = 0;
};

struct MeleeDamage
{
	float damage = 2.f;
};

struct HealAmount
{
	float amount = 0.f;
};

struct HealCooldown
{
	int timer = 0;
};

struct WanderTimer
{
	int count = INVALID;
};

struct SleepTimer
{
	int count = INVALID;
};

struct CommunicateTimer
{
	int count = INVALID;
};

struct ShopTimer
{
	int count = INVALID;
};

struct Craft
{
	int count = INVALID;
};

struct PowerupAmount
{
	float amount = 0.f;
};

struct PlayerInput
{
	bool left = false;
	bool right = false;
	bool up = false;
	bool down = false;
};

struct Symbol
{
	char symb;
};

struct IsPlayer {};
struct IsCrafter {};
struct IsCraftTable {};
struct IsShop {};

struct Team
{
	Tag tag{};
};

struct CraftTable
{
	flecs::entity entity;
};

struct TextureSource {};

