#pragma once
#include <memory>
#include "SDL3/SDL_render.h"



namespace renderer
{

    template<typename ST>
    class _Renderer
    {
    public:
        _Renderer(SDL_Window* const window, SDL_Renderer* const renderer) :
            _sdlWindow(window), _sdlRenderer(renderer)
        {}

        virtual void pushState(ST state) = 0;
        virtual void render() = 0;
    
    protected:
        using State = ST;
        SDL_Window* const _sdlWindow;
        SDL_Renderer* const _sdlRenderer;
    };

};  // end namespace renderer
