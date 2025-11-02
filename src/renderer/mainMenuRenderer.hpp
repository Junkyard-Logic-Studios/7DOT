#pragma once
#include "renderer.hpp"
#include "../core/mainMenuScene.hpp"



namespace renderer
{

    class MainMenuRenderer : public _Renderer<core::MainMenuScene::State> 
    {
    public:
        void pushState(State state)
            { this->_state = state; }

        void render(
            const std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>& window,
            const std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>& renderer
        ) {
            int winw = 640, winh = 480;
            float x, y;

            SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
            SDL_RenderClear(renderer.get());
            SDL_GetWindowSize(window.get(), &winw, &winh);
            SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);

            {
                const char *text = _state.inputDevicePolls.empty() ? "Plug in a gamepad, please." : "Connected gamepads:";

                x = (winw - SDL_strlen(text) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2.0f;
                y = winh / 3.0f;
                SDL_RenderDebugText(renderer.get(), x, y, text);
            }

            for (auto& [name, playerInput] : _state.inputDevicePolls)
            {
                char *text;
                SDL_asprintf(&text, "%s: [%f, %f, %d, %d, %d, %d, %d, %d]", name,
                    input::get::horizontalAxis(playerInput),
                    input::get::verticalAxis(playerInput),
                    input::get::start(playerInput),
                    input::get::shoot(playerInput),
                    input::get::jump(playerInput),
                    input::get::toggle(playerInput),
                    input::get::cancel(playerInput),
                    input::get::dashCount(playerInput));

                x = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4.0f;
                y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2.0f;
                SDL_RenderDebugText(renderer.get(), x, y, text);

                SDL_free(text);
            }

            SDL_RenderPresent(renderer.get());
        }

    private:
        State _state;
    };

};  // end namespace renderer
