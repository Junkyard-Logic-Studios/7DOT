#include "game.hpp"

int main()
{
	try
	{
		Game game;
		while (game.update())
			;
	}
	catch (const std::exception &e)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: %s", e.what());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", e.what(), nullptr);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
