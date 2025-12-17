#pragma once
#include "../syncedScene.hpp"
#include "../fightSelectionInfo.hpp"



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
        FightSelectionInfo fightSelection;
        NavigationOptions currentLevel = CHARACTERS;
    };

    // character-, gamemode-, stage- selection before starting a fight
    // synchronized between clients
    class Scene : public _SyncedScene<State>
    {
    public:
        Scene(Game& game);

    protected:
        UpdateReturnStatus computeFollowingState(
            const State& givenState, State& followingState, tick_t tick);

    private:
        bool updateCharacterSelection(const State& cstate, State& nstate, tick_t tick);
        void updateModeSelection(const State& cstate, State& nstate, tick_t tick);
        void updateTeamSelection(const State& cstate, State& nstate, tick_t tick);
        bool updateStageSelection(const State& cstate, State& nstate, tick_t tick);
    };

};  // end namespace selection
