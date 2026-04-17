#pragma once
#include "renderer.hpp"
#include "../fight/scene.hpp"
#include "ArcherCatalog.hpp"
#include "SpriteCatalog.hpp"
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
        std::string chooseBodyAnimation(const fight::Archer& archer) const;
        std::string chooseHeadAnimation(const std::string& bodyAnimation) const;
        int animationFrame(const SpriteDef& sprite, const std::string& animationId) const;
        int frameOrigin(const std::vector<int>& origins, int frame, int fallback) const;
        const std::string& textureFor(const SpriteDef& sprite, const Player& player) const;
        int drawSpritePart(const SpriteDef& sprite, const std::string& animationId,
            const glm::vec2& worldPosition, const Player& player, bool fliphoriz,
            float worldY, float localXOffset = 0.0f, float localYOffset = 0.0f);
        void drawArcher(const fight::Archer& archer, const Player& player);
        void drawArcherDebug(const fight::Archer& archer);
        void drawDevBuildText(int winw);

        const fight::Scene& _scene;
        TextureAtlas _atlas;
    	TextureAtlas _bgAtlas;
        SpriteCatalog _spriteCatalog;
        ArcherCatalog _archerCatalog;
        State _state;
    };

};  // end namespace renderer
