#pragma once
#include <array>
#include "../syncedScene.hpp"
#include "../renderer/renderer.hpp"
#include "stage.hpp"
#include "archer.hpp"



namespace fight
{

    struct State
    {
        Stage::State stage;
        Archer::State archers[64];
        // arrows
        // chests
        // special effects
        // ...
    };

    // actual fight of archers on selected stage
    // synchronized between clients
    class Scene : public _SyncedScene<State>
    {
    public:
        inline Scene(Game& game) :
            _SyncedScene(game) 
        {}

    protected:
        inline UpdateReturnStatus computeFollowingState(
            const State& givenState, State& followingState, tick_t tick)
            { return UpdateReturnStatus::SWITCH_SELECTION; }

    private:
        Stage _stage;
        std::vector<Archer> _archers;
    };

};  // end namespace core
