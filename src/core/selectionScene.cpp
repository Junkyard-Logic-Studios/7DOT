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
    UpdateReturnStatus returnStatus = UpdateReturnStatus::STAY;

    switch (_stateBuffer[0].currentLevel)
    {
    case CHARACTERS: 
        if(updateCharacterSelection(_stateBuffer[0], currentTick))
            returnStatus = UpdateReturnStatus::SWITCH_MAINMENU;
        break;
    
    case MODE:
        updateModeSelection(_stateBuffer[0], currentTick);
        break;
    
    case TEAM:
        updateTeamSelection(_stateBuffer[0], currentTick);
        break;

    case STAGE:
        if(updateStageSelection(_stateBuffer[0], currentTick))
            returnStatus = UpdateReturnStatus::SWITCH_FIGHT;
        break;
    };

    _renderer->pushState(_stateBuffer[0]);
    _renderer->render();

    // sleep till next game tick to avoid inputs beeing applied twice
    std::this_thread::sleep_until(Game::nextTickTime(currentTick));

    return returnStatus;
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
        
        // device input influences player
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
            if (input::get::jump(toggle))
            {
                state.currentLevel = MODE;
                return false;
            }
        }
        // join player selection
        else if (input::get::jump(toggle))
        {
            Player player;
            player.deviceID = pDevice->getID();         // TODO: set also host ID
            state.fightSelection.players.push_back(player);    
        }
        // quit to main menu
        else if (input::get::cancel(toggle))
            return true;
    }

    return false;
}

void core::SelectionScene::updateModeSelection(State& state, tick_t tick)
{
    for (auto& player : state.fightSelection.players)
    {
        auto* pDevice = _game.getInputDeviceManager().get(player);
        if (!pDevice) continue;
        
        input::PlayerInput previousInput = pDevice->getInput(tick - 1);
        input::PlayerInput currentInput = pDevice->getInput(tick);
        input::PlayerInput toggle = ~previousInput & currentInput;

        // change mode
        float previousHAxis = input::get::horizontalAxis(previousInput);
        float currentHAxis = input::get::horizontalAxis(currentInput);
        if (previousHAxis == 0.0f && currentHAxis != 0.0f)
        {
            int nm = (int)state.fightSelection.mode + (currentHAxis > 0.0f ? 1 : -1);
            nm = (nm + (int)FightSelection::Mode::MAX_ENUM) % (int)FightSelection::Mode::MAX_ENUM;
            state.fightSelection.mode = FightSelection::Mode(nm);
        }

        // confirm mode selection
        if (input::get::jump(toggle))
        {
            state.currentLevel = 
                state.fightSelection.mode == FightSelection::Mode::TEAM_2 ||
                state.fightSelection.mode == FightSelection::Mode::TEAM_4 ?
                TEAM : STAGE;
            if (state.currentLevel == TEAM)
                for (auto& p : state.fightSelection.players)
                    p.team = 0;
            return;
        }

        // quit to character selection
        if (input::get::cancel(toggle))
        {
            state.currentLevel = CHARACTERS;
            return;
        }
    }
}

void core::SelectionScene::updateTeamSelection(State& state, tick_t tick)
{
    for (auto& player : state.fightSelection.players)
    {
        auto* pDevice = _game.getInputDeviceManager().get(player);
        if (!pDevice) continue;

        input::PlayerInput previousInput = pDevice->getInput(tick - 1);
        input::PlayerInput currentInput = pDevice->getInput(tick);
        input::PlayerInput toggle = ~previousInput & currentInput;

        float previousVAxis = input::get::verticalAxis(previousInput);
        float currentVAxis = input::get::verticalAxis(currentInput);
        float previousHAxis = input::get::horizontalAxis(previousInput);
        float currentHAxis = input::get::horizontalAxis(currentInput);

        // change team
        if (state.fightSelection.mode == FightSelection::Mode::TEAM_2)
        {
            if (previousHAxis == 0.0f && currentHAxis != 0.0f)
                player.team = currentHAxis > 0.0f ? 1 : 0;
        }
        else if (state.fightSelection.mode == FightSelection::Mode::TEAM_4)
        {
            if (previousHAxis == 0.0f && currentHAxis != 0.0f)
                player.team = (player.team & ~1) | (currentHAxis > 0.0f ? 1 : 0);
            if (previousVAxis == 0.0f && currentVAxis != 0.0f)
                player.team = (player.team & ~2) | (currentVAxis > 0.0f ? 2 : 0);
        }

        // confirm mode selection
        if (input::get::jump(toggle))
        {
            state.currentLevel = STAGE;
            return;
        }

        // quit to character selection
        if (input::get::cancel(toggle))
        {
            state.currentLevel = MODE;
            return;
        }
    }
}

bool core::SelectionScene::updateStageSelection(State& state, tick_t tick)
{
    for (auto& player : state.fightSelection.players)
    {
        auto* pDevice = _game.getInputDeviceManager().get(player);
        if (!pDevice) continue;
        
        input::PlayerInput previousInput = pDevice->getInput(tick - 1);
        input::PlayerInput currentInput = pDevice->getInput(tick);
        input::PlayerInput toggle = ~previousInput & currentInput;

        // change stage
        float previousHAxis = input::get::horizontalAxis(previousInput);
        float currentHAxis = input::get::horizontalAxis(currentInput);
        if (previousHAxis == 0.0f && currentHAxis != 0.0f)
        {
            int ns = (int)state.fightSelection.stage + (currentHAxis > 0.0f ? 1 : -1);
            ns = (ns + (int)FightSelection::Stage::MAX_ENUM) % (int)FightSelection::Stage::MAX_ENUM;
            state.fightSelection.stage = FightSelection::Stage(ns);
        }

        // confirm stage selection
        if (input::get::jump(toggle))
            return true;

        // quit to mode/team selection
        if (input::get::cancel(toggle))
        {
            state.currentLevel = 
                state.fightSelection.mode == FightSelection::Mode::TEAM_2 ||
                state.fightSelection.mode == FightSelection::Mode::TEAM_4 ?
                TEAM : MODE;
            return false;
        }
    }

    return false;
}
