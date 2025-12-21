#include "scene.hpp"
#include "../renderer/selectionRenderer.hpp"
#include "../game.hpp"
#include "../fight/context.hpp"



selection::Scene::Scene(Game& game) :
    _SyncedScene(game)
{
    auto* renderer = new renderer::SelectionRenderer(_game.getWindow().get(), _game.getRenderer().get());
    _renderer.reset(static_cast<renderer::_Renderer<State>*>(renderer));
}

void selection::Scene::_activate(std::shared_ptr<SceneContext> context)
{
    _knownHosts = context->knownHosts;
}

_Scene::UpdateReturnStatus selection::Scene::computeFollowingState(
    const State& givenState, 
    State& followingState, 
    tick_t tick
) {
    UpdateReturnStatus status = UpdateReturnStatus::STAY;
    switch (givenState.currentLevel)
    {
    case CHARACTERS: 
        if(updateCharacterSelection(givenState, followingState, tick))
        {
            _game.getSceneContext()->startTime = tick + 1;
            status = UpdateReturnStatus::SWITCH_MAINMENU;
        }
        break;
    
    case MODE:
        updateModeSelection(givenState, followingState, tick);
        break;
    
    case TEAM:
        updateTeamSelection(givenState, followingState, tick);
        break;

    case STAGE:
        if(updateStageSelection(givenState, followingState, tick))
        {
            auto context = std::make_shared<fight::Context>();
            context->knownHosts = _knownHosts;
            context->startTime = tick + 1;
            context->fightSelection = followingState.fightSelection;
            _game.getSceneContext() = std::static_pointer_cast<SceneContext>(context);
            status = UpdateReturnStatus::SWITCH_FIGHT;
        }
        break;
    };

    return status;
}

bool selection::Scene::updateCharacterSelection(const State& cstate, State& nstate, tick_t tick)
{
    nstate = cstate;

    for (hostID_t hostID : _knownHosts)
        for (uint8_t deviceID = 0; deviceID < MAX_LOCAL_DEVICE_COUNT; deviceID++)
        {
            auto& iBuffer = _inputBufferSet.get(hostID, deviceID);
            input::PlayerInput previousInput = iBuffer[tick - 1];
            input::PlayerInput currentInput = iBuffer[tick];
            input::PlayerInput toggle = ~previousInput & currentInput;

            auto player = nstate.fightSelection.players.begin();
            for (; player != nstate.fightSelection.players.end(); player++)
                if (player->hostID == hostID && player->deviceID == deviceID)
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
                player.deviceID = deviceID;
                nstate.fightSelection.players.push_back(player);    
            }
            // quit to main menu
            else if (input::get::cancel(toggle))
                return true;
        }

    return false;
}

void selection::Scene::updateModeSelection(const State& cstate, State& nstate, tick_t tick)
{
    nstate = cstate;

    for (auto& player : nstate.fightSelection.players)
    {
        auto& iBuffer = _inputBufferSet.get(player);
        input::PlayerInput previousInput = iBuffer[tick - 1];
        input::PlayerInput currentInput = iBuffer[tick];
        input::PlayerInput toggle = ~previousInput & currentInput;

        // change mode
        float previousHAxis = input::get::horizontalAxis(previousInput);
        float currentHAxis = input::get::horizontalAxis(currentInput);
        if (previousHAxis == 0.0f && currentHAxis != 0.0f)
        {
            int nm = (int)nstate.fightSelection.mode + (currentHAxis > 0.0f ? 1 : -1);
            nm = (nm + (int)FightSelectionInfo::Mode::MAX_ENUM) % (int)FightSelectionInfo::Mode::MAX_ENUM;
            nstate.fightSelection.mode = FightSelectionInfo::Mode(nm);
        }

        // confirm mode selection
        if (input::get::jump(toggle))
        {
            nstate.currentLevel = 
                nstate.fightSelection.mode == FightSelectionInfo::Mode::TEAM_2 ||
                nstate.fightSelection.mode == FightSelectionInfo::Mode::TEAM_4 ?
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

void selection::Scene::updateTeamSelection(const State& cstate, State& nstate, tick_t tick)
{
    nstate = cstate;

    for (auto& player : nstate.fightSelection.players)
    {
        auto& iBuffer = _inputBufferSet.get(player);
        input::PlayerInput previousInput = iBuffer[tick - 1];
        input::PlayerInput currentInput = iBuffer[tick];
        input::PlayerInput toggle = ~previousInput & currentInput;

        float previousVAxis = input::get::verticalAxis(previousInput);
        float currentVAxis = input::get::verticalAxis(currentInput);
        float previousHAxis = input::get::horizontalAxis(previousInput);
        float currentHAxis = input::get::horizontalAxis(currentInput);

        // change team
        if (nstate.fightSelection.mode == FightSelectionInfo::Mode::TEAM_2)
        {
            if (previousHAxis == 0.0f && currentHAxis != 0.0f)
                player.team = currentHAxis > 0.0f ? 1 : 0;
        }
        else if (nstate.fightSelection.mode == FightSelectionInfo::Mode::TEAM_4)
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

bool selection::Scene::updateStageSelection(const State& cstate, State& nstate, tick_t tick)
{
    nstate = cstate;

    for (auto& player : nstate.fightSelection.players)
    {
        auto& iBuffer = _inputBufferSet.get(player);
        input::PlayerInput previousInput = iBuffer[tick - 1];
        input::PlayerInput currentInput = iBuffer[tick];
        input::PlayerInput toggle = ~previousInput & currentInput;

        // change stage
        float previousHAxis = input::get::horizontalAxis(previousInput);
        float currentHAxis = input::get::horizontalAxis(currentInput);
        if (previousHAxis == 0.0f && currentHAxis != 0.0f)
        {
            int ns = (int)nstate.fightSelection.stage + (currentHAxis > 0.0f ? 1 : -1);
            ns = (ns + (int)FightSelectionInfo::Stage::MAX_ENUM) % (int)FightSelectionInfo::Stage::MAX_ENUM;
            nstate.fightSelection.stage = FightSelectionInfo::Stage(ns);
        }

        // confirm stage selection
        if (input::get::jump(toggle))
            return true;

        // quit to mode/team selection
        if (input::get::cancel(toggle))
        {
            nstate.currentLevel = 
                nstate.fightSelection.mode == FightSelectionInfo::Mode::TEAM_2 ||
                nstate.fightSelection.mode == FightSelectionInfo::Mode::TEAM_4 ?
                TEAM : MODE;
            return false;
        }
    }

    return false;
}
