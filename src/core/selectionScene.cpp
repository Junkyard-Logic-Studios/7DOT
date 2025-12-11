#include <thread>
#include "selectionScene.hpp"
#include "player.hpp"
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

    switch (_stateBuffer[0].currentLevel)
    {
    case CHARACTERS: 
        if(updateCharacterSelection(_stateBuffer[0], currentTick))
            return UpdateReturnStatus::SWITCH_MAINMENU;
        break;
    
    case MODE:
        updateModeSelection(_stateBuffer[0], currentTick);
        break;

    case STAGE:
        if(updateStageSelection(_stateBuffer[0], currentTick))
            return UpdateReturnStatus::SWITCH_FIGHT;
        break;
    };

    _renderer->pushState(_stateBuffer[0]);
    _renderer->render();

    // sleep till next game tick to avoid inputs beeing applied twice
    std::this_thread::sleep_until(Game::nextTickTime(currentTick));

    return UpdateReturnStatus::STAY;
}

bool core::SelectionScene::updateCharacterSelection(State& state, tick_t tick)
{
    for (auto* pDevice : _game.getInputDeviceManager().getAll())
    {
        input::PlayerInput previousInput = pDevice->getInput(tick - 1);
        input::PlayerInput currentInput = pDevice->getInput(tick);
        input::PlayerInput toggle = ~previousInput & currentInput;

        auto player = state.fightSelection.players.begin();
        for (; player != state.fightSelection.players.end(); player++)
            if (player->deviceID == pDevice->getID())         // TODO: check also for host ID
                break;
        
        if (player != state.fightSelection.players.end())
        {
            // change character
            float previousHAxis = input::get::horizontalAxis(previousInput);
            float currentHAxis = input::get::horizontalAxis(currentInput);
            if (previousHAxis == 0.0f && currentHAxis > 0.0f)
                player->character++;
            if (previousHAxis == 0.0f && currentHAxis < 0.0f)
                player->character--;

            // quit player selection
            if (input::get::cancel(toggle))
            {
                state.fightSelection.players.erase(player);
                continue;
            }

            // confirm selections
            if (input::get::start(toggle))
            {
                state.currentLevel = MODE;
                return false;
            }
        }
        else
        {
            // join player selection
            if (input::get::jump(toggle))
            {
                Player player;
                player.deviceID = pDevice->getID();
                state.fightSelection.players.push_back(player);
            }

            // quit to main menu
            if (input::get::cancel(toggle))
                return true;
        }
    }

    return false;
}

void core::SelectionScene::updateModeSelection(State& state, tick_t tick)
{

}

bool core::SelectionScene::updateStageSelection(State& state, tick_t tick)
{
    return false;
}
