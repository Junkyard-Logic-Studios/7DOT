#pragma once
#include <vector>
#include "player.hpp"



struct FightSelectionInfo
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
        SACRED_GROUND,
        TWILIGHT_SPIRE,
        BACKFIRE,
        FLIGHT,
        MIRAGE,
        THORNWOOD,
        FROSTFANG_KEEP,
        KINGS_COURT,
        SUNKEN_CITY,
        MOONSTONE,
        TOWERFORGE,
        ASCENSION,
        MAX_ENUM
    }
    stage = Stage::SACRED_GROUND;
};
