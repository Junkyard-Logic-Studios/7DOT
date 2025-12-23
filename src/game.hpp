#pragma once
#include <memory>
#include "SDL3/SDL.h"
#include "SDLException.hpp"
#include "sceneContext.hpp"
#include "input/manager.hpp"
#include "mainmenu/scene.hpp"
#include "selection/scene.hpp"
#include "fight/scene.hpp"



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

    Game() :
        _inputManager(0)
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

        // create scenes
        _mainMenuScene  = std::make_unique<mainmenu::Scene>(*this);
        _selectionScene = std::make_unique<selection::Scene>(*this);
        _fightScene     = std::make_unique<fight::Scene>(*this);
        
        // pick main menu scene as first active scene
        _sceneContext = std::make_shared<SceneContext>();
        _activeScene = static_cast<_Scene*>(_mainMenuScene.get());
        _activeScene->activate(*_sceneContext);

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
                _inputManager.addGamepad(event.gdevice.which);
                break;

            case SDL_EVENT_GAMEPAD_REMOVED:
                _inputManager.removeGamepad(event.gdevice.which);
                break;
            }
        }

        // poll input devices
        _inputManager.poll(_inputPipeline);

        // update active scene
        const auto switchScene = [&](_Scene* next) {
            _activeScene->deactivate();
            _activeScene = next;
            _activeScene->activate(*_sceneContext);
        };
        switch (_activeScene->update())
        {
        case _Scene::UpdateReturnStatus::SWITCH_MAINMENU:
            switchScene(_mainMenuScene.get()); break;
        case _Scene::UpdateReturnStatus::SWITCH_SELECTION:
            switchScene(_selectionScene.get()); break;
        case _Scene::UpdateReturnStatus::SWITCH_FIGHT:
            switchScene(_fightScene.get()); break;
        case _Scene::UpdateReturnStatus::QUIT:
            return false;
        }

        return true;
    }

    const std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>& getWindow() const
        { return _window; }

    const std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>& getRenderer() const
        { return _renderer; }

    const input::Manager& getInputManager() const
        { return _inputManager; }
    
    input::Pipeline& getInputPipeline()
        { return _inputPipeline; }

    std::shared_ptr<SceneContext>& getSceneContext()
        { return _sceneContext; }

private:
    // SDL handles
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> _window{nullptr, SDL_DestroyWindow};
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> _renderer{nullptr, SDL_DestroyRenderer};

    // game scenes
    std::shared_ptr<SceneContext>     _sceneContext;
    std::unique_ptr<mainmenu::Scene>  _mainMenuScene;
    std::unique_ptr<selection::Scene> _selectionScene;
    std::unique_ptr<fight::Scene>     _fightScene;
    _Scene* _activeScene;

    // input devices
    input::Manager _inputManager;
    input::Pipeline _inputPipeline;
};
