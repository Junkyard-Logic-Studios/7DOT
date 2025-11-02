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

        inline SelectionScene(std::shared_ptr<Game> game) :
            _Scene(game)
        {}

    private:
        std::unique_ptr<renderer::_Renderer<SelectionScene::State>> _renderer;
        State* _stateBuffer;
    };

};  // end namespace core
