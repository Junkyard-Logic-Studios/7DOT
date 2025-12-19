#pragma once
#include "inputBuffer.hpp"
#include "../player.hpp"



namespace input
{

    class InputBufferSet
    {
    public:
        inline InputBufferSet(const std::vector<hostID_t>& hosts)
        {
            _buffers.resize(hosts.size() * MAX_LOCAL_DEVICE_COUNT);
            uint8_t bufferIndex = 0;
            for (hostID_t hostID : hosts)
                _lookup[hostID] = bufferIndex++;
        }

        inline InputBuffer& get(hostID_t hostID, uint8_t deviceID)
            { return _buffers.at(_lookup[hostID] * MAX_LOCAL_DEVICE_COUNT + deviceID); }
        
        inline InputBuffer& get(const Player& player)
            { return get(player.hostID, player.deviceID); }

    private:
        uint8_t _lookup[MAX_HOST_COUNT];
        std::vector<InputBuffer> _buffers;
    };

};  // end namespace input
