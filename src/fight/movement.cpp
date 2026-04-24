#include "movement.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include "collision.hpp"
#include "../constants.hpp"



namespace
{
    constexpr float INPUT_DEAD_ZONE = 0.35f;
    constexpr float TUNING_TICK_SECONDS = 0.01f;

    constexpr float tunedTicksToSeconds(float ticks)
    {
        return ticks * TUNING_TICK_SECONDS;
    }

    constexpr float tunedVelocity(float pixelsPerTuningTick)
    {
        return pixelsPerTuningTick / TUNING_TICK_SECONDS;
    }

    constexpr float tunedAcceleration(float pixelsPerTuningTickPerTuningTick)
    {
        return pixelsPerTuningTickPerTuningTick
            / (TUNING_TICK_SECONDS * TUNING_TICK_SECONDS);
    }

    float velocityStep(float acceleration, float deltaTime)
    {
        return acceleration * deltaTime;
    }

    float tickDownTimer(float time, float deltaTime)
    {
        constexpr float TIMER_EPSILON = 0.000001f;
        time = std::max(0.0f, time - deltaTime);
        return time < TIMER_EPSILON ? 0.0f : time;
    }

    constexpr float MAX_RUN_SPEED = tunedVelocity(1.65f);
    constexpr float GROUND_ACCELERATION = tunedAcceleration(0.22f);
    constexpr float AIR_ACCELERATION = tunedAcceleration(0.11f);
    constexpr float GROUND_FRICTION = tunedAcceleration(0.30f);
    constexpr float AIR_FRICTION = tunedAcceleration(0.035f);
    constexpr float GRAVITY_ACCELERATION = tunedAcceleration(0.18f);
    constexpr float JUMP_HOLD_GRAVITY_ACCELERATION = tunedAcceleration(0.07f);
    constexpr float FAST_FALL_GRAVITY_ACCELERATION = tunedAcceleration(0.30f);
    constexpr float MAX_FALL_SPEED = tunedVelocity(4.4f);
    constexpr float FAST_FALL_MAX_SPEED = tunedVelocity(6.0f);
    constexpr float WALL_SLIDE_MAX_FALL_SPEED = tunedVelocity(1.25f);
    constexpr float JUMP_VELOCITY = tunedVelocity(-4.1f);
    constexpr float JUMP_CUT_VELOCITY = tunedVelocity(-2.0f);
    constexpr float WALL_JUMP_X_VELOCITY = tunedVelocity(2.35f);
    constexpr float WALL_JUMP_AUTO_SPEED = tunedVelocity(2.05f);
    constexpr float JUMP_HOLD_TIME = tunedTicksToSeconds(12.0f);
    constexpr float WALL_JUMP_AUTO_MOVE_TIME = tunedTicksToSeconds(20.0f);

    int quantizeAxis(float value)
    {
        if (value > INPUT_DEAD_ZONE)
            return 1;
        if (value < -INPUT_DEAD_ZONE)
            return -1;
        return 0;
    }

    glm::ivec2 quantizeInput(input::PlayerInput input)
    {
        return glm::ivec2(
            quantizeAxis(input::get::horizontalAxis(input)),
            quantizeAxis(input::get::verticalAxis(input)));
    }

    float approach(float current, float target, float amount)
    {
        if (current < target)
            return std::min(current + amount, target);
        if (current > target)
            return std::max(current - amount, target);
        return current;
    }

    int wallJumpDirection(const fight::collision::Contacts& contacts)
    {
        if (contacts.leftWall && !contacts.rightWall)
            return 1;
        if (contacts.rightWall && !contacts.leftWall)
            return -1;
        return 0;
    }

    bool wantsWallSlide(const fight::collision::Contacts& contacts, const glm::ivec2& direction)
    {
        return (contacts.leftWall && direction.x < 0)
            || (contacts.rightWall && direction.x > 0);
    }

    int heldWallDirection(const fight::collision::Contacts& contacts, const glm::ivec2& direction)
    {
        if (contacts.leftWall && direction.x < 0)
            return -1;
        if (contacts.rightWall && direction.x > 0)
            return 1;
        return 0;
    }

    bool holdingGrabDirection(const fight::Archer& archer, const glm::ivec2& direction)
    {
        return (archer.wallGrabDirection < 0 && direction.x < 0)
            || (archer.wallGrabDirection > 0 && direction.x > 0);
    }

    int floorToTile(float world)
    {
        return static_cast<int>(std::floor(world / float(TILESIZE)));
    }

    std::size_t wrapTileIndex(int index, std::size_t size)
    {
        int wrapped = index % int(size);
        return std::size_t(wrapped < 0 ? wrapped + int(size) : wrapped);
    }

