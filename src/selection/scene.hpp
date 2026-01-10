#pragma once
#include "../syncedScene.hpp"
#include "../fight/mode.hpp"
#include "../fight/stage.hpp"



namespace selection
{

    enum NavigationOptions : uint32_t
    {
        CHARACTERS,
        MODE,
        TEAM,
        STAGE,
    };

    struct State 
    {
        std::vector<Player> players;
        fight::Mode mode = fight::Mode::LAST_MAN_STANDING;
        fight::Stage stage = fight::Stage::SACRED_GROUND;

        NavigationOptions currentLevel = CHARACTERS;
    };

    // character-, gamemode-, stage- selection before starting a fight
    // synchronized between clients
    class Scene : public _SyncedScene<State>
    {
    public:
        Scene(Game& game);

    protected:
        void _activate(SceneContext& context, State& startState);
        UpdateReturnStatus computeFollowingState(
            const State& givenState, State& followingState, tick_t tick);

    private:
        std::vector<hostID_t> _knownHosts;

        bool updateCharacterSelection(const State& cstate, State& nstate, tick_t tick);
        void updateModeSelection(const State& cstate, State& nstate, tick_t tick);
        void updateTeamSelection(const State& cstate, State& nstate, tick_t tick);
        bool updateStageSelection(const State& cstate, State& nstate, tick_t tick);
    };

};  // end namespace selection
