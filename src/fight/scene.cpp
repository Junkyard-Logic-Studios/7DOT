#include <filesystem>
#include <algorithm>
#include "scene.hpp"
#include "context.hpp"
#include "movement.hpp"
#include "../renderer/fightRenderer.hpp"
#include "../game.hpp"


namespace
{
    constexpr float PHYSICS_DELTA_T = static_cast<float>(DELTA_T);
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
            movement::stepArcher(
                level,
                archer,
                currentInput,
                justPressed,
                justReleased,
                PHYSICS_DELTA_T);
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
        archer.jumpHoldTime = 0.0f;
        archer.autoMoveTime = 0.0f;
        archer.autoMoveDirection = 0;
        archer.wallGrabDirection = 0;
        i++;
    }
}
