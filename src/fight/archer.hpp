#pragma once
#include <inttypes.h>



namespace fight
{

    class Archer
    {
    public:
        struct State 
        {
            float posX;
            float posY;

            float velX;
            float velY;

            uint32_t isFacingRight : 1;
            uint32_t isAlive : 1;
            uint32_t isCrouching : 1;
            // ...
        };

    private:
        // player / input device reference
    };

};  // end namespace core
