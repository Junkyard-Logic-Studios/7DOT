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
        FightRenderer(SDL_Window* const window, SDL_Renderer* const renderer) :
            _Renderer(window, renderer)
        {
            _bgAtlas.load(_sdlRenderer, ASSET_DIR "Atlas/bgAtlas.bmp", ASSET_DIR "Atlas/bgAtlas.xml");
        }

        ~FightRenderer()
        {
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

            // background
            switch (_fightSelection.stage)
            {
            case FightSelectionInfo::Stage::TOWER:
                _bgAtlas.draw(_sdlRenderer, "daySky", &screenRect);
                break;
            
            case FightSelectionInfo::Stage::CAVE:
                _bgAtlas.draw(_sdlRenderer, "CavesBack", &screenRect);
                break;

            case FightSelectionInfo::Stage::CASTLE:
                _bgAtlas.draw(_sdlRenderer, "desertDusk", &screenRect);
                break;
            }

            SDL_RenderPresent(_sdlRenderer);
        }

    private:
        FightSelectionInfo _fightSelection;
    	TextureAtlas _bgAtlas;
        State _state;
    };

};  // end namespace renderer
