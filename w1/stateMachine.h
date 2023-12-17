#pragma once
#include <vector>
#include <flecs.h>

class State
{
public:
    virtual void enter(flecs::world& ecs, flecs::entity entity) = 0;
    virtual void exit(flecs::world& ecs, flecs::entity entity) = 0;
    virtual void act(float dt, flecs::world& ecs, flecs::entity entity) = 0;
};

class StateTransition
{
public:
    virtual ~StateTransition() {}
    virtual bool isAvailable(flecs::world& ecs, flecs::entity entity) const = 0;
};

template<typename State>
class FiniteStateMachine
{
protected:
    int curStateIdx = 0;
    std::vector<State*> states;
    std::vector<std::vector<std::pair<StateTransition*, int>>> transitions;
public:
    FiniteStateMachine() = default;
    FiniteStateMachine(const FiniteStateMachine& sm) = default;
    FiniteStateMachine(FiniteStateMachine&& sm) = default;

    ~FiniteStateMachine()
    {
        for (State* state : states)
            delete state;
        states.clear();
        for (auto& transList : transitions)
            for (auto& transition : transList)
                delete transition.first;
        transitions.clear();
    }

    FiniteStateMachine& operator=(const FiniteStateMachine& sm) = default;
    FiniteStateMachine& operator=(FiniteStateMachine&& sm) = default;

    void act(float dt, flecs::world& ecs, flecs::entity entity)
    {
        if (curStateIdx < states.size())
        {
            for (const std::pair<StateTransition*, int>& transition : transitions[curStateIdx])
                if (transition.first->isAvailable(ecs, entity))
                {
                    states[curStateIdx]->exit(ecs, entity);
                    curStateIdx = transition.second;
                    states[curStateIdx]->enter(ecs, entity);
                    break;
                }
            states[curStateIdx]->act(dt, ecs, entity);
        }
        else
            curStateIdx = 0;
    }

    void reset(flecs::world& ecs, flecs::entity entity)
    {
        curStateIdx = 0;
        states[curStateIdx]->enter(ecs, entity);
    }

    int addState(State* st)
    {
        int idx = states.size();
        states.push_back(st);
        transitions.push_back(std::vector<std::pair<StateTransition*, int>>());
        return idx;
    }

    void addTransition(StateTransition* trans, int from, int to)
    {
        transitions[from].push_back(std::make_pair(trans, to));
    }
};


class StateMachine : public State, public FiniteStateMachine<State>
{
public:
    void enter(flecs::world& ecs, flecs::entity entity) override;
    void exit(flecs::world& ecs, flecs::entity entity) override;
    void act(float dt, flecs::world& ecs, flecs::entity entity) override;
};

using HierarchyStateMachine = FiniteStateMachine<StateMachine>;
