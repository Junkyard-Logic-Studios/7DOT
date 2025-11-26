#pragma once
#include "renderer.hpp"
#include "../core/fightScene.hpp"



namespace renderer
{

    class FightRenderer : public _Renderer<core::FightScene::State> 
    {};

};  // end namespace renderer
