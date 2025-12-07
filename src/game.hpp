#pragma once
#include <chrono>
#include <memory>
#include <unordered_map>
#include "SDL3/SDL.h"
#include "SDLException.hpp"
#include "constants.hpp"
#include "input/deviceManager.hpp"
#include "core/scene.hpp"
#include "core/mainMenuScene.hpp"
#include "core/selectionScene.hpp"
#include "core/fightScene.hpp"



class Game
{
public:
    inline static std::chrono::system_clock::time_point startTime()
    {
        using namespace std::chrono;
        static system_clock::time_point start = system_clock::now();
        return start;
    }

    inline static tick_t currentTick()
    {
        using namespace std::chrono;
        return duration_cast<ticks>(system_clock::now() - startTime()).count();
    }

    inline static std::chrono::system_clock::time_point nextTickTime(tick_t tick)
    {
        using namespace std::chrono;
        return startTime() + duration_cast<system_clock::duration>(ticks(tick + 1));
    }

    Game()
    {
        // init SDL
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
            throw SDLException("Failed to init SDL");

        // prepare SDL window and renderer
        SDL_Window *window;
        SDL_Renderer *renderer;
        if (!SDL_CreateWindowAndRenderer("Game", 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN, &window, &renderer))
            throw SDLException("Failed to create window and renderer");
        _window.reset(window);
        _renderer.reset(renderer);

        // pick main menu scene as first active scene
        _activeScene = static_cast<core::_Scene*>(&_mainMenuScene);
        _activeScene->activate();

        // show window once initialization is complete
        SDL_ShowWindow(_window.get());

        // log some info
        SDL_LogInfo(0, "ms per tick:              %i",  MS_PER_TICK);
        SDL_LogInfo(0, "max rollback ticks:       %li", std::chrono::duration_cast<std::chrono::ticks>(MAX_ROLLBACK).count());
        SDL_LogInfo(0, "max input lookback ticks: %li", std::chrono::duration_cast<std::chrono::ticks>(MAX_INPUT_LOOKBACK).count());
        SDL_LogInfo(0, "input buffer size:        %li", input::INPUT_BUFFER_SIZE);
    }

    ~Game()
    {
        _activeScene->deactivate();
        SDL_Quit();
    }

    bool update()
    {
        // poll SDL events
        for (SDL_Event event; SDL_PollEvent(&event);)
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                return false;

            case SDL_EVENT_GAMEPAD_ADDED:
                _deviceManager.addGamepad(event.gdevice.which);
                break;

            case SDL_EVENT_GAMEPAD_REMOVED:
                _deviceManager.removeGamepad(event.gdevice.which);
                break;
            }
        }

        // poll input devices
        for (auto* d : _deviceManager.getAll())
            d->poll();

        // update active scene
        const auto switchScene = [&](core::_Scene& next) {
            _activeScene->deactivate();
            _activeScene = &next;
            _activeScene->activate();
        };
        switch (_activeScene->update())
        {
        case core::_Scene::UpdateReturnStatus::SWITCH_MAINMENU:
            switchScene(_mainMenuScene); break;
        case core::_Scene::UpdateReturnStatus::SWITCH_SELECTION:
            switchScene(_selectionScene); break;
        case core::_Scene::UpdateReturnStatus::SWITCH_FIGHT:
            switchScene(_fightScene); break;
        case core::_Scene::UpdateReturnStatus::QUIT:
            return false;
        }

        return true;
    }

    const std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>& getWindow() const
        { return _window; }

    const std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>& getRenderer() const
        { return _renderer; }

    const input::DeviceManager& getInputDeviceManager() const
        { return _deviceManager; }

private:
    // SDL handles
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> _window{nullptr, SDL_DestroyWindow};
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> _renderer{nullptr, SDL_DestroyRenderer};

    // game scenes
    core::MainMenuScene  _mainMenuScene  = core::MainMenuScene(*this);
    core::SelectionScene _selectionScene = core::SelectionScene(*this);
    core::FightScene     _fightScene     = core::FightScene(*this);
    core::_Scene*        _activeScene;

    // input devices
    input::DeviceManager _deviceManager;
};
