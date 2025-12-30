#include <filesystem>
#include <algorithm>
#include "scene.hpp"
#include "context.hpp"
#include "../renderer/fightRenderer.hpp"
#include "../game.hpp"



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

const fight::Level& fight::Scene::getLevel(std::size_t index) const
    { return _levels[index]; }

void fight::Scene::_activate(SceneContext& context)
{
    auto& ctx = static_cast<Context&>(context);
    _players = ctx.players;
    _mode = ctx.mode;
    _stage = ctx.stage;

    // load levels for stage
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

void fight::Scene::_deactivate()
{
    _levels.clear();
}

_Scene::UpdateReturnStatus fight::Scene::computeFollowingState(const State& givenState, State& followingState, tick_t tick)
{
    followingState = givenState;

    for (auto& player : _players)
    {
        auto& iBuffer = _inputBufferSet.get(player);
        input::PlayerInput previousInput = iBuffer[tick - 1];
        input::PlayerInput currentInput = iBuffer[tick];
        input::PlayerInput toggle = ~previousInput & currentInput;

        // next level
        if (input::get::jump(toggle))
            followingState.levelIndex = (givenState.levelIndex + 1) % _levels.size();

        // quit stage
        if (input::get::cancel(toggle))
            return UpdateReturnStatus::SWITCH_SELECTION;
    }

    return UpdateReturnStatus::STAY;
}
