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

_Scene::UpdateReturnStatus fight::Scene::computeState(State& state, tick_t tick)
{
    // initialize state computation by copying previous state
    state = _getState(tick - 1);

    // update players
    std::size_t playerIndex = 0;
    for (auto& player : _players)
    {
        // get inputs
        input::PlayerInput currentInput;
        input::PlayerInput justPressed;
        input::PlayerInput justReleased;
        {
            auto& iBuffer = _getInputBuffer(player);
            input::PlayerInput previousInput = iBuffer[tick - 1];
            currentInput = iBuffer[tick];
            justPressed = ~previousInput & currentInput;
            justReleased = previousInput & ~currentInput;

            // next level
            if (input::get::start(justPressed))
                state.levelIndex = (state.levelIndex + 1) % _levels.size();

            // quit stage
            if (input::get::cancel(justPressed))
                return UpdateReturnStatus::SWITCH_SELECTION;
        }


        // update archer
        auto& archer = state.archers.at(playerIndex);
        {
            const auto& level = getLevel(state.levelIndex);
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
                    archer.jumpHoldTicks = JUMP_HOLD_TICKS;
                    archer.movementState = ArcherMovementState::AIRBORNE;
                    Archer standProbe = archer;
                    standProbe.isCrouching = false;
                    archer.isCrouching = collision::overlapsSolid(level, standProbe);
                }
                else if (awayFromWall != 0)
                {
                    archer.velocity.y = JUMP_VELOCITY;
                    archer.velocity.x = awayFromWall * WALL_JUMP_X_VELOCITY;
                    archer.jumpHoldTicks = JUMP_HOLD_TICKS;
                    archer.autoMoveTicks = WALL_JUMP_AUTO_MOVE_TICKS;
                    archer.autoMoveDirection = awayFromWall;
                    archer.isFacingRight = awayFromWall > 0;
                }
            }

            if (input::get::jump(justReleased) && archer.velocity.y < JUMP_CUT_VELOCITY)
            {
                archer.velocity.y = JUMP_CUT_VELOCITY;
                archer.jumpHoldTicks = 0;
            }

            if (archer.autoMoveTicks > 0)
            {
                archer.velocity.x = approach(
                    archer.velocity.x,
                    archer.autoMoveDirection * WALL_JUMP_AUTO_SPEED,
                    AIR_ACCELERATION);
                archer.autoMoveTicks--;
            }
            else if (archer.isCrouching)
            {
                archer.velocity.x = approach(archer.velocity.x, 0.0f, GROUND_FRICTION);
            }
            else if (moveDirection.x != 0)
            {
                float acceleration = grounded ? GROUND_ACCELERATION : AIR_ACCELERATION;
                archer.velocity.x = approach(archer.velocity.x, moveDirection.x * MAX_RUN_SPEED, acceleration);
            }
            else
            {
                float friction = grounded ? GROUND_FRICTION : AIR_FRICTION;
                archer.velocity.x = approach(archer.velocity.x, 0.0f, friction);
            }

            float maxFallSpeed = MAX_FALL_SPEED;
            float gravity = GRAVITY_ACCELERATION;
            if (holdingWall && archer.velocity.y > 0.0f && archer.autoMoveTicks == 0)
            {
                maxFallSpeed = WALL_SLIDE_MAX_FALL_SPEED;
                gravity = JUMP_HOLD_GRAVITY_ACCELERATION;
            }
            else if (!grounded && moveDirection.y > 0 && archer.velocity.y > 0.0f)
            {
                maxFallSpeed = FAST_FALL_MAX_SPEED;
                gravity = FAST_FALL_GRAVITY_ACCELERATION;
            }
            else if (input::get::jump(currentInput) && archer.jumpHoldTicks > 0 && archer.velocity.y < 0.0f)
            {
                gravity = JUMP_HOLD_GRAVITY_ACCELERATION;
            }

            archer.velocity.y = std::min(archer.velocity.y + gravity, maxFallSpeed);
            if (!input::get::jump(currentInput) || archer.velocity.y >= 0.0f)
                archer.jumpHoldTicks = 0;
            else if (archer.jumpHoldTicks > 0)
                archer.jumpHoldTicks--;

            auto endingContacts = collision::moveAndCollide(level, archer, POSITION_SCALE);
            if (endingContacts.ground || endingContacts.ceiling)
                archer.jumpHoldTicks = 0;
            if (endingContacts.ground)
                archer.autoMoveTicks = 0;

            if (!archer.isAlive)
                archer.movementState = ArcherMovementState::DEAD;
            else if (endingContacts.ground)
                archer.movementState = ArcherMovementState::GROUNDED;
            else if (archer.autoMoveTicks == 0
                && archer.velocity.y >= 0.0f
                && wantsWallSlide(endingContacts, moveDirection))
            {
                archer.velocity.y = std::min(archer.velocity.y, WALL_SLIDE_MAX_FALL_SPEED);
                archer.movementState = ArcherMovementState::WALL_SLIDING;
            }
            else
                archer.movementState = ArcherMovementState::AIRBORNE;
        }

        playerIndex++;
    }

    if (_getState(tick - 1).levelIndex != state.levelIndex)
        _initNewLevel(state);

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
