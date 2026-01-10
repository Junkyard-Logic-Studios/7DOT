#pragma once
#include <vector>
#include "../syncedScene.hpp"
#include "../renderer/renderer.hpp"
#include "../player.hpp"
#include "mode.hpp"
#include "stage.hpp"
#include "level.hpp"
#include "archer.hpp"



namespace fight
{

    struct State
    {
        uint8_t levelIndex = 0;
        std::vector<Archer> archers;
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
        Mode getMode() const;
        Stage getStage() const;
        const Level& getLevel(std::size_t index) const;

    protected:
        void _activate(SceneContext& context, State& startState);
        void _deactivate();
        UpdateReturnStatus computeFollowingState(
            const State& givenState, State& followingState, tick_t tick);

    private:
        std::vector<Player> _players;
        fight::Mode _mode;
        fight::Stage _stage;
        std::vector<Level> _levels;

        void _initNewLevel(State& state);
    };

};  // end namespace core
