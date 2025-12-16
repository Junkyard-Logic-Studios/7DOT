#pragma once
#include "../constants.hpp"



namespace core
{

    constexpr std::size_t STATE_BUFFER_SIZE =
    []{
        auto num = std::chrono::duration_cast<std::chrono::ticks>(
            MAX_ROLLBACK).count();
        std::size_t r = 1;
        while (r < num && r) r <<= 1;
        return r;
    }();


    template<typename S>
    class SceneStateBuffer
    {
    public:
        inline S& operator[](tick_t timestamp)
            { return _stateBuffer[timestamp % STATE_BUFFER_SIZE]; }
        
        inline void rollback(tick_t inputTimestamp)
            { _latestValid = std::min(_latestValid, inputTimestamp - 1); }

        inline S& getLatestValidState() const
            { return (*this)[_latestValid]; }

        inline tick_t getLatestValidTick() const
            { return _latestValid; }
        
        inline void setLatestValidTick(tick_t timestamp)
        { 
            (*this)[timestamp] = (*this)[_latestValid];
            _latestValid = timestamp;
        }
        
        inline std::tuple<const S&, S&, tick_t> calculateNextValidState()
        {
            const S& cstate = (*this)[_latestValid++];
            S& nstate = (*this)[_latestValid];
            return { cstate, nstate, _latestValid };
        }
        
    private:
        tick_t _latestValid = 0;
        std::array<S, STATE_BUFFER_SIZE> _stateBuffer;
    };

};  // end namespace core
