#pragma once
#include "renderer.hpp"
#include "../fight/scene.hpp"
#include "TextureAtlas.hpp"



namespace renderer
{

    class FightRenderer : public _Renderer<fight::State> 
    {
    public:
        FightRenderer(SDL_Window* const window, SDL_Renderer* const renderer, const fight::Scene& scene) :
            _Renderer(window, renderer),
            _scene(scene)
        {
            _atlas.load(_sdlRenderer, ASSET_DIR "Atlas/atlas.bmp", ASSET_DIR "Atlas/atlas.xml");
            _bgAtlas.load(_sdlRenderer, ASSET_DIR "Atlas/bgAtlas.bmp", ASSET_DIR "Atlas/bgAtlas.xml");
        }

        ~FightRenderer()
        {
            _atlas.unload();
            _bgAtlas.unload();
        }

        void pushState(State state)
            { this->_state = state; }

        void render()
        {
            // reset drawing
			SDL_SetRenderDrawColor(_sdlRenderer, 0, 0, 0, 255);
			SDL_RenderClear(_sdlRenderer);
			SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 255, 255);

			int winw = 640, winh = 480;
			SDL_GetWindowSize(_sdlWindow, &winw, &winh);
			float x, y = winh / 5.0f;
			SDL_FRect screenRect = {0.0f, 0.0f, (float)winw, (float)winh};

            auto& level = _scene.getLevel(_state.levelIndex);

            // backdrop
            _bgAtlas.draw(_sdlRenderer, "daySky", &screenRect);

            // background
            std::string tsname(fight::StageName(_scene.getStage()));
            tsname[0] = std::tolower(tsname[0]);
            tsname.erase(std::remove_if(tsname.begin(), tsname.end(), isspace), tsname.end());
            for (int y = 0; y < level.height; y++)
                for (int x = 0; x < level.width; x++)
                    if (level._backgroundBits[y] & (1ul << x))
                        _atlas.drawTile(_sdlRenderer, "tilesets/" + tsname + "BG", 
                            level._backgroundTiles[level.width * y + x], x, y);

            // foreground
            for (int y = 0; y < level.height; y++)
                for (int x = 0; x < level.width; x++)
                    if (level._solidBits[y] & (1ul << x))
                        _atlas.drawTile(_sdlRenderer, "tilesets/" + tsname,
                            level._solidTiles[level.width * y + x], x, y);

            SDL_RenderPresent(_sdlRenderer);
        }

    private:
        const fight::Scene& _scene;
        TextureAtlas _atlas;
    	TextureAtlas _bgAtlas;
        State _state;
    };

};  // end namespace renderer
