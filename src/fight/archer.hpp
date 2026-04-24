#pragma once
#include <inttypes.h>
#include <algorithm>
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
        struct Hitbox
        {
            glm::vec2 tl;
            glm::vec2 br;
        };

        glm::vec2 position;
        glm::vec2 velocity;
        glm::ivec2 movementDirection = glm::ivec2(0);
        glm::ivec2 aimDirection = glm::ivec2(1, 0);
        ArcherMovementState movementState = ArcherMovementState::AIRBORNE;
        float jumpHoldTime = 0.0f;
        float autoMoveTime = 0.0f;
        int8_t autoMoveDirection = 0;
        int8_t wallGrabDirection = 0;

        uint32_t isFacingRight : 1;
        uint32_t isAlive : 1;
        uint32_t isCrouching : 1;
        // ...

        static constexpr auto HEIGHT = 18;
        static constexpr auto CROUCH_HEIGHT = 11;
        static constexpr auto WIDTH = 10;
        static constexpr auto HEAD_HEIGHT = 7;

        inline float hitboxHeight() const
            { return isCrouching ? CROUCH_HEIGHT : HEIGHT; }

        inline glm::vec2 hitboxTL() const
        {
            glm::vec2 br = hitboxBR();
            return glm::vec2(br.x - WIDTH, br.y - hitboxHeight());
        }

        inline glm::vec2 hitboxBR() const
            { return position + .5f * glm::vec2(WIDTH, HEIGHT); }

        inline Hitbox fullHitbox() const
            { return { hitboxTL(), hitboxBR() }; }

        inline Hitbox headHitbox() const
        {
            glm::vec2 tl = hitboxTL();
            glm::vec2 br = hitboxBR();
            float headHeight = std::min(float(HEAD_HEIGHT), hitboxHeight());
            return { tl, glm::vec2(br.x, tl.y + headHeight) };
        }

        inline Hitbox bodyHitbox() const
        {
            glm::vec2 tl = hitboxTL();
            glm::vec2 br = hitboxBR();
            float headHeight = std::min(float(HEAD_HEIGHT), hitboxHeight());
            return { glm::vec2(tl.x, tl.y + headHeight), br };
        }
    };

};  // end namespace fight
