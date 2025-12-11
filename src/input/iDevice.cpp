#include "iDevice.hpp"
#include "../game.hpp"



void input::IDevice::poll()
{
    // actual input device polling might take multiple ticks, therefore 
    // define an inputs timestamp to be after its polling has completed
    PlayerInput input = _poll();
    if (get::actions(input) != _lastActions)
    {
        set::timestamp(input, Game::currentTick() + std::chrono::duration_cast<std::chrono::ticks>(VIRTUAL_INPUT_LAG).count());
        set::deviceID(input, _id);
        _inputBuffer.insert(input);
        _lastActions = get::actions(input);
    }
}

input::PlayerInput input::IDevice::getInput(tick_t tick) const
{
    return _inputBuffer[tick];
}
