#include "mainMenuScene.hpp"
#include "../renderer/mainMenuRenderer.hpp"
#include "../game.hpp"



core::MainMenuScene::MainMenuScene(Game& game) :
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

    int64_t currentTick = Game::currentTick();

    State newState {};

    for (auto* pDevice : _game.getInputDevices())
        newState.inputDevicePolls.push_back({ pDevice->getName(), pDevice->getInput(currentTick) });

    newState.currentLevel = _state.currentLevel;
    newState.selected = _state.selected;

    if (newState.inputDevicePolls.size() != _state.inputDevicePolls.size())
    {
        _state = newState;
        return;
    }

    // check for confirmation inputs
    for (int i = 0; i < newState.inputDevicePolls.size(); i++)
    {
        input::PlayerInput toggle = (~_state.inputDevicePolls[i].second) & newState.inputDevicePolls[i].second;
        if (!input::get::jump(toggle)) continue;

        switch (_state.selected)
        {
        case TITLE:
            newState.selected = PVP;
            break;
        case PVP:
            newState.currentLevel = PVP;
            newState.selected = PVP_LOCAL;
            break;
        case SESSION_STATS:
            newState.currentLevel = SESSION_STATS;
            newState.selected = SESSION_STATS_BACK;
            break;
        case OPTIONS:
            newState.currentLevel = OPTIONS;
            newState.selected = OPTIONS_BACK;
            break;
        
        case PVP_LOCAL: break;
        case PVP_REMOTE: break;
        case PVP_BACK:
            newState.selected = PVP;
            newState.currentLevel = TITLE;
            break;

        case SESSION_STATS_BACK:
            newState.selected = SESSION_STATS;
            newState.currentLevel = TITLE;
            break;

        case OPTIONS_BACK:
            newState.selected = OPTIONS;
            newState.currentLevel = TITLE;
            break;

        case QUIT: break;
        }
    }

    // check for cancel inputs
    for (int i = 0; i < newState.inputDevicePolls.size(); i++)
    {
        input::PlayerInput toggle = (~_state.inputDevicePolls[i].second) & newState.inputDevicePolls[i].second;
        if (!input::get::cancel(toggle) || _state.currentLevel == TITLE) continue;

        newState.selected = _state.currentLevel;
        newState.currentLevel = TITLE;
    }

    // check for up/down inputs
    for (int i = 0; i < newState.inputDevicePolls.size(); i++)
    {
        float prev = input::get::verticalAxis(_state.inputDevicePolls[i].second);
        float value = input::get::verticalAxis(newState.inputDevicePolls[i].second);
        if (prev != 0.0f || value == 0.0f || _state.selected == TITLE) continue;

        auto next = NavigationOptions(value > 0 ? _state.selected + 1 : _state.selected - 1);

        switch (_state.currentLevel)
        {
        case TITLE:
            newState.selected = value > 0 ? 
                (_state.selected == QUIT ? QUIT : next) : 
                (_state.selected == PVP ? PVP : next);
            break;

        case PVP:
            newState.selected = value > 0 ?
                (_state.selected == PVP_BACK ? PVP_BACK : next) :
                (_state.selected == PVP_LOCAL ? PVP_LOCAL : next);
            break;
        }
    }

    _state = newState;

    _renderer->pushState(_state);
    _renderer->render(_game.getWindow(), _game.getRenderer());
}
