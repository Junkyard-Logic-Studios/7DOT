#pragma once
#include "renderer.hpp"
#include "../fight/scene.hpp"



namespace renderer
{

    class FightRenderer : public _Renderer<fight::State> 
    {};

};  // end namespace renderer
