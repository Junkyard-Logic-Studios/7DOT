#pragma once
#include <inttypes.h>
#include "../constants.hpp"



namespace input
{
    
    using PlayerInput = uint64_t;


    enum BitMask : uint64_t
    {
        HOST_ID               = 0xFF00'0000'0000'0000,

        DEVICE_ID             = 0x00FF'0000'0000'0000,

        ACTIONS               = 0x0000'FFFF'0000'0000,

        DASH                  = 0x0000'0007'0000'0000,
        START                 = 0x0000'0008'0000'0000,
        SHOOT                 = 0x0000'0010'0000'0000,
        JUMP                  = 0x0000'0020'0000'0000,
        TOGGLE                = 0x0000'0040'0000'0000,
        CANCEL                = 0x0000'0080'0000'0000,
        HORIZONTAL_AXIS_SIGN  = 0x0000'0800'0000'0000,
        HORIZONTAL_AXIS_VALUE = 0x0000'0700'0000'0000,
        HORIZONTAL_AXIS       = HORIZONTAL_AXIS_SIGN | HORIZONTAL_AXIS_VALUE,
        VERTICAL_AXIS_SIGN    = 0x0000'8000'0000'0000,
        VERTICAL_AXIS_VALUE   = 0x0000'7000'0000'0000,
        VERTICAL_AXIS         = VERTICAL_AXIS_SIGN | VERTICAL_AXIS_VALUE,
    
        TIMESTAMP             = 0x0000'0000'FFFF'FFFF
    };


    namespace set
    {

        inline void hostID(PlayerInput& input, uint8_t id)
            { input = (input & ~BitMask::HOST_ID) | ((uint64_t(id) << 56) & BitMask::HOST_ID); }

        inline void deviceID(PlayerInput& input, uint8_t id)
            { input = (input & ~BitMask::DEVICE_ID) | ((uint64_t(id) << 48) & BitMask::DEVICE_ID); }

        inline void actions(PlayerInput& input, uint16_t actions)
            { input = (input & ~BitMask::ACTIONS) | ((uint64_t(actions) << 32) & BitMask::ACTIONS); }

        inline void timestamp(PlayerInput& input, tick_t tick)
            { input = (input & ~BitMask::TIMESTAMP) | (tick & BitMask::TIMESTAMP); }

        inline void dashCount(PlayerInput& input, uint64_t count) 
            { input = (input & ~BitMask::DASH) | ((count << 32) & BitMask::DASH); }

        inline void start(PlayerInput& input, bool value)  
            { input = (input & ~BitMask::START) | (value * BitMask::START); }

        inline void shoot(PlayerInput& input, bool value)  
            { input = (input & ~BitMask::SHOOT) | (value * BitMask::SHOOT); }

        inline void jump(PlayerInput& input, bool value)   
            { input = (input & ~BitMask::JUMP) | (value * BitMask::JUMP); }

        inline void toggle(PlayerInput& input, bool value) 
            { input = (input & ~BitMask::TOGGLE) | (value * BitMask::TOGGLE); }

        inline void cancel(PlayerInput& input, bool value) 
            { input = (input & ~BitMask::CANCEL) | (value * BitMask::CANCEL); }
        
        inline void horizontalAxis(PlayerInput& input, int16_t value)
        {
            uint64_t sign = value < 0;
            uint64_t temp = (value ^ -sign) << 28;      // get one's complement representation
            input = (input & ~BitMask::HORIZONTAL_AXIS) 
                  | (temp & BitMask::HORIZONTAL_AXIS_VALUE) 
                  | (sign * BitMask::HORIZONTAL_AXIS_SIGN);
        }

        inline void verticalAxis(PlayerInput& input, int16_t value)
        {
            uint64_t sign = value < 0;
            uint64_t temp = (value ^ -sign) << 32;      // get one's complement representation
            input = (input & ~BitMask::VERTICAL_AXIS) 
                  | (temp & BitMask::VERTICAL_AXIS_VALUE) 
                  | (sign * BitMask::VERTICAL_AXIS_SIGN);
        }

    };  // end namespace set


    namespace get
    {

        inline uint8_t hostID(const PlayerInput input)
            { return (input & BitMask::HOST_ID) >> 56; }
        
        inline uint8_t deviceID(const PlayerInput input)
            { return (input & BitMask::DEVICE_ID) >> 48; }
        
        inline uint16_t actions(const PlayerInput input)
            { return (input & BitMask::ACTIONS) >> 32; }
        
        inline tick_t timestamp(const PlayerInput input)
            { return input & BitMask::TIMESTAMP; }

        inline unsigned int dashCount (const PlayerInput input) 
            { return (input & BitMask::DASH) >> 32; }
        
        inline bool start(const PlayerInput input)  
            { return input & BitMask::START; }

        inline bool shoot(const PlayerInput input)  
            { return input & BitMask::SHOOT; }

        inline bool jump(const PlayerInput input)   
            { return input & BitMask::JUMP; }

        inline bool toggle(const PlayerInput input) 
            { return input & BitMask::TOGGLE; }

        inline bool cancel(const PlayerInput input) 
            { return input & BitMask::CANCEL; }
                
        inline float horizontalAxis(const PlayerInput input) 
        {
            int64_t sign = (input & BitMask::HORIZONTAL_AXIS_SIGN) == BitMask::HORIZONTAL_AXIS_SIGN;
            int64_t value = (input & BitMask::HORIZONTAL_AXIS_VALUE) >> 40;
            return float((value ^ -sign) + sign) / 7.0f;
        }

        inline float verticalAxis(const PlayerInput input)
        {
            int64_t sign = (input & BitMask::VERTICAL_AXIS_SIGN) == BitMask::VERTICAL_AXIS_SIGN;
            int64_t value = (input & BitMask::VERTICAL_AXIS_VALUE) >> 44;
            return float((value ^ -sign) + sign) / 7.0f;
        }

    };  // end namespace get

};  // end namespace input
