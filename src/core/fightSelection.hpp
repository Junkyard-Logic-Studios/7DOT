#pragma once
#include <vector>



namespace core
{

    struct FightSelection
    {
        struct Player {};
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
