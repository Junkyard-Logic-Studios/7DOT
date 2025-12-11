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
            IDevice* dev = static_cast<IDevice*>(new Keyboard(0));
            _localDevices[0] = dev;
            _devices.push_back(dev);
        }

        inline ~DeviceManager()
        {
            for (auto d : _devices)
                delete d;
            _devices.clear();
        }

        inline void addGamepad(SDL_JoystickID id)
        {
            std::size_t localID = 1;
            for (;_localDevices[localID] && localID < 16; localID++);
            if (localID == 16)
                throw std::runtime_error("all local device slots occupied");
            
            auto *pGamepad = SDL_OpenGamepad(id);
            if (pGamepad)
            {
                IDevice* dev = static_cast<IDevice*>(new Gamepad(localID, pGamepad));
                _localDevices[localID] = dev;
                _devices.push_back(dev);
            }
        }

        inline void removeGamepad(SDL_JoystickID id)
        {
            for (auto it = _devices.begin(); it != _devices.end(); it++)
                if (auto p = dynamic_cast<Gamepad*>(*it); 
                    p != nullptr && p->getJoystickID() == id)
                {
                    _localDevices[p->getID()] = nullptr;
                    _devices.erase(it);
                    return delete p;
                }
        }

        inline const IDevice* get(uint8_t hostID, uint8_t deviceID) const
            { return _localDevices[deviceID]; }     // TODO: consider also host ID

        inline const std::vector<IDevice*>& getAll() const
            { return _devices; }

    private:
        IDevice* _localDevices[16] = { nullptr };
        std::vector<IDevice*> _devices;
    };

};  // end namespace input
