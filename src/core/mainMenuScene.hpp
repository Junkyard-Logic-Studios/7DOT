#pragma once
#include <vector>
#include "scene.hpp"
#include "../renderer/renderer.hpp"
#include "../input/input.hpp"



namespace core
{

    // first scene launched when starting application
    // separate for every client
    class MainMenuScene : public _Scene
    {
    public:
        enum NavigationOptions
        {
            TITLE,
            PVP,
            SESSION_STATS,
            OPTIONS,
            QUIT
        };

        struct State
        {
            std::vector<std::pair<const char*, input::PlayerInput>> inputDevicePolls;

            NavigationOptions currentLevel;
            NavigationOptions selected;
        };

        MainMenuScene(std::shared_ptr<Game> game);

        void update();

    private:
        std::unique_ptr<renderer::_Renderer<MainMenuScene::State>> _renderer;
        State _state = { {}, TITLE, TITLE };
    };

};  // end namespace core