    bool tileIndexForAxis(int index, std::size_t size, bool wraps, std::size_t& result)
    {
        if (wraps)
        {
            result = wrapTileIndex(index, size);
            return true;
        }

        if (index < 0 || index >= int(size))
            return false;

        result = std::size_t(index);
        return true;
    }

    bool isSolidRepeatedAt(const fight::Level& level, int x, int y)
    {
        std::size_t tileX = 0;
        std::size_t tileY = 0;
        if (!tileIndexForAxis(x, level.getWidth(), level.wrapsHorizontally(), tileX)
            || !tileIndexForAxis(y, level.getHeight(), level.wrapsVertically(), tileY))
            return false;

        return level.getSolidAt(tileX, tileY) != -1;
    }

    bool snapToWallGrabCorner(const fight::Level& level, fight::Archer& archer, int wallDirection)
    {
        if (wallDirection == 0)
            return false;

        fight::Archer::Hitbox body = archer.bodyHitbox();
        int wallTileX = wallDirection < 0
            ? floorToTile(body.tl.x - 0.1f)
            : floorToTile(body.br.x + 0.1f);
        int minTileY = floorToTile(body.tl.y);
        int maxTileY = floorToTile(body.br.y - 0.001f);

        for (int y = minTileY; y <= maxTileY; y++)
        {
            if (!isSolidRepeatedAt(level, wallTileX, y)
                || isSolidRepeatedAt(level, wallTileX, y - 1))
                continue;

            float tileTop = float(y * TILESIZE);
            archer.position.x = wallDirection < 0
                ? float((wallTileX + 1) * TILESIZE) + fight::Archer::WIDTH / 2.0f
                : float(wallTileX * TILESIZE) - fight::Archer::WIDTH / 2.0f;
            archer.position.y = tileTop
                - fight::Archer::HEIGHT / 2.0f
                + archer.hitboxHeight()
                - fight::Archer::HEAD_HEIGHT;
            return true;
        }

        return false;
    }

    void startLedgeJump(fight::Archer& archer, int jumpDirection)
    {
        archer.velocity.y = JUMP_VELOCITY;
        archer.velocity.x = jumpDirection * WALL_JUMP_X_VELOCITY;
        archer.jumpHoldTime = JUMP_HOLD_TIME;
        archer.autoMoveTime = WALL_JUMP_AUTO_MOVE_TIME;
        archer.autoMoveDirection = jumpDirection;
        archer.wallGrabDirection = 0;
        archer.isFacingRight = jumpDirection > 0;
        archer.movementState = fight::ArcherMovementState::AIRBORNE;
    }
}


