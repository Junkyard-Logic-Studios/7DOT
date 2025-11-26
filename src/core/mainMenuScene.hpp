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
        enum NavigationOptions : uint32_t
        {
            TITLE,
            
            PVP,
            SESSION_STATS,
            OPTIONS,
            QUIT,

            PVP_LOCAL,
            PVP_REMOTE,
            PVP_BACK,

            SESSION_STATS_BACK,
            
            OPTIONS_BACK,
        };

        struct State
        {
            std::vector<std::pair<const char*, input::PlayerInput>> inputDevicePolls;

            NavigationOptions currentLevel;
            NavigationOptions selected;
        };

        MainMenuScene(Game& game);
        void activate();
        void deactivate();
        UpdateReturnStatus update();

    private:
        std::unique_ptr<renderer::_Renderer<MainMenuScene::State>> _renderer;
        State _state = { {}, TITLE, TITLE };
    };

};  // end namespace core
