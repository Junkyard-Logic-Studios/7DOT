#pragma once
#include <vector>
#include "player.hpp"


namespace core
{

    struct FightSelection
    {

        std::vector<Player> players;

        enum class Mode
        {
            LAST_MAN_STANDING,
            HEAD_HUNTERS,
            TEAM,
        } 
        mode = Mode::LAST_MAN_STANDING;

        enum class Stage
        {
            TOWER,
            CAVE,
            CASTLE,
        }
        stage = Stage::TOWER;
    };

};  // end namespace core
