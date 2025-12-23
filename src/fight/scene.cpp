#include "scene.hpp"
#include "context.hpp"
#include "../renderer/fightRenderer.hpp"
#include "../game.hpp"



fight::Scene::Scene(Game& game) :
    _SyncedScene(game)
{
    auto* renderer = new renderer::FightRenderer(_game.getWindow().get(), _game.getRenderer().get());
    _renderer.reset(static_cast<renderer::_Renderer<State>*>(renderer));
}

void fight::Scene::_activate(SceneContext& context)
{
    _fightSelection = static_cast<Context&>(context).fightSelection;
    static_cast<renderer::FightRenderer*>(_renderer.get())->
        setFightSelectionInfo(_fightSelection);
}

_Scene::UpdateReturnStatus fight::Scene::computeFollowingState(const State& givenState, State& followingState, tick_t tick)
{ 
    return tick % (3000 / MS_PER_TICK) ? 
        UpdateReturnStatus::STAY : 
        UpdateReturnStatus::SWITCH_SELECTION;
}
