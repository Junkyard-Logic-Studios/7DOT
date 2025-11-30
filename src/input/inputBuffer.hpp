#pragma once
#include <array>
#include "../constants.hpp"
#include "input.hpp"
#include <stdexcept>
#include <string>



namespace input
{

    constexpr std::size_t INPUT_BUFFER_SIZE = 
        []{
            auto num = std::chrono::duration_cast<std::chrono::ticks>(
                MAX_ROLLBACK + MAX_INPUT_LOOKBACK).count();
            std::size_t r = 1;
            while (r < num && r) r <<= 1;
            return r;
        }();


    class InputBuffer
    {
    public:
        inline void insert(PlayerInput input)
        {
            tick_t timestamp = get::timestamp(input);

        #if DEBUG == true
            if (timestamp + INPUT_BUFFER_SIZE <= _latest)
                throw std::invalid_argument("timestamp too old\n"\
                    "timestamp: " + std::to_string(timestamp) +
                    "\n_latest: " + std::to_string(_latest) +
                    "\nsize: " + std::to_string(INPUT_BUFFER_SIZE));
        #endif

            // insert before latest
            if (timestamp < _latest)
            {
                for (tick_t t = timestamp + 1;
                     get::timestamp(_buffer[t % INPUT_BUFFER_SIZE]) < timestamp;
                     t++)
                    _buffer[t % INPUT_BUFFER_SIZE] = input;
            }

            // insert after latest
            if (timestamp > _latest)
            {
                PlayerInput ex = _buffer[_latest % INPUT_BUFFER_SIZE];
                for (_latest = std::max(_latest + 1, timestamp - tick_t(INPUT_BUFFER_SIZE)); 
                     _latest < timestamp; 
                     _latest++)
                    _buffer[_latest % INPUT_BUFFER_SIZE] = ex;
            }

            // update at key position
            _buffer[timestamp % INPUT_BUFFER_SIZE] = input;
        }

        inline PlayerInput operator[](tick_t timestamp) const
        {
        #if DEBUG == true
            if (timestamp + INPUT_BUFFER_SIZE <= _latest)
                throw std::invalid_argument("timestamp too old\n"\
                    "timestamp: " + std::to_string(timestamp) +
                    "\n_latest: " + std::to_string(_latest) +
                    "\nsize: " + std::to_string(INPUT_BUFFER_SIZE));
        #endif

            timestamp = std::min(timestamp, _latest);
            return _buffer[timestamp % INPUT_BUFFER_SIZE];
        }

        inline tick_t latest() const
            { return _latest; }

    private:
        tick_t _latest = 0;
        std::array<PlayerInput, INPUT_BUFFER_SIZE> _buffer{0};
    };

};  // end namespace input
