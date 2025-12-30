#pragma once
#include "renderer.hpp"
#include "../selection/scene.hpp"
#include "TextureAtlas.hpp"
#include "../fight/mode.hpp"
#include "../fight/stage.hpp"



namespace renderer
{

    class SelectionRenderer : public _Renderer<selection::State> 
    {
    public:
        SelectionRenderer(SDL_Window* const window, SDL_Renderer* const renderer) :
            _Renderer(window, renderer)
        {
			_menuAtlas.load(_sdlRenderer, ASSET_DIR "Atlas/menuAtlas.bmp", ASSET_DIR "Atlas/menuAtlas.xml");
        }

        ~SelectionRenderer()
        {
			_menuAtlas.unload();
        }

        void pushState(State state)
        {
            this->_state = state;
        }

        void render()
        {
            using opt = selection::NavigationOptions;

            // reset drawing
			SDL_SetRenderDrawColor(_sdlRenderer, 0, 0, 0, 255);
			SDL_RenderClear(_sdlRenderer);
			SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 255, 255);

			int winw = 640, winh = 480;
			SDL_GetWindowSize(_sdlWindow, &winw, &winh);
			float x, y = winh / 5.0f;
			SDL_FRect screenRect = {0.0f, 0.0f, (float)winw, (float)winh};

            // function to write a line of debug text
			auto fWriteLine = [&](const char *text)
			{
				x = (winw - SDL_strlen(text) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2.0f;
				SDL_RenderDebugText(_sdlRenderer, x, y, text);
				y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4.0f;
			};

            // write a line of debug text into a specific team bracket
            float ys[] = { winh / 3.f, winh / 3.f, 2 * winh / 3.f, 2 * winh / 3.f };
            auto fWriteTeam = [&](const char* text, uint8_t team)
            {
                x = (team & 1 ? winw * .75f : winw * .25f) - 
                    SDL_strlen(text) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE / 2.0f;
                SDL_RenderDebugText(_sdlRenderer, x, ys[team], text);
                ys[team] += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2.0f;
            };

            // background
            switch (_state.currentLevel)
            {
            case opt::CHARACTERS:
            case opt::MODE:
            case opt::TEAM:
				SDL_FRect R;
				R = {0.0f, 0.0f,        (float)winw,  winh * .5f};
				_menuAtlas.draw(_sdlRenderer, "titleSky", &R);
				R = {0.0f, (float)winh, (float)winw, -winh * .5f};
				_menuAtlas.draw(_sdlRenderer, "titleSky", &R);
                R = {0.0f, 0.0f, (float)winw, (float)winh};
                _menuAtlas.draw(_sdlRenderer, "mmg/bgGlow", &R);
                break;

            case opt::STAGE:
                R = {0.0f, 0.0f, (float)winw, (float)winh};
                _menuAtlas.draw(_sdlRenderer, "mapWater", &R);
                _menuAtlas.draw(_sdlRenderer, "mapLand", &R);
                break;
            }

            // foreground
            switch (_state.currentLevel)
            {
            case opt::CHARACTERS:
                fWriteLine("[ Characters ]");
                for (auto& player : _state.players)
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
                fWriteLine("[ Mode ]");
                {
                    char temp[32];
                    SDL_snprintf(temp, 32, "< %s >", fight::ModeName(_state.mode));
                    fWriteLine(temp);
                }
                break;
            
            case opt::TEAM:
                fWriteLine("[ Teams ]");
                fWriteTeam("Team 1", 0);
                fWriteTeam("Team 2", 1);
                if (_state.mode == fight::Mode::TEAM_4)
                {
                    fWriteTeam("Team 3", 2);
                    fWriteTeam("Team 4", 3);
                }
                for (auto& player : _state.players)
                {
                    char* text;
                    SDL_asprintf(&text, "device (host: %d, local: %d)", 
                        player.hostID, player.deviceID);
                    fWriteTeam(text, player.team);
					SDL_free(text);
                }
                break;

            case opt::STAGE:
                y = winh * 0.8f;
                fWriteLine("[ Stage ]");
                {
                    char temp[32];
                    SDL_snprintf(temp, 32, "< %s >", fight::stageToName(_state.stage));
                    fWriteLine(temp);
                }
                break;
            }

            SDL_RenderPresent(_sdlRenderer);
        }
    
    private:
		TextureAtlas _menuAtlas;
        State _state;
    };

};  // end namespace renderer
