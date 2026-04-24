#pragma once
#include "renderer.hpp"
#include "../fight/scene.hpp"
#include "ArcherCatalog.hpp"
#include "FightTilemapCache.hpp"
#include "SpriteCatalog.hpp"
#include "TextureAtlas.hpp"
#include "ViewportLayout.hpp"



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
        std::string tilesetName() const;
        void ensureWorldTexture(int width, int height);
        void ensureTilemapCache(const fight::Level& level, const FightTilemapCacheKey& key);
        void rebuildTilemapCache(const fight::Level& level, const FightTilemapCacheKey& key);
        void destroyTexture(SDL_Texture*& texture);
        void drawDeadSpace(const ViewportLayout& layout);
        void drawFillerCoverOutward(TextureAtlas& atlas, const std::string& asset,
            const SDL_FRect& rect, int anchorX, int anchorY);

        const fight::Scene& _scene;
        TextureAtlas _atlas;
    	TextureAtlas _bgAtlas;
        TextureAtlas _menuAtlas;
        SpriteCatalog _spriteCatalog;
        ArcherCatalog _archerCatalog;
        FightTilemapCacheState _tilemapCacheState;
        SDL_Texture* _worldTexture = nullptr;
        SDL_Texture* _tilemapTexture = nullptr;
        int _worldWidth = 0;
        int _worldHeight = 0;
        State _state;
    };

};  // end namespace renderer
