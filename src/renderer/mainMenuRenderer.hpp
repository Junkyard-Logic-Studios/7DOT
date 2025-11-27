#pragma once
#include "renderer.hpp"
#include "../core/mainMenuScene.hpp"
#include "TextureAtlas.hpp"

namespace renderer
{

	class MainMenuRenderer : public _Renderer<core::MainMenuScene::State>
	{
	public:
		TextureAtlas atlas;
		void pushState(State state)
		{
			this->_state = state;
		}

		void render(
				const std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> &window,
				const std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> &renderer)
		{
			// reset drawing
			SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
			SDL_RenderClear(renderer.get());
			SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);

			int winw = 640, winh = 480;
			SDL_GetWindowSize(window.get(), &winw, &winh);
			float x, y = winh / 3.0f;
			SDL_FRect screenRect = {0.0f, 0.0f, (float)winw, (float)winh};

			// function to write a line of debug text
			auto fWriteLine = [&](const char *text, bool cursor = false)
			{
				x = (winw - SDL_strlen(text) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2.0f;
				if (cursor)
					SDL_RenderDebugText(renderer.get(), x - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2.0, y, ">");
				SDL_RenderDebugText(renderer.get(), x, y, text);
				y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4.0f;
			};

			// Asset loading (inefficient to do every frame, but okay for prototype)
			atlas.load(renderer.get(), "../resources/assets/Atlas/bgAtlas.bmp", "../resources/assets/Atlas/bgAtlas.xml");

			// draw from current state
			using opt = core::MainMenuScene::NavigationOptions;
			switch (_state.currentLevel)
			{
			case opt::TITLE:
				if (_state.selected == opt::TITLE)
				{
					atlas.draw(renderer.get(), "distantSky", &screenRect);
					fWriteLine("start");
				}
				else
				{
					atlas.draw(renderer.get(), "capitolBack", &screenRect);
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
					SDL_RenderDebugText(renderer.get(), x, y, text);
					y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2.0f;

					SDL_free(text);
				}
				y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4.0f;
				fWriteLine("Back", _state.selected == opt::OPTIONS_BACK);
				break;
			}

			SDL_RenderPresent(renderer.get());
		}

	private:
		State _state;
	};

}; // end namespace renderer
