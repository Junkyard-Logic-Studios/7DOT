#pragma once
#include <inttypes.h>
#include "glm/vec2.hpp"



namespace fight
{

    struct Archer 
    {
        glm::vec2 position;
        glm::vec2 velocity;

        uint32_t isFacingRight : 1;
        uint32_t isAlive : 1;
        uint32_t isCrouching : 1;
        // ...
    };

};  // end namespace core
