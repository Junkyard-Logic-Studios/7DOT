#pragma once
#include "scene.hpp"
#include "../renderer/renderer.hpp"



namespace core
{

    // character-, gamemode-, stage- selection before starting a fight
    // synchronized between clients
    class SelectionScene : public _Scene
    {
    public:
        struct State {};

        inline SelectionScene(Game& game) :
            _Scene(game)
        {}

        inline void activate() {}
        inline void deactivate() {}
        inline UpdateReturnStatus update() 
            { return SWITCH_MAINMENU; }

    private:
        std::unique_ptr<renderer::_Renderer<SelectionScene::State>> _renderer;
        State* _stateBuffer;
    };

};  // end namespace core
