#pragma once
#include "scene.hpp"
#include "constants.hpp"
#include "input/inputBufferSet.hpp"
#include "input/pipeline.hpp"
#include "renderer/renderer.hpp"
#include <memory>



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

    void activate(SceneContext& context);
    void deactivate();
    UpdateReturnStatus update();
    
protected:
    inline const input::InputBuffer& _getInputBuffer(const Player& player) const
        { return _inputBufferSet.get(player); }
    inline const input::InputBuffer& _getInputBuffer(hostID_t hostID, uint8_t deviceID) const
        { return _inputBufferSet.get(hostID, deviceID); }
    inline const S& _getState(tick_t tick) const
        { return _stateBuffer[(tick + STATE_BUFFER_SIZE) % STATE_BUFFER_SIZE]; }

    std::unique_ptr<renderer::_Renderer<S>> _renderer;
    
    inline virtual void _activate(SceneContext& context, S& startState) {};
    inline virtual void _deactivate() {};
    virtual UpdateReturnStatus computeState(S& state, tick_t tick) = 0;
    
private:
    tick_t _startTime = 0;
    tick_t _latestValid = 0;
    input::InputBufferSet _inputBufferSet {{}};
    S _stateBuffer[STATE_BUFFER_SIZE];
};
