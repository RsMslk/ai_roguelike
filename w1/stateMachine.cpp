#include "stateMachine.h"

void StateMachine::enter(flecs::world& ecs, flecs::entity entity)
{
    curStateIdx = 0;
    if (states.size() > 0)
        states[curStateIdx]->enter(ecs, entity);
}

void StateMachine::exit(flecs::world& ecs, flecs::entity entity)
{
    states[curStateIdx]->exit(ecs, entity);
}

void StateMachine::act(float dt, flecs::world& ecs, flecs::entity entity)
{
    FiniteStateMachine::act(dt, ecs, entity);
}
