#pragma once
#include "renderer.hpp"
#include "../fight/scene.hpp"
#include "TextureAtlas.hpp"



namespace renderer
{

    class FightRenderer : public _Renderer<fight::State> 
    {
    public:
        FightRenderer(SDL_Window* const window, SDL_Renderer* const renderer, const fight::Scene& scene);
        ~FightRenderer();
        void pushState(State state);
        void render();

    private:
        const fight::Scene& _scene;
        TextureAtlas _atlas;
    	TextureAtlas _bgAtlas;
        State _state;
    };

};  // end namespace renderer
