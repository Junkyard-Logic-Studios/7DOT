#pragma once
#include <stdexcept>
#include <string> // Ich brauche das (MAC) weil sonst weine ich
#include "SDL3/SDL_error.h"



class SDLException final : public std::runtime_error
{
public:
	explicit SDLException(const char *message) : 
        std::runtime_error(std::string(message) + ": " + SDL_GetError()) 
    {}
};
