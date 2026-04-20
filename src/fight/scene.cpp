#include <filesystem>
#include <algorithm>
#include <cmath>
#include "scene.hpp"
#include "context.hpp"
#include "collision.hpp"
#include "../renderer/fightRenderer.hpp"
#include "../game.hpp"


namespace
{
    constexpr float INPUT_DEAD_ZONE = 0.35f;
    constexpr float MAX_RUN_SPEED = 1.65f;
    constexpr float GROUND_ACCELERATION = 0.22f;
    constexpr float AIR_ACCELERATION = 0.11f;
    constexpr float GROUND_FRICTION = 0.30f;
    constexpr float AIR_FRICTION = 0.035f;
    constexpr float GRAVITY_ACCELERATION = 0.18f;
    constexpr float JUMP_HOLD_GRAVITY_ACCELERATION = 0.07f;
    constexpr float FAST_FALL_GRAVITY_ACCELERATION = 0.30f;
    constexpr float MAX_FALL_SPEED = 4.4f;
    constexpr float FAST_FALL_MAX_SPEED = 6.0f;
    constexpr float WALL_SLIDE_MAX_FALL_SPEED = 1.25f;
    constexpr float JUMP_VELOCITY = -4.1f;
    constexpr float JUMP_CUT_VELOCITY = -2.0f;
    constexpr float WALL_JUMP_X_VELOCITY = 2.35f;
    constexpr float WALL_JUMP_AUTO_SPEED = 2.05f;
    constexpr uint8_t JUMP_HOLD_TICKS = 12;
    constexpr uint8_t WALL_JUMP_AUTO_MOVE_TICKS = 20;
    constexpr float POSITION_SCALE = 1.0f;

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


fight::Scene::Scene(Game& game) :
    _SyncedScene(game)
{
    auto* renderer = new renderer::FightRenderer(_game.getWindow().get(), _game.getRenderer().get(), *this);
    _renderer.reset(static_cast<renderer::_Renderer<State>*>(renderer));
}

fight::Mode fight::Scene::getMode() const
    { return _mode; }

fight::Stage fight::Scene::getStage() const
    { return _stage; }

const std::vector<Player>& fight::Scene::getPlayers() const
    { return _players; }

const fight::Level& fight::Scene::getLevel(std::size_t index) const
    { return _levels[index]; }

void fight::Scene::_activate(SceneContext& context, State& startState)
{
    auto& ctx = static_cast<Context&>(context);
    _players = ctx.players;
    _mode = ctx.mode;
    _stage = ctx.stage;

    // load levels for stage
    {
        auto dir = std::filesystem::path(ASSET_DIR "Levels");
        char num[3];
        SDL_snprintf(num, 3, "%02i", int(_stage));
        dir /= std::string(num) + " - " + stageToName(_stage);
        
        std::vector<std::string> files;
        for (auto& entry : std::filesystem::directory_iterator(dir))
            if (entry.path().extension() == ".oel")
                files.push_back(entry.path().string());
        
        std::sort(files.begin(), files.end());
        _levels.reserve(files.size());
        for (auto& file : files)
            _levels.emplace_back(_stage, file.c_str());
    }

    // prepare starting state
    {
        startState.archers.resize(_players.size());
        _initNewLevel(startState);
    }
}

void fight::Scene::_deactivate()
{
    _levels.clear();
}

_Scene::UpdateReturnStatus fight::Scene::computeFollowingState(const State& givenState, State& followingState, tick_t tick)
{
    followingState = givenState;

    std::size_t i = 0;
    for (auto& player : _players)
    {
        const auto& gArcher = givenState.archers.at(i);
        auto& fArcher = followingState.archers.at(i);

        // inputs
        {
            auto& iBuffer = _inputBufferSet.get(player);
            input::PlayerInput previousInput = iBuffer[tick - 1];
            input::PlayerInput currentInput = iBuffer[tick];
            input::PlayerInput toggle = ~previousInput & currentInput;

            // quit stage
            if (input::get::cancel(toggle))
                return UpdateReturnStatus::SWITCH_SELECTION;

            const auto& level = getLevel(followingState.levelIndex);
            auto startingContacts = collision::contactsAt(level, gArcher);
            glm::ivec2 moveDirection = quantizeInput(currentInput);
            bool jumpPressed = input::get::jump(toggle);
            bool jumpHeld = input::get::jump(currentInput);
            bool jumpReleased = input::get::jump(previousInput) && !jumpHeld;
            bool grounded = startingContacts.ground;
            bool holdingWall = !grounded && wantsWallSlide(startingContacts, moveDirection);

            fArcher.movementDirection = moveDirection;
            if (moveDirection != glm::ivec2(0))
                fArcher.aimDirection = moveDirection;
            fArcher.velocity = gArcher.velocity;

            bool wantsCrouch = grounded && moveDirection.y > 0 && moveDirection.x == 0;
            if (!wantsCrouch && gArcher.isCrouching)
            {
                Archer standProbe = gArcher;
                standProbe.isCrouching = false;
                wantsCrouch = collision::overlapsSolid(level, standProbe);
            }
            fArcher.isCrouching = wantsCrouch;

            if (moveDirection.x != 0)
                fArcher.isFacingRight = moveDirection.x > 0;

            if (jumpPressed)
            {
                int awayFromWall = wallJumpDirection(startingContacts);
                if (grounded)
                {
                    fArcher.velocity.y = JUMP_VELOCITY;
                    fArcher.jumpHoldTicks = JUMP_HOLD_TICKS;
                    fArcher.movementState = ArcherMovementState::AIRBORNE;
                    Archer standProbe = fArcher;
                    standProbe.isCrouching = false;
                    fArcher.isCrouching = collision::overlapsSolid(level, standProbe);
                }
                else if (awayFromWall != 0)
                {
                    fArcher.velocity.y = JUMP_VELOCITY;
                    fArcher.velocity.x = awayFromWall * WALL_JUMP_X_VELOCITY;
                    fArcher.jumpHoldTicks = JUMP_HOLD_TICKS;
                    fArcher.autoMoveTicks = WALL_JUMP_AUTO_MOVE_TICKS;
                    fArcher.autoMoveDirection = awayFromWall;
                    fArcher.isFacingRight = awayFromWall > 0;
                }
            }

            if (jumpReleased && fArcher.velocity.y < JUMP_CUT_VELOCITY)
            {
                fArcher.velocity.y = JUMP_CUT_VELOCITY;
                fArcher.jumpHoldTicks = 0;
            }

            if (fArcher.autoMoveTicks > 0)
            {
                fArcher.velocity.x = approach(
                    fArcher.velocity.x,
                    fArcher.autoMoveDirection * WALL_JUMP_AUTO_SPEED,
                    AIR_ACCELERATION);
                fArcher.autoMoveTicks--;
            }
            else if (fArcher.isCrouching)
            {
                fArcher.velocity.x = approach(fArcher.velocity.x, 0.0f, GROUND_FRICTION);
            }
            else if (moveDirection.x != 0)
            {
                float acceleration = grounded ? GROUND_ACCELERATION : AIR_ACCELERATION;
                fArcher.velocity.x = approach(fArcher.velocity.x, moveDirection.x * MAX_RUN_SPEED, acceleration);
            }
            else
            {
                float friction = grounded ? GROUND_FRICTION : AIR_FRICTION;
                fArcher.velocity.x = approach(fArcher.velocity.x, 0.0f, friction);
            }

            float maxFallSpeed = MAX_FALL_SPEED;
            float gravity = GRAVITY_ACCELERATION;
            if (holdingWall && fArcher.velocity.y > 0.0f && fArcher.autoMoveTicks == 0)
            {
                maxFallSpeed = WALL_SLIDE_MAX_FALL_SPEED;
                gravity = JUMP_HOLD_GRAVITY_ACCELERATION;
            }
            else if (!grounded && moveDirection.y > 0 && fArcher.velocity.y > 0.0f)
            {
                maxFallSpeed = FAST_FALL_MAX_SPEED;
                gravity = FAST_FALL_GRAVITY_ACCELERATION;
            }
            else if (jumpHeld && fArcher.jumpHoldTicks > 0 && fArcher.velocity.y < 0.0f)
            {
                gravity = JUMP_HOLD_GRAVITY_ACCELERATION;
            }

            fArcher.velocity.y = std::min(fArcher.velocity.y + gravity, maxFallSpeed);
            if (!jumpHeld || fArcher.velocity.y >= 0.0f)
                fArcher.jumpHoldTicks = 0;
            else if (fArcher.jumpHoldTicks > 0)
                fArcher.jumpHoldTicks--;

            auto endingContacts = collision::moveAndCollide(level, fArcher, POSITION_SCALE);
            if (endingContacts.ground || endingContacts.ceiling)
                fArcher.jumpHoldTicks = 0;
            if (endingContacts.ground)
                fArcher.autoMoveTicks = 0;

            if (!fArcher.isAlive)
                fArcher.movementState = ArcherMovementState::DEAD;
            else if (endingContacts.ground)
                fArcher.movementState = ArcherMovementState::GROUNDED;
            else if (fArcher.autoMoveTicks == 0
                && fArcher.velocity.y >= 0.0f
                && wantsWallSlide(endingContacts, moveDirection))
            {
                fArcher.velocity.y = std::min(fArcher.velocity.y, WALL_SLIDE_MAX_FALL_SPEED);
                fArcher.movementState = ArcherMovementState::WALL_SLIDING;
            }
            else
                fArcher.movementState = ArcherMovementState::AIRBORNE;
        }

        i++;
    }

    if (givenState.levelIndex != followingState.levelIndex)
        _initNewLevel(followingState);

    return UpdateReturnStatus::STAY;
}

void fight::Scene::_initNewLevel(State& state)
{
    std::size_t i = 0;
    for (auto& archer : state.archers)
    {
        archer.isAlive = true;
        archer.isCrouching = false;
        archer.position = _levels[state.levelIndex].getPlayerSpawnLocation(i);
        archer.velocity = glm::vec2(0);
        archer.isFacingRight = (archer.position.x < _levels[state.levelIndex].getWidth() * TILESIZE / 2);
        archer.movementDirection = glm::ivec2(0);
        archer.aimDirection = glm::ivec2(archer.isFacingRight ? 1 : -1, 0);
        archer.movementState = ArcherMovementState::AIRBORNE;
        archer.jumpHoldTicks = 0;
        archer.autoMoveTicks = 0;
        archer.autoMoveDirection = 0;
        i++;
    }
}
