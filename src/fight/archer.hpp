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

        static constexpr auto HEIGHT = 18;
        static constexpr auto WIDTH = 10;

        inline glm::vec2 hitboxTL() const
            { return position - .5f * glm::vec2(WIDTH, HEIGHT); }

        inline glm::vec2 hitboxBR() const
            { return position + .5f * glm::vec2(WIDTH, HEIGHT); }
    };

};  // end namespace core
