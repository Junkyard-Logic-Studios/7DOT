#pragma once
#include <array>
#include "scene.hpp"
#include "sceneStateBuffer.hpp"
#include "../renderer/renderer.hpp"
#include "stage.hpp"
#include "archer.hpp"



namespace core
{

    // actual fight of archers on selected stage
    // synchronized between clients
    class FightScene : public _Scene
    {
    public:
        struct State
        {
            Stage::State stage;
            Archer::State archers[64];
            // arrows
            // chests
            // special effects
            // ...
        };

        inline FightScene(Game& game) :
            _Scene(game) 
        {}

        inline void activate() {}
        inline void deactivate() {}
        inline UpdateReturnStatus update() 
            { return SWITCH_SELECTION; }

    private:
        std::unique_ptr<renderer::_Renderer<FightScene::State>> _renderer;
        SceneStateBuffer<State> _stateBuffer;

        Stage _stage;
        std::vector<Archer> _archers;
    };

};  // end namespace core
