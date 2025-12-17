#pragma once
#include "renderer.hpp"
#include "../mainmenu/scene.hpp"
#include "TextureAtlas.hpp"



namespace renderer
{

	class MainMenuRenderer : public _Renderer<mainmenu::State>
	{
	public:
		MainMenuRenderer(SDL_Window *const window, SDL_Renderer *const renderer) : 
			_Renderer(window, renderer)
		{
			_bgAtlas.load(_sdlRenderer, ASSET_DIR "Atlas/bgAtlas.bmp", ASSET_DIR "Atlas/bgAtlas.xml");
			_menuAtlas.load(_sdlRenderer, ASSET_DIR "Atlas/menuAtlas.bmp", ASSET_DIR "Atlas/menuAtlas.xml");
		}

		~MainMenuRenderer()
		{
			_bgAtlas.unload();
			_menuAtlas.unload();
		}

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
			auto fWriteLine = [&](const char *text, bool cursor = false)
			{
				x = (winw - SDL_strlen(text) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2.0f;
				if (cursor)
					_menuAtlas.draw(_sdlRenderer, "title/darkWorld/arrow", winw / 2 + 157, y - 21, 1.0f, true);
				SDL_RenderDebugText(_sdlRenderer, x, y, text);
				y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4.0f;
			};

			// draw from current state
			using opt = mainmenu::NavigationOptions;
			switch (_state.currentLevel)
			{
			case opt::TITLE:
				if (_state.selected == opt::TITLE)
				{
					_bgAtlas.draw(_sdlRenderer, "distantSky", &screenRect);
					fWriteLine("start");
				}
				else
				{
					_bgAtlas.draw(_sdlRenderer, "CavesBack", &screenRect);
					fWriteLine("PVP", _state.selected == opt::PVP);
					fWriteLine("Session Stats", _state.selected == opt::SESSION_STATS);
					fWriteLine("Options", _state.selected == opt::OPTIONS);
					fWriteLine("Quit", _state.selected == opt::QUIT);
				}
				break;

			case opt::PVP:
				fWriteLine("Local", _state.selected == opt::PVP_LOCAL);
				fWriteLine("Remote", _state.selected == opt::PVP_REMOTE);
				fWriteLine("Back", _state.selected == opt::PVP_BACK);
				break;

			case opt::SESSION_STATS:
				fWriteLine("Back", _state.selected == opt::SESSION_STATS_BACK);
				break;

			case opt::OPTIONS:
				fWriteLine(_state.inputDevicePolls.empty() ? "Plug in a gamepad, please." : "Connected gamepads:");
				for (auto &[name, playerInput] : _state.inputDevicePolls)
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
					SDL_RenderDebugText(_sdlRenderer, x, y, text);
					y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2.0f;

					SDL_free(text);
				}
				y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4.0f;
				fWriteLine("Back", _state.selected == opt::OPTIONS_BACK);
				break;
			}

			SDL_RenderPresent(_sdlRenderer);
		}

	private:
		TextureAtlas _bgAtlas;
		TextureAtlas _menuAtlas;
		State _state;
	};

}; // end namespace renderer
