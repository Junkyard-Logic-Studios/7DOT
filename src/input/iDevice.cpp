#include "iDevice.hpp"
#include "../game.hpp"



bool input::IDevice::poll(PlayerInput& value)
{
    value = _poll();
    
    // actual input device polling might take multiple ticks, therefore 
    // define an inputs timestamp to be after its polling has completed
    set::timestamp(value, Game::currentTick() + 
        std::chrono::duration_cast<std::chrono::ticks>(VIRTUAL_INPUT_LAG).count());
    set::deviceID(value, _id);
    
    if (get::actions(value) != _lastActions)
    {
        _lastActions = get::actions(value);
        return true;
    }
    return false;
}
