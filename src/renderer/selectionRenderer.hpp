#pragma once
#include "renderer.hpp"
#include "../core/selectionScene.hpp"



namespace renderer
{

    class SelectionRenderer : public _Renderer<core::SelectionScene::State> 
    {};

};  // end namespace renderer
