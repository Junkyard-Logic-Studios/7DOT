#pragma once
#include "renderer.hpp"
#include "../selection/scene.hpp"
#include "ArcherCatalog.hpp"
#include "SpriteCatalog.hpp"
#include "TextureAtlas.hpp"
#include "../fight/mode.hpp"
#include "../fight/stage.hpp"
#include <string>

namespace renderer
{

    class SelectionRenderer : public _Renderer<selection::State>
    {
    public:
        SelectionRenderer(SDL_Window *const window, SDL_Renderer *const renderer) : _Renderer(window, renderer)
        {
            _menuAtlas.load(_sdlRenderer, ASSET_DIR "Atlas/menuAtlas.bmp", ASSET_DIR "Atlas/menuAtlas.xml");
            _atlas.load(_sdlRenderer, ASSET_DIR "Atlas/atlas.bmp", ASSET_DIR "Atlas/atlas.xml");
            _spriteCatalog.load(ASSET_DIR "Atlas/SpriteData/spriteData.xml");
            _archerCatalog.load(
                ASSET_DIR "Atlas/GameData/archerData.xml",
                _spriteCatalog,
                [&](const std::string &texture)
                { return _atlas.getRect(texture) != nullptr; });
        }

        ~SelectionRenderer()
        {
            _menuAtlas.unload();
            _atlas.unload();
        }

        void pushState(State state)
        {
            this->_state = state;
        }

        void render()
        {
            using opt = selection::NavigationOptions;

            // reset drawing
            SDL_SetRenderDrawColor(_sdlRenderer, 0, 0, 0, 255);
            SDL_RenderClear(_sdlRenderer);
            SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 255, 255);

            int winw = 640, winh = 480;
            SDL_GetWindowSize(_sdlWindow, &winw, &winh);
            float x, y = winh / 5.0f;
            SDL_FRect screenRect = {0.0f, 0.0f, (float)winw, (float)winh};

            // function to write a line of debug text
            auto fWriteLine = [&](const char *text)
            {
                x = (winw - SDL_strlen(text) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2.0f;
                SDL_RenderDebugText(_sdlRenderer, x, y, text);
                y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4.0f;
            };

            // write a line of debug text into a specific team bracket
            float ys[] = {winh / 3.f, winh / 3.f, 2 * winh / 3.f, 2 * winh / 3.f};
            auto fWriteTeam = [&](const char *text, uint8_t team)
            {
                x = (team & 1 ? winw * .75f : winw * .25f) -
                    SDL_strlen(text) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE / 2.0f;
                SDL_RenderDebugText(_sdlRenderer, x, ys[team], text);
                ys[team] += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2.0f;
            };

            // background
            switch (_state.currentLevel)
            {
            case opt::CHARACTERS:
            case opt::MODE:
            case opt::TEAM:
                SDL_FRect R;
                R = {0.0f, 0.0f, (float)winw, winh * .5f};
                _menuAtlas.draw(_sdlRenderer, "titleSky", &R);
                R = {0.0f, (float)winh, (float)winw, -winh * .5f};
                _menuAtlas.draw(_sdlRenderer, "titleSky", &R);
                R = {0.0f, 0.0f, (float)winw, (float)winh};
                _menuAtlas.draw(_sdlRenderer, "mmg/bgGlow", &R);
                break;

            case opt::STAGE:
                R = {0.0f, 0.0f, (float)winw, (float)winh};
                _menuAtlas.draw(_sdlRenderer, "mapWater", &R);
                _menuAtlas.draw(_sdlRenderer, "mapLand", &R);
                break;
            }

            // foreground
            switch (_state.currentLevel)
            {
            case opt::CHARACTERS:
                fWriteLine("[ Characters ]");
                for (auto &player : _state.players)
                {
                    const ArcherSkin *skin = previewSkin(player.character);
                    std::string skinName = skin ? skin->name : "missing";

                    char *text;
                    SDL_asprintf(&text, "device (host: %d, local: %d)",
                                 player.hostID, player.deviceID);
                    x = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4.0f;
                    float lineY = y;
                    SDL_RenderDebugText(_sdlRenderer, x, lineY, text);

                    SDL_free(text);

                    SDL_asprintf(&text, "character: < %u >  %s",
                                 renderer::ArcherCatalog::wrapCharacter(player.character, _archerCatalog.validBaseCount()),
                                 skinName.c_str());
                    SDL_RenderDebugText(_sdlRenderer, x, lineY + SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2.0f, text);
                    SDL_free(text);

                    drawCharacterPreview(player, x + 350.0f, lineY + 58.0f, 3.0f);
                    y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 9.0f;
                }
                y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4.0f;
                break;

            case opt::MODE:
                fWriteLine("[ Mode ]");
                {
                    char temp[32];
                    SDL_snprintf(temp, 32, "< %s >", fight::ModeName(_state.mode));
                    fWriteLine(temp);
                }
                break;

            case opt::TEAM:
                fWriteLine("[ Teams ]");
                fWriteTeam("Team 1", 0);
                fWriteTeam("Team 2", 1);
                if (_state.mode == fight::Mode::TEAM_4)
                {
                    fWriteTeam("Team 3", 2);
                    fWriteTeam("Team 4", 3);
                }
                for (auto &player : _state.players)
                {
                    char *text;
                    SDL_asprintf(&text, "device (host: %d, local: %d)",
                                 player.hostID, player.deviceID);
                    fWriteTeam(text, player.team);
                    SDL_free(text);
                }
                break;

            case opt::STAGE:
                y = winh * 0.8f;
                fWriteLine("[ Stage ]");
                {
                    char temp[32];
                    SDL_snprintf(temp, 32, "< %s >", fight::stageToName(_state.stage));
                    fWriteLine(temp);
                }
                break;
            }

            SDL_RenderPresent(_sdlRenderer);
        }

