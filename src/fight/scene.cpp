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
    constexpr float MOVE_ACCELERATION = 1.0f;
    constexpr float GRAVITY_ACCELERATION = MS_PER_TICK * 0.1f;
    constexpr float JUMP_VELOCITY = -30.0f;
    constexpr float WALL_JUMP_X_VELOCITY = 18.0f;
    constexpr float POSITION_SCALE = MS_PER_TICK * 0.1f;
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

            // next level
            if (input::get::shoot(toggle))
                followingState.levelIndex = (givenState.levelIndex + 1) % _levels.size();

            // quit stage
            if (input::get::cancel(toggle))
                return UpdateReturnStatus::SWITCH_SELECTION;

            const auto& level = getLevel(followingState.levelIndex);
            auto contacts = collision::contactsAt(level, gArcher);

            fArcher.velocity = gArcher.velocity;
            fArcher.velocity.x += input::get::horizontalAxis(currentInput) * MOVE_ACCELERATION;
            fArcher.velocity.y += GRAVITY_ACCELERATION;

            if (input::get::jump(toggle))
            {
                if (contacts.ground)
                    fArcher.velocity.y = JUMP_VELOCITY;
                else if (contacts.leftWall && !contacts.rightWall)
                {
                    fArcher.velocity.y = JUMP_VELOCITY;
                    fArcher.velocity.x = WALL_JUMP_X_VELOCITY;
                }
                else if (contacts.rightWall && !contacts.leftWall)
                {
                    fArcher.velocity.y = JUMP_VELOCITY;
                    fArcher.velocity.x = -WALL_JUMP_X_VELOCITY;
                }
            }

            collision::moveAndCollide(level, fArcher, POSITION_SCALE);

            // drag
            fArcher.velocity.x = std::max(0.0f, std::abs(fArcher.velocity.x) - float(MS_PER_TICK))
                * (fArcher.velocity.x < 0.0 ? -1.0 : 1.0);
            fArcher.velocity.y = std::max(0.0f, std::abs(fArcher.velocity.y) - float(MS_PER_TICK))
                * (fArcher.velocity.y < 0.0 ? -1.0 : 1.0);
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
        i++;
    }
}
