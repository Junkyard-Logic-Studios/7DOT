#pragma once
#include <vector>
#include "../sceneContext.hpp"
#include "../fightSelectionInfo.hpp"



namespace fight
{

    struct Context : SceneContext
    {
        FightSelectionInfo fightSelection;
    };

};  // end namespace fight
