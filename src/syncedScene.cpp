#include <thread>
#include "syncedScene.hpp"
#include "game.hpp"



template<typename S>
void _SyncedScene<S>::activate(std::shared_ptr<SceneContext> context)
{
    _inputBufferSet = input::InputBufferSet(context->knownHosts);
    _startTime = context->startTime;

    // start with state where scene last left off
    _stateBuffer[_startTime % STATE_BUFFER_SIZE] = 
        _stateBuffer[_latestValid % STATE_BUFFER_SIZE];
    _latestValid = _startTime;

    _activate(context);
}

template<typename S>
void _SyncedScene<S>::deactivate() {}

template<typename S>
_Scene::UpdateReturnStatus _SyncedScene<S>::update()
{
    tick_t currentTick = Game::currentTick();
    UpdateReturnStatus returnStatus = UpdateReturnStatus::STAY;

    while (_latestValid < currentTick)
    {
        // compute one new state
        returnStatus = computeFollowingState(
            _stateBuffer[(_latestValid + 0) % STATE_BUFFER_SIZE], 
            _stateBuffer[(_latestValid + 1) % STATE_BUFFER_SIZE], 
            _latestValid + 1);
        
        _latestValid++;

        // check input pipeline for new (/delayed -> rollback-worthy) inputs
        input::PlayerInput i;
        while (_game.getInputPipeline().take(i))
            if (input::get::timestamp(i) > _startTime)
            {
                _inputBufferSet.get(input::get::hostID(i), input::get::deviceID(i)).insert(i);
                _latestValid = std::min(_latestValid, input::get::timestamp(i) - 1);
            }
    }

    if (_renderer)
    {
        _renderer->pushState(_stateBuffer[_latestValid % STATE_BUFFER_SIZE]);
        _renderer->render();
    }

    // sleep till next game tick to avoid inputs beeing applied twice
    std::this_thread::sleep_until(Game::nextTickTime(currentTick));

    return returnStatus;
}


template class _SyncedScene<selection::State>;
template class _SyncedScene<fight::State>;
