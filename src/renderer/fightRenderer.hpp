#pragma once
#include "renderer.hpp"
#include "../fight/scene.hpp"
#include "TextureAtlas.hpp"
#include "../fightSelectionInfo.hpp"



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

        void setFightSelectionInfo(FightSelectionInfo fightSelection)
            { this->_fightSelection = fightSelection; }

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

            // backdrop
            switch (_fightSelection.stage)
            {
            //case FightSelectionInfo::Stage::TOWER:
            //    _bgAtlas.draw(_sdlRenderer, "daySky", &screenRect);
            //    break;
            //
            //case FightSelectionInfo::Stage::CAVE:
            //    _bgAtlas.draw(_sdlRenderer, "CavesBack", &screenRect);
            //    break;
            //
            //case FightSelectionInfo::Stage::CASTLE:
            //    _bgAtlas.draw(_sdlRenderer, "desertDusk", &screenRect);
            //    break;
            default: 
                _bgAtlas.draw(_sdlRenderer, "daySky", &screenRect);
                break;
            }

            auto& level = _scene.getLevel(_state.levelIndex);

            // background
            
            for (int y = 0; y < level.height; y++)
                for (int x = 0; x < level.width; x++)
                    if (level._backgroundBits[y] & (1ul << x))
                        _atlas.drawTile(
                            _sdlRenderer, 
                            "tilesets/sacredGroundBG", 
                            level._backgroundTiles[level.width * y + x],
                            x * TILESIZE,
                            y * TILESIZE
                        );

            SDL_RenderPresent(_sdlRenderer);
        }

    private:
        const fight::Scene& _scene;
        FightSelectionInfo _fightSelection;
        TextureAtlas _atlas;
    	TextureAtlas _bgAtlas;
        State _state;
    };

};  // end namespace renderer
