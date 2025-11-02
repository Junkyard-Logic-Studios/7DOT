#pragma once
#include <memory>
#include "SDL3/SDL_render.h"



namespace renderer
{

    template<typename ST>
    class _Renderer
    {
    public:
        virtual void pushState(ST state) = 0;
        virtual void render(
                const std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>& window,
                const std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>& renderer
            ) = 0;
    
    protected:
        using State = ST;
    };

};  // end namespace renderer
