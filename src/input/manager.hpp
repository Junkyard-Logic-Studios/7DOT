#pragma once
#include <array>
#include "iDevice.hpp"
#include "pipeline.hpp"



namespace input
{

    class Manager
    {
    public:
        inline Manager(hostID_t hostID) :
            _hostID(hostID)
        {
            IDevice* dev = static_cast<IDevice*>(new Keyboard(0));
            _devices[0] = dev;
        }

        inline ~Manager()
        {
            for (auto* d : _devices)
                if (d) delete d;
        }

        inline hostID_t getHostID() const
            { return _hostID; }

        inline std::array<IDevice*, MAX_LOCAL_DEVICE_COUNT> getLocalDevices() const
            { return _devices; }
        
        inline void addGamepad(SDL_JoystickID joystickID)
        {
            std::size_t localID = 1;
            for (; _devices[localID] && localID < MAX_LOCAL_DEVICE_COUNT; localID++);
            if (localID == MAX_LOCAL_DEVICE_COUNT)
                throw std::runtime_error("all local device slots occupied");
            
            auto *pGamepad = SDL_OpenGamepad(joystickID);
            if (pGamepad)
                _devices[localID] = static_cast<IDevice*>(new Gamepad(localID, pGamepad));
        }

        inline void removeGamepad(SDL_JoystickID joystickID)
        {
            for (auto& d : _devices)
                if (auto p = dynamic_cast<Gamepad*>(d);
                    p != nullptr && p->getJoystickID() == joystickID)
                {
                    d = nullptr;
                    return delete p;
                }
        }

        inline void poll(Pipeline& pipeline)
        {
            // put local device inputs into input pipeline
            PlayerInput i;
            for (auto* device : _devices)
                if (device && device->poll(i))
                {
                    set::hostID(i, _hostID);   
                    pipeline.put(i);
                }
            
            // TODO: put local device inputs into connection send buffers
            // TODO: put remote inputs from connection receive buffers into input pipeline 
        }

    private:
        hostID_t _hostID;
        std::array<IDevice*, MAX_LOCAL_DEVICE_COUNT> _devices { nullptr };
    };

};  // end namespace input
