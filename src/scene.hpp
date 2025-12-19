#pragma once
#include <memory>
#include "sceneContext.hpp"



class Game;

class _Scene
{
public:
    enum class UpdateReturnStatus
    {
        STAY,
        SWITCH_MAINMENU,
        SWITCH_SELECTION,
        SWITCH_FIGHT,
        QUIT
    };

    inline _Scene(Game& game) :
        _game(game)
    {}

    virtual void activate(std::shared_ptr<SceneContext> context) = 0;
    virtual void deactivate() = 0;
    virtual UpdateReturnStatus update() = 0;

protected:
    Game& _game;
};
