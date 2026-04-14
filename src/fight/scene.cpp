#include <filesystem>
#include <algorithm>
#include "glm/geometric.hpp"
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

            // movement
            fArcher.velocity = gArcher.velocity + glm::vec2(
                input::get::horizontalAxis(currentInput), 
                MS_PER_TICK * .1f - 30.f * input::get::jump(toggle));                

            // next level
            if (input::get::shoot(toggle))
                followingState.levelIndex = (givenState.levelIndex + 1) % _levels.size();

            // quit stage
            if (input::get::cancel(toggle))
                return UpdateReturnStatus::SWITCH_SELECTION;
        }

        // collisions (super temporary)
        {
            const auto& level = getLevel(followingState.levelIndex);

            for (std::size_t x = 0; x < level.getWidth(); x++)
                for (std::size_t y = 0; y < level.getHeight(); y++)
                {
                    bool solid = level.getSolidAt(x, y) != -1;
                    if (!solid)
                        continue;   // not a solid tile
                    
                    bool sepU = fArcher.hitboxBR().y < (y - .5f) * TILESIZE;
                    bool sepD = fArcher.hitboxTL().y > (y + .5f) * TILESIZE;
                    bool sepL = fArcher.hitboxBR().x < (x - .5f) * TILESIZE;
                    bool sepR = fArcher.hitboxTL().x > (x + .5f) * TILESIZE;

                    if (sepU || sepD || sepL || sepR)
                        continue;   // separated in at least one direction

                    auto v = (fArcher.position - glm::vec2(x, y) * float(TILESIZE));
                    v *= glm::abs(v.x) > glm::abs(v.y) ? glm::vec2(1,0) : glm::vec2(0,1);
                    fArcher.velocity += v / glm::length(v) * .5f;

                    continue;
                }
        }
        
        // apply
        {
            // velocity
            fArcher.position = gArcher.position + fArcher.velocity * float(MS_PER_TICK) * .1f;

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
