#pragma once

#include "stateMachine.h"
#include "ecsTypes.h"

// states
State* create_attack_enemy_state();
State* create_move_to_enemy_state();
State* create_flee_from_enemy_state();
State* create_patrol_state(float patrol_dist);
State* create_self_heal_state(HealAmount heal);
State* create_guard_state(float guard_dist);
State* create_player_heal_state(HealAmount heal, int cooldown);
State* create_wander_state(int time);
State* create_craft_state();
State* create_shop_state(int time);
State* create_sleep_state(int time);
State* create_move_to_craft_state();
State* create_move_to_shop_state();
State* create_move_to_communicate_state();
State* create_communicate_state(int time);
State* create_nop_state();

// transitions
StateTransition* create_enemy_available_transition(float dist);
StateTransition* create_enemy_reachable_transition();
StateTransition* create_hitpoints_less_than_transition(float thres);
StateTransition* create_player_hitpoints_less_than_transition(float thres);
StateTransition* create_negate_transition(StateTransition* in);
StateTransition* create_and_transition(StateTransition* lhs, StateTransition* rhs);
StateTransition* create_wander_timer_transition();
StateTransition* create_sleep_timer_transition();
StateTransition* create_communicate_timer_transition();
StateTransition* create_shop_timer_transition();
StateTransition* create_communicate_transition();
StateTransition* create_shop_transition();
StateTransition* create_craft_transition();
StateTransition* create_move_to_shop_transition(int thres);
StateTransition* create_craft_threshold_transition(int thres);
