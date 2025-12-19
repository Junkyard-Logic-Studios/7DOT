#pragma once
#include <vector>
#include "constants.hpp"



struct SceneContext 
{
    tick_t startTime;
    std::vector<hostID_t> knownHosts = { 0 };
};
