#pragma once



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

    virtual void activate() = 0;
    virtual void deactivate() = 0;
    virtual UpdateReturnStatus update() = 0;

protected:
    Game& _game;
};
