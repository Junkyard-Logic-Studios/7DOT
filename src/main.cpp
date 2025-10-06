#include <memory>
#include <stdexcept>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>


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
		if (not SDL_Init(SDL_INIT_VIDEO))
			throw SDLException("Failed to init SDL");
		if (not SDL_Vulkan_LoadLibrary(nullptr))
			throw SDLException("Failed to load Vulkan");

		auto* window = SDL_CreateWindow("Game", 800, 600,
			SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
		if (not window)
			throw SDLException("Failed to create window");

		_window.reset(window);

	}

	~Game()
	{
		SDL_Quit();
	}

	void run()
	{
		SDL_ShowWindow(_window.get());
		_running = true;
		while (_running)
		{
			for (SDL_Event event; SDL_PollEvent(&event);)
			{
				if (event.type == SDL_EVENT_QUIT)
					_running = false;
			}
		}
	}

private:
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> _window {nullptr, SDL_DestroyWindow};
	bool _running = false;
};


int main()
{
	try
	{
		Game game {};
		game.run();
	}
	catch (const std::exception& e)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: %s", e.what());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", e.what(), nullptr);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
