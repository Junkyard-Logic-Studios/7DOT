#pragma once
#include "scene.hpp"
#include "constants.hpp"
#include "renderer/renderer.hpp"



constexpr std::size_t STATE_BUFFER_SIZE =
[]{
    auto num = std::chrono::duration_cast<std::chrono::ticks>(
        MAX_ROLLBACK).count();
    std::size_t r = 1;
    while (r < num && r) r <<= 1;
    return r;
}();


template<typename S>
class _SyncedScene : public _Scene
{
public:
    inline _SyncedScene(Game& game) :
        _Scene(game)
    {}

    virtual void activate();
    virtual void deactivate();
    virtual UpdateReturnStatus update();

protected:
    tick_t _latestValid = 0;
    std::array<S, STATE_BUFFER_SIZE> _stateBuffer;

    std::unique_ptr<renderer::_Renderer<S>> _renderer;

    virtual UpdateReturnStatus computeFollowingState(
        const S& givenState, S& followingState, tick_t tick) = 0;

private:
    void checkInputs();
};
