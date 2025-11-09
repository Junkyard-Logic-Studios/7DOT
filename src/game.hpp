#pragma once
#include <chrono>
#include <memory>
#include <unordered_map>
#include "SDL3/SDL.h"
#include "SDLException.hpp"
#include "constants.hpp"
#include "input/iDevice.hpp"
#include "core/scene.hpp"
#include "core/mainMenuScene.hpp"
#include "core/selectionScene.hpp"
#include "core/fightScene.hpp"



class Game
{
public:
    inline static int64_t currentTick()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() / MS_PER_TICK;
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
                if (_gamepads.find(event.gdevice.which) == _gamepads.end())
                {
                    auto *pGamepad = SDL_OpenGamepad(event.gdevice.which);
                    if (pGamepad)
                        _gamepads.emplace(event.gdevice.which, pGamepad);
                }
                break;

            case SDL_EVENT_GAMEPAD_REMOVED:
                auto it = _gamepads.find(event.gdevice.which);
                if (it != _gamepads.end())
                    _gamepads.erase(it);
                break;
            }
        }

        // poll input devices
        _keyboard.poll();
        for (auto& [_, gamepad] : _gamepads)
            gamepad.poll();

        // update active scene
        switch (_activeScene->update())
        {
        case core::_Scene::UpdateReturnStatus::SWITCH_MAINMENU:
            _activeScene->deactivate();
            _activeScene = &_mainMenuScene;
            _activeScene->activate();
            break;

        case core::_Scene::UpdateReturnStatus::SWITCH_SELECTION:
            _activeScene->deactivate();
            _activeScene = &_selectionScene;
            _activeScene->activate();
            break;

        case core::_Scene::UpdateReturnStatus::SWITCH_FIGHT:
            _activeScene->deactivate();
            _activeScene = &_fightScene;
            _activeScene->activate();
            break;

        case core::_Scene::UpdateReturnStatus::QUIT:
            return false;
        }

        return true;
    }

    const std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>& getWindow() const
        { return _window; }

    const std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>& getRenderer() const
        { return _renderer; }

    std::vector<input::IDevice*> getInputDevices()
    {
        std::vector<input::IDevice*> v = 
            { static_cast<input::IDevice*>(&_keyboard) };
        for (auto& [_, gamepad] : _gamepads)
            v.push_back(static_cast<input::IDevice*>(&gamepad));
    
        return v;
    }

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
    input::Keyboard _keyboard;
    std::unordered_map<SDL_JoystickID, input::Gamepad> _gamepads;
};
