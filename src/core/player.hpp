#pragma once
#include <inttypes.h>



namespace core
{

    struct Player 
    {
        uint8_t hostID         = 0;
        uint8_t deviceID       = 0;
        uint8_t team           = 0;
        unsigned int character = 0;
    };

};  // end namespace core
