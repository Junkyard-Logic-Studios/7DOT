#pragma once
#include "renderer.hpp"
#include "../core/selectionScene.hpp"



namespace renderer
{

    class SelectionRenderer : public _Renderer<core::SelectionScene::State> 
    {
    public:
        SelectionRenderer(SDL_Window* const window, SDL_Renderer* const renderer) :
            _Renderer(window, renderer)
        {}

        void pushState(State state)
        {
            this->_state = state;
        }

        void render()
        {
            // reset drawing
			SDL_SetRenderDrawColor(_sdlRenderer, 0, 0, 0, 255);
			SDL_RenderClear(_sdlRenderer);
			SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 255, 255);

			int winw = 640, winh = 480;
			SDL_GetWindowSize(_sdlWindow, &winw, &winh);
			float x, y = winh / 3.0f;
			SDL_FRect screenRect = {0.0f, 0.0f, (float)winw, (float)winh};

            // function to write a line of debug text
			auto fWriteLine = [&](const char *text)
			{
				x = (winw - SDL_strlen(text) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2.0f;
				SDL_RenderDebugText(_sdlRenderer, x, y, text);
				y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4.0f;
			};

            // draw from current state
            using opt = core::SelectionScene::NavigationOptions;
            switch (_state.currentLevel)
            {
            case opt::CHARACTERS:
                fWriteLine("Characters");
                for (auto& player : _state.fightSelection.players)
                {
                    char* text;
                    SDL_asprintf(&text, "device (host: %d, local: %d)  -  character: < %u >",
                        player.hostID, player.deviceID, player.character);
                    x = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4.0f;
					SDL_RenderDebugText(_sdlRenderer, x, y, text);
					y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2.0f;

					SDL_free(text);
                }
                y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4.0f;
                break;
            
            case opt::MODE:
                fWriteLine("Mode");
                break;

            case opt::STAGE:
                fWriteLine("Stage");
                break;
            }

            SDL_RenderPresent(_sdlRenderer);
        }
    
    private:
        State _state;
    };

};  // end namespace renderer
