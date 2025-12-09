#include <thread>
#include "selectionScene.hpp"
#include "../renderer/selectionRenderer.hpp"
#include "../game.hpp"



core::SelectionScene::SelectionScene(Game& game) :
    _Scene(game)
{
    auto* renderer = new renderer::SelectionRenderer(_game.getWindow().get(), _game.getRenderer().get());
    _renderer.reset(static_cast<renderer::_Renderer<SelectionScene::State>*>(renderer));

    // TODO: replace with properly sized static array
    _stateBuffer = new State[1];
}

void core::SelectionScene::activate()
{}

void core::SelectionScene::deactivate()
{}

core::_Scene::UpdateReturnStatus core::SelectionScene::update() 
{ 
    tick_t currentTick = Game::currentTick();

    for (auto* pDevice : _game.getInputDeviceManager().getAll())
    {
        input::PlayerInput previousInput = pDevice->getInput(currentTick - 1);
        input::PlayerInput currentInput = pDevice->getInput(currentTick);
        input::PlayerInput toggle = ~previousInput & currentInput;

        if (input::get::cancel(toggle))
            return UpdateReturnStatus::SWITCH_MAINMENU;
    }

    _renderer->pushState(_stateBuffer[0]);
    _renderer->render();

    // sleep till next game tick to avoid inputs beeing applied twice
    std::this_thread::sleep_until(Game::nextTickTime(currentTick));

    return UpdateReturnStatus::STAY;
}
