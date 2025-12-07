#pragma once
#include <vector>
#include <unordered_map>
#include "iDevice.hpp"



namespace input
{

    class DeviceManager
    {
    public:
        inline DeviceManager()
        {
            _devices.emplace_back(new Keyboard());
        }

        inline ~DeviceManager()
        {
            for (auto d : _devices)
                delete d;
            _devices.clear();
        }

        inline void addGamepad(SDL_JoystickID id)
        {                
            auto *pGamepad = SDL_OpenGamepad(id);
            if (pGamepad)
                _devices.emplace_back(new Gamepad(pGamepad));
        }

        inline void removeGamepad(SDL_JoystickID id)
        {
            for (auto it = _devices.begin(); it != _devices.end(); it++)
                if (auto p = dynamic_cast<Gamepad*>(*it); 
                    p != nullptr && p->getID() == id)
                {
                    _devices.erase(it);
                    return;
                }
        }

        inline const std::vector<IDevice*>& getAll() const
            { return _devices; }

    private:
        std::vector<IDevice*> _devices;
    };

};  // end namespace input
