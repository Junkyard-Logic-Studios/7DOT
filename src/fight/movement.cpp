#include "movement.hpp"
#include <algorithm>
#include "collision.hpp"



namespace
{
    constexpr float INPUT_DEAD_ZONE = 0.35f;

    glm::vec2 quantizeInput(input::PlayerInput input)
    {
        float h = input::get::horizontalAxis(input);
        float v = input::get::verticalAxis(input);
        return glm::vec2((h > INPUT_DEAD_ZONE) - (h < -INPUT_DEAD_ZONE),
                         (v > INPUT_DEAD_ZONE) - (v < -INPUT_DEAD_ZONE));
    }

    float tickDownTimer(float time, float deltaTime)
    {
        return std::max(0.0f, time - deltaTime);
    }

    constexpr float MAX_RUN_SPEED                   =  165.0f;
    constexpr float GROUND_ACCELERATION             = 2200.0f;
    constexpr float AIR_ACCELERATION                = 1100.0f;
    constexpr float GROUND_FRICTION                 = 3000.0f;
    constexpr float AIR_FRICTION                    =  350.0f;
    constexpr float GRAVITY_ACCELERATION            = 1800.0f;
    constexpr float JUMP_HOLD_GRAVITY_ACCELERATION  =  700.0f;
    constexpr float FAST_FALL_GRAVITY_ACCELERATION  = 3000.0f;
    constexpr float MAX_FALL_SPEED                  =  440.0f;
    constexpr float FAST_FALL_MAX_SPEED             =  600.0f;
    constexpr float WALL_SLIDE_MAX_FALL_SPEED       =  125.0f;
    constexpr float JUMP_VELOCITY                   = -410.0f;
    constexpr float JUMP_CUT_VELOCITY               = -200.0f;
    constexpr float WALL_JUMP_X_VELOCITY            =  235.0f;
    constexpr float WALL_JUMP_AUTO_SPEED            =  205.0f;
    constexpr float JUMP_HOLD_TIME                  =    0.12;
    constexpr float WALL_JUMP_AUTO_MOVE_TIME        =    0.20f;

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

    bool wantsWallSlide(const fight::collision::Contacts& contacts, const glm::vec2& direction)
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
    glm::vec2 moveDirection = quantizeInput(currentInput);
    bool grounded = startingContacts.ground;
    bool holdingWall = !grounded && wantsWallSlide(startingContacts, moveDirection);

    archer.movementDirection = moveDirection;
    if (moveDirection != glm::vec2(0))
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
            AIR_ACCELERATION * deltaTime);
        archer.autoMoveTime = tickDownTimer(archer.autoMoveTime, deltaTime);
    }
    else if (archer.isCrouching)
    {
        archer.velocity.x = approach(archer.velocity.x, 0.0f, GROUND_FRICTION * deltaTime);
    }
    else if (moveDirection.x != 0)
    {
        float acceleration = grounded ? GROUND_ACCELERATION : AIR_ACCELERATION;
        archer.velocity.x = approach(
            archer.velocity.x,
            moveDirection.x * MAX_RUN_SPEED,
            acceleration * deltaTime);
    }
    else
    {
        float friction = grounded ? GROUND_FRICTION : AIR_FRICTION;
        archer.velocity.x = approach(archer.velocity.x, 0.0f, friction * deltaTime);
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

    archer.velocity.y = std::min(archer.velocity.y + gravity * deltaTime, maxFallSpeed);
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
