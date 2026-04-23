#pragma once
#include <inttypes.h>
#include "glm/vec2.hpp"



namespace fight
{
    enum class ArcherMovementState : uint8_t
    {
        GROUNDED,
        AIRBORNE,
        WALL_SLIDING,
        LEDGE_CLINGING,
        DODGING,
        DEAD
    };

    struct Archer 
    {
        glm::vec2 position;
        glm::vec2 velocity;
        glm::ivec2 movementDirection = glm::ivec2(0);
        glm::ivec2 aimDirection = glm::ivec2(1, 0);
        ArcherMovementState movementState = ArcherMovementState::AIRBORNE;
        float jumpHoldTime = 0.0f;
        float autoMoveTime = 0.0f;
        int8_t autoMoveDirection = 0;

        uint32_t isFacingRight : 1;
        uint32_t isAlive : 1;
        uint32_t isCrouching : 1;
        // ...

        static constexpr auto HEIGHT = 18;
        static constexpr auto CROUCH_HEIGHT = 11;
        static constexpr auto WIDTH = 10;

        inline float hitboxHeight() const
            { return isCrouching ? CROUCH_HEIGHT : HEIGHT; }

        inline glm::vec2 hitboxTL() const
        {
            glm::vec2 br = hitboxBR();
            return glm::vec2(br.x - WIDTH, br.y - hitboxHeight());
        }

        inline glm::vec2 hitboxBR() const
            { return position + .5f * glm::vec2(WIDTH, HEIGHT); }
    };

};  // end namespace fight
