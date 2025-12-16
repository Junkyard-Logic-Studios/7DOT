#pragma once
#include <array>
#include "scene.hpp"
#include "sceneStateBuffer.hpp"
#include "../constants.hpp"
#include "../renderer/renderer.hpp"
#include "fightSelection.hpp"



namespace core
{

    // character-, gamemode-, stage- selection before starting a fight
    // synchronized between clients
    class SelectionScene : public _Scene
    {
    public:
        enum NavigationOptions : uint32_t
        {
            CHARACTERS,
            MODE,
            TEAM,
            STAGE,
        };

        struct State 
        {
            FightSelection fightSelection;
            NavigationOptions currentLevel = CHARACTERS;
        };

        SelectionScene(Game& game);
        void activate();
        void deactivate();
        UpdateReturnStatus update();

    private:
        std::unique_ptr<renderer::_Renderer<SelectionScene::State>> _renderer;
        SceneStateBuffer<State> _stateBuffer;

        bool updateCharacterSelection(const State& cstate, State& nstate, tick_t tick);
        void updateModeSelection(const State& cstate, State& nstate, tick_t tick);
        void updateTeamSelection(const State& cstate, State& nstate, tick_t tick);
        bool updateStageSelection(const State& cstate, State& nstate, tick_t tick);
    };

};  // end namespace core
