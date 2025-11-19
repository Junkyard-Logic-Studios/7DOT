#pragma once
#include <type_traits>
#include <chrono>
using namespace std::chrono_literals;


#define DEBUG
constexpr auto MS_PER_TICK = 10;            // -> 100 ticks per second
constexpr auto MAX_ROLLBACK = 1s;           // -> maximum rollback of 1 second
constexpr auto MAX_INPUT_LOOKBACK = 1s;     // -> state computation looks at no inputs older than 1 second


namespace std::chrono 
{
    using ticks = duration<int64_t, std::ratio<MS_PER_TICK, 1000>::type>;
};
