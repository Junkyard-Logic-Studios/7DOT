#pragma once
#include <vector>
#include <array>
#include <utility>
#include <iterator>
#include "iDevice.hpp"
#include "../core/player.hpp"



namespace input
{

    using hostID_t = uint8_t;
    constexpr std::size_t MAX_HOST_COUNT = std::numeric_limits<hostID_t>::max() + 1;
    constexpr std::size_t MAX_LOCAL_DEVICE_COUNT = 16;
    constexpr std::size_t MAX_DEVICE_COUNT = MAX_HOST_COUNT * MAX_LOCAL_DEVICE_COUNT;


    class DeviceManager
    {
    public:
        struct Iterator
        {
            Iterator(
                std::vector<hostID_t>::const_iterator hostIt, 
                const std::vector<hostID_t>::const_iterator hostEnd,
                const std::array<IDevice*, MAX_DEVICE_COUNT>& devices) :
                    _hostIt(hostIt), 
                    _hostEnd(hostEnd),
                    _devices(devices), 
                    _localID(0)
            {}

            std::pair<hostID_t, IDevice*> operator*() const 
            { 
                return { *_hostIt, _devices[*_hostIt * MAX_LOCAL_DEVICE_COUNT + _localID] };
            }

            Iterator& operator++()
            {
                _localID++;

                for (;_hostIt != _hostEnd; _hostIt++, _localID = 0)
                    for (; _localID < MAX_LOCAL_DEVICE_COUNT; _localID++)
                        if (_devices[*_hostIt * MAX_LOCAL_DEVICE_COUNT + _localID])
                            goto NON_NULL;
            
            NON_NULL:
                return *this;
            }

            friend bool operator==(const Iterator& a, const Iterator& b) 
                { return a._hostIt == b._hostIt && a._localID == b._localID; };
            friend bool operator!=(const Iterator& a, const Iterator& b) 
                { return a._hostIt != b._hostIt || a._localID != b._localID; }; 

        private:
            std::vector<hostID_t>::const_iterator _hostIt;
            const std::vector<hostID_t>::const_iterator _hostEnd;
            const std::array<IDevice*, MAX_DEVICE_COUNT>& _devices;
            uint8_t _localID;
        };

        inline DeviceManager(hostID_t hostID) :
            _hostID(hostID)
        {
            IDevice* dev = static_cast<IDevice*>(new Keyboard(0));
            _devices[_hostID * MAX_LOCAL_DEVICE_COUNT] = dev;
            _knownHosts.push_back(hostID);
        }

        inline ~DeviceManager()
        {
            for (auto* d : _devices)
                if (d)
                    delete d;
        }

        inline hostID_t getHostID() const
            { return _hostID; }

        inline void addGamepad(SDL_JoystickID id)
        {
            std::size_t localID = 1;
            for (; _devices[_hostID * MAX_LOCAL_DEVICE_COUNT + localID] 
                && localID < MAX_LOCAL_DEVICE_COUNT; localID++);
            if (localID == MAX_LOCAL_DEVICE_COUNT)
                throw std::runtime_error("all local device slots occupied");
            
            auto *pGamepad = SDL_OpenGamepad(id);
            if (pGamepad)
            {
                IDevice* dev = static_cast<IDevice*>(new Gamepad(localID, pGamepad));
                _devices[_hostID * MAX_LOCAL_DEVICE_COUNT + localID] = dev;
            }
        }

        inline void removeGamepad(SDL_JoystickID id)
        {
            for (std::size_t i = _hostID * MAX_LOCAL_DEVICE_COUNT;
                i < (_hostID + 1) * MAX_LOCAL_DEVICE_COUNT; i++)
                if (auto p = dynamic_cast<Gamepad*>(_devices[i]);
                    p != nullptr && p->getJoystickID() == id)
                {
                    _devices[i] = nullptr;
                    return delete p;
                }
        }

        inline const IDevice* get(hostID_t hostID, uint8_t deviceID) const
        {
            return _devices[hostID * MAX_LOCAL_DEVICE_COUNT + deviceID];
        }

        inline const IDevice* get(const core::Player& player) const
            { return get(player.hostID, player.deviceID); }

        inline Iterator begin() const
            { return Iterator(_knownHosts.begin(), _knownHosts.end(), _devices); }
        
        inline Iterator end() const
            { return Iterator(_knownHosts.end(), _knownHosts.end(), _devices); }

    private:
        hostID_t _hostID;
        std::vector<hostID_t> _knownHosts;
        std::array<IDevice*, MAX_DEVICE_COUNT> _devices { nullptr };
    };

};  // end namespace input
