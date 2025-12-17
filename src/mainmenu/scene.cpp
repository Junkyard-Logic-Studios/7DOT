#include <thread>
#include "scene.hpp"
#include "../renderer/mainMenuRenderer.hpp"
#include "../game.hpp"



mainmenu::Scene::Scene(Game& game) :
    _Scene(game)
{
    auto* renderer = new renderer::MainMenuRenderer(_game.getWindow().get(), _game.getRenderer().get());
    _renderer.reset(static_cast<renderer::_Renderer<mainmenu::State>*>(renderer));
}

void mainmenu::Scene::activate() 
{}

void mainmenu::Scene::deactivate() 
{}

_Scene::UpdateReturnStatus mainmenu::Scene::update()
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

    tick_t currentTick = Game::currentTick();

    // check latest inputs from all devices
    _state.inputDevicePolls.clear();
    for (auto [_, pDevice] : _game.getInputDeviceManager())
    {
        input::PlayerInput previousInput = pDevice->getInput(currentTick - 1);
        input::PlayerInput currentInput = pDevice->getInput(currentTick);
        input::PlayerInput toggle = ~previousInput & currentInput;

        _state.inputDevicePolls.push_back({ pDevice->getName(), currentInput });

        // check for confirmation inputs
        if (input::get::jump(toggle))
        {
            switch (_state.selected)
            {
            case TITLE:
                _state.selected = PVP;
                break;
            case PVP:
                _state.currentLevel = PVP;
                _state.selected = PVP_LOCAL;
                break;
            case SESSION_STATS:
                _state.currentLevel = SESSION_STATS;
                _state.selected = SESSION_STATS_BACK;
                break;
            case OPTIONS:
                _state.currentLevel = OPTIONS;
                _state.selected = OPTIONS_BACK;
                break;
            
            case PVP_LOCAL:
                return UpdateReturnStatus::SWITCH_SELECTION;

            case PVP_REMOTE: break;
                return UpdateReturnStatus::SWITCH_SELECTION;
            
            case PVP_BACK:
                _state.selected = PVP;
                _state.currentLevel = TITLE;
                break;

            case SESSION_STATS_BACK:
                _state.selected = SESSION_STATS;
                _state.currentLevel = TITLE;
                break;

            case OPTIONS_BACK:
                _state.selected = OPTIONS;
                _state.currentLevel = TITLE;
                break;

            case QUIT:
                return UpdateReturnStatus::QUIT;
            }

            continue;   // recognize only 1 input per device
        }

        // check for cancellation inputs
        if (input::get::cancel(toggle) && _state.currentLevel != TITLE)
        {
            _state.selected = _state.currentLevel;
            _state.currentLevel = TITLE;

            continue;   // recognize only 1 input per device
        }

        // check for up/down inputs
        float previousVAxis = input::get::verticalAxis(previousInput);
        float currentVAxis = input::get::verticalAxis(currentInput);
        if (previousVAxis == 0.0f && currentVAxis != 0.0f && _state.selected != TITLE)
        {
            auto next = NavigationOptions(currentVAxis > 0 ? 
                _state.selected + 1 : _state.selected - 1);

            switch (_state.currentLevel)
            {
            case TITLE:
                _state.selected = currentVAxis > 0 ? 
                    (_state.selected == QUIT ? QUIT : next) : 
                    (_state.selected == PVP ? PVP : next);
                break;

            case PVP:
                _state.selected = currentVAxis > 0 ?
                    (_state.selected == PVP_BACK ? PVP_BACK : next) :
                    (_state.selected == PVP_LOCAL ? PVP_LOCAL : next);
                break;
            }

            continue;   // recognize only 1 input per device
        }
    }

    _renderer->pushState(_state);
    _renderer->render();

    // sleep till next game tick to avoid inputs beeing applied twice
    std::this_thread::sleep_until(Game::nextTickTime(currentTick));

    return UpdateReturnStatus::STAY;
}
