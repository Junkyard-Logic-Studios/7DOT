#pragma once
#include <vector>
#include "../syncedScene.hpp"
#include "../fightSelectionInfo.hpp"
#include "../renderer/renderer.hpp"
#include "level.hpp"
#include "archer.hpp"



namespace fight
{

    struct State
    {
        uint8_t levelIndex = 0;
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
        const Level& getLevel(std::size_t index) const;

    protected:
        void _activate(SceneContext& context);
        void _deactivate();
        UpdateReturnStatus computeFollowingState(
            const State& givenState, State& followingState, tick_t tick);

    private:
        FightSelectionInfo _fightSelection;
        std::vector<Level> _levels;
        std::vector<Archer> _archers;
    };

};  // end namespace core
