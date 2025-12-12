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
                switch (_state.fightSelection.mode)
                {
                case core::FightSelection::Mode::LAST_MAN_STANDING:
                    fWriteLine("< Last Man Standing >");  break;
                case core::FightSelection::Mode::HEAD_HUNTERS:
                    fWriteLine("< Head Hunters >");       break;
                case core::FightSelection::Mode::TEAM_2:
                    fWriteLine("< 2 Teams Deathmatch >"); break;
                case core::FightSelection::Mode::TEAM_4:
                    fWriteLine("< 4 Teams Deathmatch >"); break;
                };
                break;
            
            case opt::TEAM:
                fWriteLine("Teams");
                fWriteTeam("Team 1", 0);
                fWriteTeam("Team 2", 1);
                if (_state.fightSelection.mode == core::FightSelection::Mode::TEAM_4)
                {
                    fWriteTeam("Team 3", 2);
                    fWriteTeam("Team 4", 3);
                }
                for (auto& player : _state.fightSelection.players)
                {
                    char* text;
                    SDL_asprintf(&text, "device (host: %d, local: %d)", 
                        player.hostID, player.deviceID);
                    fWriteTeam(text, player.team);
					SDL_free(text);
                }
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
