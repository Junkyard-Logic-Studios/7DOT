#include "iDevice.hpp"
#include "../game.hpp"



void input::IDevice::poll()
{
    // actual input device polling might take multiple ticks, therefore 
    // define an inputs timestamp to be after its polling has completed
    PlayerInput input = _poll();
    if (input != _lastInput)
    {
        _inputBuffer.insert(Game::currentTick(), input);
        _lastInput = input;
    }
}

input::PlayerInput input::IDevice::getInput(int64_t tick) const
{
    return _inputBuffer[tick];
}
