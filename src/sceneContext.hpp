#pragma once
#include <vector>
#include "constants.hpp"
#include "player.hpp"
#include "fight/mode.hpp"
#include "fight/stage.hpp"


struct SceneContext 
{
    tick_t startTime;
    std::vector<hostID_t> knownHosts = { 0 };
    std::vector<Player> players;
    fight::Mode mode = fight::Mode::LAST_MAN_STANDING;
    fight::Stage stage = fight::Stage::SACRED_GROUND;
};
