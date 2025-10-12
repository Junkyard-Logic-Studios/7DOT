#include <memory>
#include <stdexcept>
#include <unordered_map>
#include "input/input.hpp"
#include "SDL3/SDL.h"


class SDLException final : public std::runtime_error
{
public:
	explicit SDLException(const std::string& message) :
		std::runtime_error(message + ": " + SDL_GetError())
	{}
};


class Game
{
public:
	Game()
	{
		if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
			throw SDLException("Failed to init SDL");

		SDL_Window* window;
		SDL_Renderer* renderer;
		if (!SDL_CreateWindowAndRenderer("Game", 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN, &window, &renderer))
			throw SDLException("Failed to create window and renderer");

		_window.reset(window);
		_renderer.reset(renderer);

		SDL_ShowWindow(_window.get());
	}

	~Game()
	{
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
						if (auto* pGamepad = SDL_OpenGamepad(event.gdevice.which); pGamepad)
							_gamepads.emplace(event.gdevice.which, pGamepad);
					break;

				case SDL_EVENT_GAMEPAD_REMOVED:
					if (auto it = _gamepads.find(event.gdevice.which); it != _gamepads.end())
						_gamepads.erase(it);
					break;
			}
		}

		// render
		int winw = 640, winh = 480;
		float x, y;

		SDL_SetRenderDrawColor(_renderer.get(), 0, 0, 0, 255);
		SDL_RenderClear(_renderer.get());
		SDL_GetWindowSize(_window.get(), &winw, &winh);
		SDL_SetRenderDrawColor(_renderer.get(), 255, 255, 255, 255);

		{
			const char* text = _gamepads.empty() ? "Plug in a gamepad, please." : "Connected gamepads:";
	
			x = (winw - SDL_strlen(text) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2.0f;
			y = winh / 3.0f;
			SDL_RenderDebugText(_renderer.get(), x, y, text);
		}

		for (auto& [id, gamepad] : _gamepads)
		{
			char* text;
			input::PlayerInput playerInput = gamepad.poll();
			SDL_asprintf(&text, "%d: %s [%f, %f, %d, %d, %d, %d, %d, %d]", id, gamepad.getName(), 
				input::get::horizontalAxis(playerInput),
				input::get::verticalAxis(playerInput),
				input::get::start(playerInput),	
				input::get::shoot(playerInput),	
				input::get::jump(playerInput),
				input::get::toggle(playerInput),
				input::get::cancel(playerInput),
				input::get::dashCount(playerInput)
			);

			x = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4.0f;
			y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2.0f;
			SDL_RenderDebugText(_renderer.get(), x, y, text);

			free(text);
		}

		SDL_RenderPresent(_renderer.get());	

		return true;
	}

private:
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> _window {nullptr, SDL_DestroyWindow};
	std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> _renderer {nullptr, SDL_DestroyRenderer};

	std::unordered_map<SDL_JoystickID, input::Gamepad> _gamepads;
};


int main()
{
	try
	{
		Game game;
		while (game.update());
	}
	catch (const std::exception& e)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: %s", e.what());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", e.what(), nullptr);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
