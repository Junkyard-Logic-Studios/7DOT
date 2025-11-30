#pragma once
#include "configure.hpp"
#include <type_traits>
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
