#pragma once
#include <memory>



class Game;

namespace core
{

    class _Scene
    {
    public:
        inline _Scene(Game& game) :
            _game(game)
        {}

        inline bool isActive()
            { return _isActive; }

        inline virtual void activate()
            { _isActive = true; }
        
        inline virtual void deactivate()
            { _isActive = false; }
        
        virtual void update() = 0;

    protected:
        Game& _game;
        bool _isActive = false;
    };

};  // end namespace core
