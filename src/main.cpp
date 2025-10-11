#include <memory>
#include <stdexcept>
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
			if (event.type == SDL_EVENT_QUIT)
				return false;

			if (event.type == SDL_EVENT_GAMEPAD_ADDED)
			{
				if (_gamepad) continue; 	// already assigned

				auto* gamepad = SDL_OpenGamepad(event.gdevice.which);
				if (!gamepad)
					throw SDLException("Failed to open gamepad");

				_gamepad.reset(gamepad);
			}

			if (event.type == SDL_EVENT_GAMEPAD_REMOVED)
			{
				if (_gamepad && (SDL_GetGamepadID(_gamepad.get()) == event.gdevice.which))
					_gamepad.release();
			}
		}

		// render
		int winw = 640, winh = 480;
		const char* text = "Plug in a joystick, please.";
		float x, y;

		if (_gamepad)
			text = SDL_GetGamepadName(_gamepad.get());

		SDL_SetRenderDrawColor(_renderer.get(), 0, 0, 0, 255);
		SDL_RenderClear(_renderer.get());
		SDL_GetWindowSize(_window.get(), &winw, &winh);

		x = (((float) winw) - (SDL_strlen(text) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)) / 2.0f;
		y = (((float) winh) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2.0f;
		SDL_SetRenderDrawColor(_renderer.get(), 255, 255, 255, 255);
		SDL_RenderDebugText(_renderer.get(), x, y, text);
		SDL_RenderPresent(_renderer.get());

		return true;
	}

private:
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> _window {nullptr, SDL_DestroyWindow};
	std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> _renderer {nullptr, SDL_DestroyRenderer};
	std::unique_ptr<SDL_Gamepad, decltype(&SDL_CloseGamepad)> _gamepad {nullptr, SDL_CloseGamepad};
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
