#pragma once
#include "../syncedScene.hpp"
#include "../fightSelectionInfo.hpp"
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
        Scene(Game& game);

    protected:
        void _activate(SceneContext& context);
        UpdateReturnStatus computeFollowingState(
            const State& givenState, State& followingState, tick_t tick);

    private:
        FightSelectionInfo _fightSelection;
        Stage _stage;
        std::vector<Archer> _archers;
    };

};  // end namespace core