void fight::movement::stepArcher(
    const Level& level,
    Archer& archer,
    input::PlayerInput currentInput,
    input::PlayerInput justPressed,
    input::PlayerInput justReleased,
    float deltaTime)
{
    auto startingContacts = collision::contactsAt(level, archer);
    glm::ivec2 moveDirection = quantizeInput(currentInput);
    bool grounded = startingContacts.ground;
    bool holdingWall = !grounded && wantsWallSlide(startingContacts, moveDirection);

    archer.movementDirection = moveDirection;
    if (moveDirection != glm::ivec2(0))
        archer.aimDirection = moveDirection;

    bool wantsCrouch = grounded && moveDirection.y > 0 && moveDirection.x == 0;
    if (!wantsCrouch && archer.isCrouching)
    {
        Archer standProbe = archer;
        standProbe.isCrouching = false;
        wantsCrouch = collision::overlapsSolid(level, standProbe);
    }
    archer.isCrouching = wantsCrouch;

    if (moveDirection.x != 0)
        archer.isFacingRight = moveDirection.x > 0;

    if (archer.movementState == ArcherMovementState::LEDGE_CLINGING)
    {
        bool stillHoldingGrab = holdingGrabDirection(archer, moveDirection);
        bool stillAtCorner = snapToWallGrabCorner(level, archer, archer.wallGrabDirection);
        if (archer.isAlive
            && stillAtCorner
            && input::get::jump(justPressed)
            && moveDirection.x != 0)
        {
            startLedgeJump(archer, moveDirection.x);
            return;
        }

        if (archer.isAlive && stillHoldingGrab && stillAtCorner)
        {
            archer.velocity = glm::vec2(0.0f);
            archer.jumpHoldTime = 0.0f;
            archer.autoMoveTime = 0.0f;
            archer.autoMoveDirection = 0;
            return;
        }

        archer.wallGrabDirection = 0;
        archer.movementState = ArcherMovementState::AIRBORNE;
    }

    if (input::get::jump(justPressed))
    {
        int awayFromWall = wallJumpDirection(startingContacts);
        if (grounded)
        {
            archer.velocity.y = JUMP_VELOCITY;
            archer.jumpHoldTime = JUMP_HOLD_TIME;
            archer.movementState = ArcherMovementState::AIRBORNE;
            Archer standProbe = archer;
            standProbe.isCrouching = false;
            archer.isCrouching = collision::overlapsSolid(level, standProbe);
        }
        else if (awayFromWall != 0)
        {
            archer.velocity.y = JUMP_VELOCITY;
            archer.velocity.x = awayFromWall * WALL_JUMP_X_VELOCITY;
            archer.jumpHoldTime = JUMP_HOLD_TIME;
            archer.autoMoveTime = WALL_JUMP_AUTO_MOVE_TIME;
            archer.autoMoveDirection = awayFromWall;
            archer.isFacingRight = awayFromWall > 0;
        }
    }

    if (input::get::jump(justReleased) && archer.velocity.y < JUMP_CUT_VELOCITY)
    {
        archer.velocity.y = JUMP_CUT_VELOCITY;
        archer.jumpHoldTime = 0.0f;
    }

    if (archer.autoMoveTime > 0.0f)
    {
        archer.velocity.x = approach(
            archer.velocity.x,
            archer.autoMoveDirection * WALL_JUMP_AUTO_SPEED,
            velocityStep(AIR_ACCELERATION, deltaTime));
        archer.autoMoveTime = tickDownTimer(archer.autoMoveTime, deltaTime);
    }
    else if (archer.isCrouching)
    {
        archer.velocity.x = approach(archer.velocity.x, 0.0f, velocityStep(GROUND_FRICTION, deltaTime));
    }
    else if (moveDirection.x != 0)
    {
        float acceleration = grounded ? GROUND_ACCELERATION : AIR_ACCELERATION;
        archer.velocity.x = approach(
            archer.velocity.x,
            moveDirection.x * MAX_RUN_SPEED,
            velocityStep(acceleration, deltaTime));
    }
    else
    {
        float friction = grounded ? GROUND_FRICTION : AIR_FRICTION;
        archer.velocity.x = approach(archer.velocity.x, 0.0f, velocityStep(friction, deltaTime));
    }

    float maxFallSpeed = MAX_FALL_SPEED;
    float gravity = GRAVITY_ACCELERATION;
    if (holdingWall && archer.velocity.y > 0.0f && archer.autoMoveTime <= 0.0f)
    {
        maxFallSpeed = WALL_SLIDE_MAX_FALL_SPEED;
        gravity = JUMP_HOLD_GRAVITY_ACCELERATION;
    }
    else if (!grounded && moveDirection.y > 0 && archer.velocity.y > 0.0f)
    {
        maxFallSpeed = FAST_FALL_MAX_SPEED;
        gravity = FAST_FALL_GRAVITY_ACCELERATION;
    }
    else if (input::get::jump(currentInput) && archer.jumpHoldTime > 0.0f && archer.velocity.y < 0.0f)
    {
        gravity = JUMP_HOLD_GRAVITY_ACCELERATION;
    }

    archer.velocity.y = std::min(archer.velocity.y + velocityStep(gravity, deltaTime), maxFallSpeed);
    if (!input::get::jump(currentInput) || archer.velocity.y >= 0.0f)
        archer.jumpHoldTime = 0.0f;
    else if (archer.jumpHoldTime > 0.0f)
        archer.jumpHoldTime = tickDownTimer(archer.jumpHoldTime, deltaTime);

    auto endingContacts = collision::moveAndCollide(level, archer, deltaTime);
    if (endingContacts.ground || endingContacts.ceiling)
        archer.jumpHoldTime = 0.0f;
    if (endingContacts.ground)
        archer.autoMoveTime = 0.0f;

    if (!archer.isAlive)
    {
        archer.wallGrabDirection = 0;
        archer.movementState = ArcherMovementState::DEAD;
    }
    else if (endingContacts.ground)
    {
        archer.wallGrabDirection = 0;
        archer.movementState = ArcherMovementState::GROUNDED;
    }
    else if (archer.autoMoveTime <= 0.0f
        && archer.velocity.y >= 0.0f
        && wantsWallSlide(endingContacts, moveDirection))
    {
        int grabDirection = heldWallDirection(endingContacts, moveDirection);
        if (snapToWallGrabCorner(level, archer, grabDirection))
        {
            archer.velocity = glm::vec2(0.0f);
            archer.wallGrabDirection = grabDirection;
            archer.movementState = ArcherMovementState::LEDGE_CLINGING;
        }
        else
        {
            archer.velocity.y = std::min(archer.velocity.y, WALL_SLIDE_MAX_FALL_SPEED);
            archer.wallGrabDirection = 0;
            archer.movementState = ArcherMovementState::WALL_SLIDING;
        }
    }
    else
    {
        archer.wallGrabDirection = 0;
        archer.movementState = ArcherMovementState::AIRBORNE;
    }
}
