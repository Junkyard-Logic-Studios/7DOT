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
            TEAM_2,
            TEAM_4,
            MAX_ENUM
        } 
        mode = Mode::LAST_MAN_STANDING;

        enum class Stage
        {
            TOWER,
            CAVE,
            CASTLE,
            MAX_ENUM
        }
        stage = Stage::TOWER;
    };

};  // end namespace core
