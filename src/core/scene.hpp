#pragma once
#include <memory>
#include "../constants.hpp"



class Game;

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


    class _Scene
    {
    public:
        enum UpdateReturnStatus
        {
            STAY = 0,
            SWITCH_MAINMENU,
            SWITCH_SELECTION,
            SWITCH_FIGHT,
            QUIT
        };

        inline _Scene(Game& game) :
            _game(game)
        {}

        virtual void activate() = 0;
        virtual void deactivate() = 0;
        virtual UpdateReturnStatus update() = 0;

    protected:
        Game& _game;
    };

};  // end namespace core
