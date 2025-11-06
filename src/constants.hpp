#pragma once
#include <type_traits>

template<typename T, typename std::enable_if_t<std::is_integral<std::decay_t<T>>::value>* = nullptr>
constexpr T roundup_div(T dividend, T divisor)
{
    return (dividend + divisor - 1) / divisor;
}

template<typename T, typename std::enable_if_t<std::is_integral<std::decay_t<T>>::value>* = nullptr>
constexpr T roundup_pow2(T num)
{
    T r = 1;
    while (r < num && r) r <<= 1;
    return r;
}

#define MS_PER_TICK 10                      // -> 100 ticks per second
#define MAX_ROLLBACK_MS 1000                // -> maximum rollback of 1 second
#define MAX_ROLLBACK_TICKS roundup_div(MAX_ROLLBACK_MS, MS_PER_TICK)
#define MAX_INPUT_LOOKBACK_MS 1000          // -> state computation looks at no inputs older than 1 second
#define MAX_INPUT_LOOKBACK_TICKS roundup_div(MAX_INPUT_LOOKBACK_MS, MS_PER_TICK)
