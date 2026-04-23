#include "movement.hpp"
#include <algorithm>
#include "collision.hpp"



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
        archer.movementState = ArcherMovementState::DEAD;
    else if (endingContacts.ground)
        archer.movementState = ArcherMovementState::GROUNDED;
    else if (archer.autoMoveTime <= 0.0f
        && archer.velocity.y >= 0.0f
        && wantsWallSlide(endingContacts, moveDirection))
    {
        archer.velocity.y = std::min(archer.velocity.y, WALL_SLIDE_MAX_FALL_SPEED);
        archer.movementState = ArcherMovementState::WALL_SLIDING;
    }
    else
        archer.movementState = ArcherMovementState::AIRBORNE;
}
