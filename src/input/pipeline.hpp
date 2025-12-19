#pragma once
#include <mutex>
#include <array>
#include "input.hpp"



namespace input
{

    constexpr std::size_t INPUT_PIPELINE_SIZE = MAX_DEVICE_COUNT;


    class Pipeline
    {
    public:
        inline void put(PlayerInput value)
        {
            const std::lock_guard<std::mutex> lock(_mutex);
            _buffer[_top % INPUT_PIPELINE_SIZE] = value;
            _top++;
        }

        inline bool take(PlayerInput& value)
        {
            const std::lock_guard<std::mutex> lock(_mutex);
            if (_bot < _top)
            {
                value = _buffer[_bot % INPUT_PIPELINE_SIZE];
                _bot++;
                return true;
            }
            else
                return false;
        }

        inline std::size_t size() const
            { return _top - _bot; }

    private:
        std::mutex _mutex;
        std::size_t _bot = 0;
        std::size_t _top = 0;
        std::array<PlayerInput, INPUT_PIPELINE_SIZE> _buffer;
    };

};  // end namespace input
