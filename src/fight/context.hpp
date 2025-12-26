#pragma once
#include <vector>
#include "../sceneContext.hpp"
#include "mode.hpp"
#include "stage.hpp"



namespace fight
{

    struct Context : SceneContext
    {
        std::vector<Player> players;
        fight::Mode mode = Mode::LAST_MAN_STANDING;
        fight::Stage stage = Stage::SACRED_GROUND;
    };

};  // end namespace fight
