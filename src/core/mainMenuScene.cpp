#include "mainMenuScene.hpp"
#include "../renderer/mainMenuRenderer.hpp"
#include "../game.hpp"



core::MainMenuScene::MainMenuScene(std::shared_ptr<Game> game) :
    _Scene(game)
{
    auto* renderer = new renderer::MainMenuRenderer();
    _renderer.reset(static_cast<renderer::_Renderer<MainMenuScene::State>*>(renderer));
}


void core::MainMenuScene::update()
{
    // main menu navigation logic using game inputs
    //
    // -------------------------------
    //  PvP 
    //      local
    //      remote
    //
    //  Session Stats
    //
    //  Options
    //      controls
    //          keyboard key mapping
    //          gamepad button mapping
    //      display
    //          resolution
    //          fullscreen
    //      sound
    //          music
    //          effects
    //          bass
    //
    //  Quit
    // -------------------------------

    _state.inputDevicePolls.clear();
    for (auto* pDevice : _game->getInputDevices())
        _state.inputDevicePolls.push_back({ pDevice->getName(), pDevice->poll() });

    _renderer->pushState(_state);
    _renderer->render(_game->getWindow(), _game->getRenderer());
}
