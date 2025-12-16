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
}

void core::SelectionScene::activate()
{
    _stateBuffer.setLatestValidTick(Game::currentTick());
}

void core::SelectionScene::deactivate()
{}

core::_Scene::UpdateReturnStatus core::SelectionScene::update() 
{ 
    tick_t currentTick = Game::currentTick();
    tick_t latestValidTick = _stateBuffer.getLatestValidTick();
    UpdateReturnStatus returnStatus = UpdateReturnStatus::STAY;

    while (latestValidTick < currentTick)
    {
        // cstate: current state 
        // nstate: next state to be calculated from current state
        // tick: tick for nstate
        auto [cstate, nstate, tick] = _stateBuffer.calculateNextValidState();
        latestValidTick = tick;

        switch (cstate.currentLevel)
        {
        case CHARACTERS: 
            if(updateCharacterSelection(cstate, nstate, tick))
                returnStatus = UpdateReturnStatus::SWITCH_MAINMENU;
            break;
        
        case MODE:
            updateModeSelection(cstate, nstate, tick);
            break;
        
        case TEAM:
            updateTeamSelection(cstate, nstate, tick);
            break;
    
        case STAGE:
            if(updateStageSelection(cstate, nstate, tick))
                returnStatus = UpdateReturnStatus::SWITCH_FIGHT;
            break;
        };
    
        _renderer->pushState(nstate);
    }
    _renderer->render();

    // sleep till next game tick to avoid inputs beeing applied twice
    std::this_thread::sleep_until(Game::nextTickTime(currentTick));

    return returnStatus;
}

bool core::SelectionScene::updateCharacterSelection(const State& cstate, State& nstate, tick_t tick)
{
    nstate = cstate;

    for (auto [hostID, pDevice] : _game.getInputDeviceManager())
    {
        input::PlayerInput previousInput = pDevice->getInput(tick - 1);
        input::PlayerInput currentInput = pDevice->getInput(tick);
        input::PlayerInput toggle = ~previousInput & currentInput;

        auto player = nstate.fightSelection.players.begin();
        for (; player != nstate.fightSelection.players.end(); player++)
            if (player->hostID == hostID && player->deviceID == pDevice->getID())
                break;
        
        // device input influences player
        if (player != nstate.fightSelection.players.end())
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
                nstate.fightSelection.players.erase(player);
                continue;
            }

            // confirm selections
            if (input::get::jump(toggle))
            {
                nstate.currentLevel = MODE;
                return false;
            }
        }
        // join player selection
        else if (input::get::jump(toggle))
        {
            Player player;
            player.hostID = hostID;
            player.deviceID = pDevice->getID();
            nstate.fightSelection.players.push_back(player);    
        }
        // quit to main menu
        else if (input::get::cancel(toggle))
            return true;
    }

    return false;
}

void core::SelectionScene::updateModeSelection(const State& cstate, State& nstate, tick_t tick)
{
    nstate = cstate;

    for (auto& player : nstate.fightSelection.players)
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
            int nm = (int)nstate.fightSelection.mode + (currentHAxis > 0.0f ? 1 : -1);
            nm = (nm + (int)FightSelection::Mode::MAX_ENUM) % (int)FightSelection::Mode::MAX_ENUM;
            nstate.fightSelection.mode = FightSelection::Mode(nm);
        }

        // confirm mode selection
        if (input::get::jump(toggle))
        {
            nstate.currentLevel = 
                nstate.fightSelection.mode == FightSelection::Mode::TEAM_2 ||
                nstate.fightSelection.mode == FightSelection::Mode::TEAM_4 ?
                TEAM : STAGE;
            if (nstate.currentLevel == TEAM)
                for (auto& p : nstate.fightSelection.players)
                    p.team = 0;
            return;
        }

        // quit to character selection
        if (input::get::cancel(toggle))
        {
            nstate.currentLevel = CHARACTERS;
            return;
        }
    }
}

void core::SelectionScene::updateTeamSelection(const State& cstate, State& nstate, tick_t tick)
{
    nstate = cstate;

    for (auto& player : nstate.fightSelection.players)
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
        if (nstate.fightSelection.mode == FightSelection::Mode::TEAM_2)
        {
            if (previousHAxis == 0.0f && currentHAxis != 0.0f)
                player.team = currentHAxis > 0.0f ? 1 : 0;
        }
        else if (nstate.fightSelection.mode == FightSelection::Mode::TEAM_4)
        {
            if (previousHAxis == 0.0f && currentHAxis != 0.0f)
                player.team = (player.team & ~1) | (currentHAxis > 0.0f ? 1 : 0);
            if (previousVAxis == 0.0f && currentVAxis != 0.0f)
                player.team = (player.team & ~2) | (currentVAxis > 0.0f ? 2 : 0);
        }

        // confirm mode selection
        if (input::get::jump(toggle))
        {
            nstate.currentLevel = STAGE;
            return;
        }

        // quit to character selection
        if (input::get::cancel(toggle))
        {
            nstate.currentLevel = MODE;
            return;
        }
    }
}

bool core::SelectionScene::updateStageSelection(const State& cstate, State& nstate, tick_t tick)
{
    nstate = cstate;

    for (auto& player : nstate.fightSelection.players)
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
            int ns = (int)nstate.fightSelection.stage + (currentHAxis > 0.0f ? 1 : -1);
            ns = (ns + (int)FightSelection::Stage::MAX_ENUM) % (int)FightSelection::Stage::MAX_ENUM;
            nstate.fightSelection.stage = FightSelection::Stage(ns);
        }

        // confirm stage selection
        if (input::get::jump(toggle))
            return true;

        // quit to mode/team selection
        if (input::get::cancel(toggle))
        {
            nstate.currentLevel = 
                nstate.fightSelection.mode == FightSelection::Mode::TEAM_2 ||
                nstate.fightSelection.mode == FightSelection::Mode::TEAM_4 ?
                TEAM : MODE;
            return false;
        }
    }

    return false;
}
