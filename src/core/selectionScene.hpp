#pragma once
#include "scene.hpp"
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
        State* _stateBuffer;

        bool updateCharacterSelection(State& state, tick_t tick);
        void updateModeSelection(State& state, tick_t tick);
        bool updateStageSelection(State& state, tick_t tick);
    };

};  // end namespace core
