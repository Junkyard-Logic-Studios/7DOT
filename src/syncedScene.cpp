#include <thread>
#include "syncedScene.hpp"
#include "game.hpp"



template<typename S>
void _SyncedScene<S>::activate()
{
    // start with state where scene last left off
    auto currentTick = Game::currentTick();
    _stateBuffer[currentTick % STATE_BUFFER_SIZE] = 
        _stateBuffer[_latestValid % STATE_BUFFER_SIZE];
    _latestValid = currentTick;
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

        // check input queue for new (/delayed -> rollback-worthy) inputs
        checkInputs();
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

template<typename S>
void _SyncedScene<S>::checkInputs()
{
//  for input : input_queue
//      input_buffers.insert(input)
//      _latestValid = min(_latestValid, input.timestamp)
}


template class _SyncedScene<selection::State>;
template class _SyncedScene<fight::State>;