    private:
        static constexpr float HEAD_X_OFFSET = 0.0f;
        static constexpr float HEAD_Y_OFFSET = 0.0f;
        static constexpr float BOW_X_OFFSET = 0.0f;
        static constexpr float BOW_Y_OFFSET = 0.0f;

        const ArcherSkin *previewSkin(unsigned int character) const
        {
            if (_archerCatalog.validBaseCount() == 0)
                return nullptr;

            return &_archerCatalog.skinForCharacter(character);
        }

        int previewAnimationFrame(const SpriteDef &sprite, const std::string &animationId) const
        {
            const SpriteAnimation *animation = sprite.animation(animationId);
            if (!animation || animation->frames.empty())
                return 0;

            return animation->frames.front();
        }

        int frameOrigin(const std::vector<int> &origins, int frame, int fallback) const
        {
            if (frame >= 0 && static_cast<std::size_t>(frame) < origins.size())
                return origins[frame];

            return fallback;
        }

        float previewPartX(const SpriteDef &sprite, float anchorX, float scale,
            bool fliphoriz, float localXOffset) const
        {
            float localX = sprite.x + localXOffset;
            if (fliphoriz)
                return anchorX - (localX + sprite.frameWidth - sprite.originX) * scale;

            return anchorX + (localX - sprite.originX) * scale;
        }

        void drawPreviewFrame(
            const SpriteDef &sprite,
            int frame,
            float anchorX,
            float anchorY,
            float scale,
            bool fliphoriz,
            bool hasYOverride = false,
            float yOverride = 0.0f,
            float localXOffset = 0.0f,
            float localYOffset = 0.0f)
        {
            if (sprite.texture.empty())
                return;

            float dstY = hasYOverride ? yOverride : anchorY + (sprite.y + localYOffset - sprite.originY) * scale;
            SDL_FRect dst = {
                previewPartX(sprite, anchorX, scale, fliphoriz, localXOffset),
                dstY,
                sprite.frameWidth * scale,
                sprite.frameHeight * scale};

            _atlas.drawFrame(_sdlRenderer, sprite.texture, sprite.frameWidth, sprite.frameHeight,
                             frame, &dst, fliphoriz);
        }

        void drawPreviewPart(
            const SpriteDef &sprite,
            const std::string &animationId,
            float anchorX,
            float anchorY,
            float scale,
            bool fliphoriz,
            bool hasYOverride = false,
            float yOverride = 0.0f,
            float localXOffset = 0.0f,
            float localYOffset = 0.0f)
        {
            drawPreviewFrame(sprite, previewAnimationFrame(sprite, animationId),
                             anchorX, anchorY, scale, fliphoriz, hasYOverride,
                             yOverride, localXOffset, localYOffset);
        }

        void drawCharacterPreview(const Player &player, float anchorX, float anchorY, float scale)
        {
            const ArcherSkin *skin = previewSkin(player.character);
            if (!skin)
                return;

            const SpriteDef *body = _spriteCatalog.find(skin->bodySprite);
            const SpriteDef *head = _spriteCatalog.find(skin->headNormalSprite);
            const SpriteDef *bow = _spriteCatalog.find(skin->bowSprite);
            if (!body || !head || !bow)
                return;

            constexpr bool fliphoriz = false;
            int bodyFrame = previewAnimationFrame(*body, "stand");
            int headXOrigin = frameOrigin(body->headXOrigins, bodyFrame, body->originX);
            int headYOrigin = frameOrigin(body->headYOrigins, bodyFrame, body->originY);

            auto drawHead = [&](const std::string &spriteId)
            {
                if (spriteId.empty())
                    return;

                const SpriteDef *headPart = _spriteCatalog.find(spriteId);
                if (!headPart)
                    return;

                float headLocalX = headXOrigin - body->originX + HEAD_X_OFFSET;
                float headY = anchorY + (headPart->y - headYOrigin - headPart->originY + HEAD_Y_OFFSET) * scale;
                drawPreviewPart(*headPart, "idle", anchorX, anchorY, scale, fliphoriz, true, headY, headLocalX);
            };

            drawHead(skin->headBackSprite);
            drawPreviewFrame(*body, bodyFrame, anchorX, anchorY, scale, fliphoriz);
            drawHead(skin->headNormalSprite);

            if (!bow->hideBowIdle)
                drawPreviewPart(*bow, "idle", anchorX, anchorY, scale, fliphoriz,
                    false, 0.0f, BOW_X_OFFSET, BOW_Y_OFFSET);
        }

        TextureAtlas _menuAtlas;
        TextureAtlas _atlas;
        SpriteCatalog _spriteCatalog;
        ArcherCatalog _archerCatalog;
        State _state;
    };

}; // end namespace renderer
