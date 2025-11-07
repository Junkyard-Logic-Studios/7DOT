#pragma once
#include <array>
#include "../constants.hpp"
#include "input.hpp"
#include <stdexcept>
#include <string>



namespace input
{

    template<typename TTime, typename TInput, std::size_t NElem>
    class InputBufferT
    {
    public:
        inline void insert(TTime timestamp, TInput input)
        {
        #ifdef DEBUG
            if (timestamp + NElem <= _latest)
                throw std::invalid_argument("timestamp too old\n"\
                    "timestamp: " + std::to_string(timestamp) +
                    "\n_latest: " + std::to_string(_latest) +
                    "\nNElem: " + std::to_string(NElem));
        #endif

            // insert before latest
            if (timestamp < _latest)
            {
                TTime t = timestamp + 1;
                while (_timestampBuffer[t % NElem] < timestamp)
                {
                    _timestampBuffer[t % NElem] = timestamp;
                    _inputBuffer[t % NElem] = input;
                    t++;
                }
            }

            // insert after latest
            if (timestamp > _latest)
            {
                TTime exTime = _latest;
                TInput exInput = _inputBuffer[_latest % NElem];
                _latest = std::max(_latest + 1, timestamp - TTime(NElem));
                for (; _latest < timestamp; _latest++)
                {
                    _timestampBuffer[_latest % NElem] = exTime;
                    _inputBuffer[_latest % NElem] = exInput;
                }
            }

            // update at key position
            _timestampBuffer[timestamp % NElem] = timestamp;
            _inputBuffer[timestamp % NElem] = input;
        }

        inline TInput operator[](TTime timestamp) const 
        {
        #ifdef DEBUG
            if (timestamp + NElem <= _latest)
                throw std::invalid_argument("timestamp too old\n"\
                    "timestamp: " + std::to_string(timestamp) +
                    "\n_latest: " + std::to_string(_latest) +
                    "\nNElem: " + std::to_string(NElem));
        #endif

            timestamp = std::min(timestamp, _latest);
            return _inputBuffer[timestamp % NElem];
        }

        inline TTime latest() const
        {
            return _latest;
        }

        inline std::size_t size() const
        {
            return NElem;
        }

    private:
        TTime _latest = 0;
        std::array<TTime, NElem> _timestampBuffer {0};
        std::array<TInput, NElem> _inputBuffer {0};        
    };


    using InputBuffer = 
        InputBufferT<
            int64_t, 
            PlayerInput, 
            static_cast<std::size_t>(roundup_pow2(MAX_ROLLBACK_TICKS + MAX_INPUT_LOOKBACK_TICKS))
        >;

};  // end namespace input
