#pragma once
#include "configure.hpp"
#include <inttypes.h>
#include <chrono>
using namespace std::chrono_literals;


constexpr auto MS_PER_TICK = 10;            // -> 100 ticks per second
constexpr auto MAX_ROLLBACK = 1s;           // -> maximum rollback of 1 second
constexpr auto MAX_INPUT_LOOKBACK = 1s;     // -> state computation looks at no inputs older than 1 second
constexpr auto VIRTUAL_INPUT_LAG = 10ms;    // -> artificially delay all inputs

namespace std::chrono 
{
    using ticks = duration<int64_t, std::ratio<MS_PER_TICK, 1000>::type>;
};

using tick_t = int32_t;

using hostID_t = uint8_t;
constexpr std::size_t MAX_HOST_COUNT = std::numeric_limits<hostID_t>::max() + 1;
constexpr std::size_t MAX_LOCAL_DEVICE_COUNT = 16;
constexpr std::size_t MAX_DEVICE_COUNT = MAX_HOST_COUNT * MAX_LOCAL_DEVICE_COUNT;
