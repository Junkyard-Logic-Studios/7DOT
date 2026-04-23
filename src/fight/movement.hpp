#pragma once
#include "../input/input.hpp"
#include "archer.hpp"
#include "level.hpp"



namespace fight::movement
{
    void stepArcher(
        const Level& level,
        Archer& archer,
        input::PlayerInput currentInput,
        input::PlayerInput justPressed,
        input::PlayerInput justReleased,
        float deltaTime);

};  // end namespace fight::movement
