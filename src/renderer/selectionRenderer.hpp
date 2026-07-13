#pragma once
#include "renderer.hpp"
#include "../selection/scene.hpp"
#include "ArcherCatalog.hpp"
#include "StageMapCatalog.hpp"
#include "StageMapView.hpp"
#include "SpriteCatalog.hpp"
#include "TextureAtlas.hpp"
#include "ViewportLayout.hpp"
#include "../charactereditor/CharacterStore.hpp"
#include "../fight/mode.hpp"
#include "../fight/stage.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace renderer
{

    class SelectionRenderer : public _Renderer<selection::State>
    {
    public:
        SelectionRenderer(SDL_Window *const window, SDL_Renderer *const renderer) : _Renderer(window, renderer)
        {
            _menuAtlas.load(_sdlRenderer, ASSET_DIR "Atlas/menuAtlas.bmp", ASSET_DIR "Atlas/menuAtlas.xml");
            _stageMapCatalog.load(ASSET_DIR "Atlas/GameData/themeData.xml");
            _menuSpriteCatalog.load(ASSET_DIR "Atlas/SpriteData/menuSpriteData.xml");
            validateStageMapAtlas();
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
            _customAtlas.unload();
        }

        void pushState(State state)
        {
            this->_state = state;
        }

        void render()
        {
            using opt = selection::NavigationOptions;

            refreshCustomAssets();

            // reset drawing
            SDL_SetRenderDrawColor(_sdlRenderer, 0, 0, 0, 255);
            SDL_RenderClear(_sdlRenderer);
            SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 255, 255);

            int winw = 640, winh = 480;
            SDL_GetWindowSize(_sdlWindow, &winw, &winh);
            float x, y = winh / 5.0f;
            ViewportLayout stageLayout;
            StageMapView stageView;

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
                stageLayout = stageMapLayout(winw, winh);
                stageView = stageMapView(stageLayout);
                applyStageMapColorBoost();
                drawStageMap(stageView);
                break;
            }

            // foreground
            switch (_state.currentLevel)
            {
            case opt::CHARACTERS:
                fWriteLine("[ Characters ]");
                for (auto &player : _state.players)
                {
                    std::string skinName = characterName(player.character);

                    char *text;
                    SDL_asprintf(&text, "device (host: %d, local: %d)",
                                 player.hostID, player.deviceID);
                    x = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4.0f;
                    float lineY = y;
                    SDL_RenderDebugText(_sdlRenderer, x, lineY, text);

                    SDL_free(text);

                    SDL_asprintf(&text, "character: < %u >  %s",
                                 renderer::ArcherCatalog::wrapCharacter(player.character, availableCharacterCount()),
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
                drawStageIcons(stageView);
                resetStageMapColorBoost();
                y = stageLayout.mapRect.y + stageLayout.mapRect.h * 0.8f;
                fWriteLine("[ Stage ]");
                {
                    char temp[32];
                    SDL_snprintf(temp, 32, "< %s >", fight::stageToName(_state.stage));
                    fWriteLine(temp);
                }
                drawStageIconCarousel(stageLayout.mapRect);
                drawStageDeadSpace(stageLayout);
                break;
            }

            SDL_RenderPresent(_sdlRenderer);
        }

    private:
        static constexpr float HEAD_X_OFFSET = 0.0f;
        static constexpr float HEAD_Y_OFFSET = 0.0f;
        static constexpr float BOW_X_OFFSET = 0.0f;
        static constexpr float BOW_Y_OFFSET = 0.0f;
        static constexpr float MAP_SELECTION_ZOOM = 1.5f;
        static constexpr float MAP_SATURATION_BOOST = 1.12f;
        static constexpr float MAP_CAMERA_TRANSITION_SECONDS = 0.35f;
        static constexpr float MAP_CURSOR_TRANSITION_SECONDS = 0.35f;
        static constexpr float MAP_CURSOR_BOB_PERIOD_SECONDS = 1.2f;
        static constexpr float MAP_CURSOR_BOB_PIXELS = 4.0f;
        static constexpr float MAP_CURSOR_TARGET_GAP_PIXELS = 10.0f;
        static constexpr float MAP_WATER_SWAY_SECONDS = 4.0f;
        static constexpr float MAP_WATER_SWAY_PIXELS = 4.0f;
        static constexpr float MAP_WATER_X_OFFSET_PIXELS = 3.0f;
        static constexpr float MAP_WATER_ROW_TIME_OFFSET_SECONDS = 0.435f;
        static constexpr float SUNKEN_CITY_ANIMATION_SPEED = 1.25f;
        static constexpr float PI = 3.14159265358979323846f;

        void validateAtlasSprite(const std::string &name) const
        {
            if (_menuAtlas.getRect(name) == nullptr)
                throw std::runtime_error("Missing menu atlas sprite: " + name);
        }

        void validateMenuSprite(const std::string &name) const
        {
            const SpriteDef *sprite = _menuSpriteCatalog.find(name);
            if (!sprite)
                throw std::runtime_error("Missing menu sprite data: " + name);
            if (_menuAtlas.getRect(sprite->texture) == nullptr)
                throw std::runtime_error("Missing menu atlas sprite: " + sprite->texture);
        }

        void validateStageMapAtlas() const
        {
            validateAtlasSprite("mapWater");
            validateAtlasSprite("mapLand");
            validateAtlasSprite("mapCursor");
            validateAtlasSprite("levelBlock");
            validateAtlasSprite("darkLevelBlock");
            validateMenuSprite("twilightSpire");
            validateMenuSprite("boat");
            validateMenuSprite("sunkenCityMap");
            validateMenuSprite("towerForgeMap");
            validateMenuSprite("ascensionUnlock");
            validateMenuSprite("ghostShipMap");
            validateMenuSprite("dreadwoodMap");
            validateMenuSprite("darkfangMap");
            validateMenuSprite("cataclysmMap");

            for (const auto &entry : _stageMapCatalog.entries())
                validateAtlasSprite(entry.iconAtlasName);
        }

        ViewportLayout stageMapLayout(int winw, int winh) const
        {
            return computeViewportLayout(winw, winh, 320, 240);
        }

        SDL_FPoint lerpPoint(SDL_FPoint from, SDL_FPoint to, float t) const
        {
            return {
                from.x + (to.x - from.x) * t,
                from.y + (to.y - from.y) * t};
        }

        float smoothstep(float t) const
        {
            t = std::clamp(t, 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        SDL_FPoint selectedStageMapPosition() const
        {
            const StageMapEntry *entry = _stageMapCatalog.find(_state.stage);
            return entry ? entry->mapPosition : SDL_FPoint{StageMapView::NATIVE_SIZE * 0.5f, StageMapView::NATIVE_SIZE * 0.5f};
        }

        SDL_FPoint mapCameraCenterAt(Uint64 now) const
        {
            if (!_mapCameraInitialized)
                return selectedStageMapPosition();

            float elapsedSeconds = static_cast<float>(now - _mapCameraStartTicks) / 1000.0f;
            float t = smoothstep(elapsedSeconds / MAP_CAMERA_TRANSITION_SECONDS);
            return lerpPoint(_mapCameraStart, _mapCameraTarget, t);
        }

        SDL_FPoint updateStageMapCamera(Uint64 now)
        {
            SDL_FPoint target = selectedStageMapPosition();
            if (!_mapCameraInitialized)
            {
                _mapCameraStart = target;
                _mapCameraTarget = target;
                _mapCameraStage = _state.stage;
                _mapCameraStartTicks = now;
                _mapCameraInitialized = true;
                return target;
            }

            if (_mapCameraStage != _state.stage)
            {
                _mapCameraStart = mapCameraCenterAt(now);
                _mapCameraTarget = target;
                _mapCameraStage = _state.stage;
                _mapCameraStartTicks = now;
            }

            return mapCameraCenterAt(now);
        }

        SDL_FPoint mapCursorPositionAt(Uint64 now) const
        {
            if (!_mapCursorInitialized)
                return selectedStageMapPosition();

            float elapsedSeconds = static_cast<float>(now - _mapCursorStartTicks) / 1000.0f;
            float t = smoothstep(elapsedSeconds / MAP_CURSOR_TRANSITION_SECONDS);
            return lerpPoint(_mapCursorStart, _mapCursorTarget, t);
        }

        float mapCursorTransitionProgress(Uint64 now) const
        {
            if (!_mapCursorInitialized)
                return 1.0f;

            float elapsedSeconds = static_cast<float>(now - _mapCursorStartTicks) / 1000.0f;
            return std::clamp(elapsedSeconds / MAP_CURSOR_TRANSITION_SECONDS, 0.0f, 1.0f);
        }

        SDL_FPoint updateStageMapCursor(Uint64 now)
        {
            SDL_FPoint target = selectedStageMapPosition();
            if (!_mapCursorInitialized)
            {
                _mapCursorStart = target;
                _mapCursorTarget = target;
                _mapCursorStage = _state.stage;
                _mapCursorStartTicks = now;
                _mapCursorInitialized = true;
                return target;
            }

            if (_mapCursorStage != _state.stage)
            {
                _mapCursorStart = mapCursorPositionAt(now);
                _mapCursorTarget = target;
                _mapCursorStage = _state.stage;
                _mapCursorStartTicks = now;
            }

            return mapCursorPositionAt(now);
        }

        float mapCursorBobOffset(Uint64 now) const
        {
            float transitionProgress = mapCursorTransitionProgress(now);
            if (transitionProgress < 1.0f)
                return 0.0f;

            float elapsedSeconds = static_cast<float>(now - _mapCursorStartTicks) / 1000.0f;
            return std::sin((elapsedSeconds / MAP_CURSOR_BOB_PERIOD_SECONDS) * PI * 2.0f) *
                   MAP_CURSOR_BOB_PIXELS;
        }

        const SpriteAnimation *mapActorAnimation(const SpriteDef &sprite, const std::string &animationId) const
        {
            const SpriteAnimation *animation = sprite.animation(animationId);
            if (animation)
                return animation;

            animation = sprite.animation("idle");
            if (animation)
                return animation;

            return sprite.animations.empty() ? nullptr : &sprite.animations.begin()->second;
        }

        void drawMapSpriteAnimation(
            const std::string &spriteId,
            const std::string &animationId,
            const StageMapView &view,
            float elapsedSeconds,
            bool hasAnchor = false,
            SDL_FPoint anchor = {0.0f, 0.0f},
            float cropLeft = 0.0f,
            float cropTop = 0.0f,
            float cropRight = 0.0f,
            float cropBottom = 0.0f,
            bool anchorIsCenter = false)
        {
            const SpriteDef *sprite = _menuSpriteCatalog.find(spriteId);
            if (!sprite)
                return;

            const SpriteAnimation *animation = mapActorAnimation(*sprite, animationId);
            if (!animation)
                return;

            SDL_FPoint world = hasAnchor ? anchor : SDL_FPoint{static_cast<float>(sprite->x), static_cast<float>(sprite->y)};
            if (hasAnchor && anchorIsCenter)
            {
                world.x -= sprite->frameWidth * 0.5f;
                world.y -= sprite->frameHeight * 0.5f;
            }
            int frame = animationFrameAt(*animation, elapsedSeconds);
            SDL_FRect dst = view.worldRectToScreen(
                world.x - static_cast<float>(sprite->originX),
                world.y - static_cast<float>(sprite->originY),
                static_cast<float>(sprite->frameWidth),
                static_cast<float>(sprite->frameHeight));

            if (cropLeft > 0.0f || cropTop > 0.0f || cropRight > 0.0f || cropBottom > 0.0f)
            {
                _menuAtlas.drawFrameSection(_sdlRenderer, sprite->texture, sprite->frameWidth,
                                            sprite->frameHeight, frame, &dst, cropLeft, cropTop, cropRight, cropBottom);
            }
            else
            {
                _menuAtlas.drawFrame(_sdlRenderer, sprite->texture, sprite->frameWidth, sprite->frameHeight,
                                     frame, &dst);
            }
        }

        SDL_FPoint stageMapPosition(fight::Stage stage) const
        {
            const StageMapEntry *entry = _stageMapCatalog.find(stage);
            return entry ? entry->mapPosition : SDL_FPoint{0.0f, 0.0f};
        }

        void drawStageMapActors(const StageMapView &view)
        {
            float elapsedSeconds = static_cast<float>(SDL_GetTicks()) / 1000.0f;

            drawMapSpriteAnimation("boat", "idle", view, elapsedSeconds);
            drawMapSpriteAnimation("twilightSpire",
                                   _state.stage == fight::Stage::TWILIGHT_SPIRE ? "selected" : "notSelected",
                                   view, elapsedSeconds);
            drawMapSpriteAnimation("towerForgeMap",
                                   _state.stage == fight::Stage::TOWERFORGE ? "unlockSelected" : "unlockNotSelected",
                                   view, elapsedSeconds, false, {0.0f, 0.0f}, 4.0f);
            drawMapSpriteAnimation("ascensionUnlock", "unlockNotSelected",
                                   view, elapsedSeconds, false, {0.0f, 0.0f}, 8.0f);
        }

        StageMapView stageMapView(const ViewportLayout &layout)
        {
            return computeStageMapView(layout.mapRect, updateStageMapCamera(SDL_GetTicks()),
                                       MAP_SELECTION_ZOOM);
        }

        void drawFillerCoverOutward(
            TextureAtlas &atlas,
            const std::string &asset,
            const SDL_FRect &rect,
            int anchorX,
            int anchorY)
        {
            const SDL_Rect *source = atlas.getRect(asset);
            if (!source || rect.w <= 0.0f || rect.h <= 0.0f || source->w <= 0 || source->h <= 0)
                return;

            float scale = std::max(
                rect.w / static_cast<float>(source->w),
                rect.h / static_cast<float>(source->h));
            SDL_FRect dst = {
                rect.x + (rect.w - source->w * scale) * 0.5f,
                rect.y + (rect.h - source->h * scale) * 0.5f,
                source->w * scale,
                source->h * scale};

            if (anchorX > 0)
                dst.x = rect.x + rect.w - dst.w;
            else if (anchorX < 0)
                dst.x = rect.x;

            if (anchorY > 0)
                dst.y = rect.y + rect.h - dst.h;
            else if (anchorY < 0)
                dst.y = rect.y;

            atlas.draw(_sdlRenderer, asset, &dst);
        }

        void drawStageDeadSpace(const ViewportLayout &layout)
        {
            const SDL_FRect &left = layout.deadSpace[0];
            if (left.w > 0.0f && left.h > 0.0f)
                drawFillerCoverOutward(_atlas, "aspectBarLeft", left, 1, 0);

            const SDL_FRect &right = layout.deadSpace[1];
            if (right.w > 0.0f && right.h > 0.0f)
                drawFillerCoverOutward(_atlas, "aspectBarRight", right, -1, 0);

            const SDL_FRect &top = layout.deadSpace[2];
            if (top.w > 0.0f && top.h > 0.0f)
                drawFillerCoverOutward(_menuAtlas, "towerTile", top, 0, 1);

            const SDL_FRect &bottom = layout.deadSpace[3];
            if (bottom.w > 0.0f && bottom.h > 0.0f)
                drawFillerCoverOutward(_menuAtlas, "towerTile", bottom, 0, -1);
        }

        void applyStageMapColorBoost()
        {
            _menuAtlas.setColorModFloat(MAP_SATURATION_BOOST, MAP_SATURATION_BOOST,
                                        MAP_SATURATION_BOOST);
        }

        void resetStageMapColorBoost()
        {
            _menuAtlas.setColorModFloat(1.0f, 1.0f, 1.0f);
        }

        void drawStageMap(const StageMapView &view)
        {
            const SDL_Rect *water = _menuAtlas.getRect("mapWater");
            if (water)
            {
                float seconds = static_cast<float>(SDL_GetTicks()) / 1000.0f;
                float rowHeight = view.mapRect.h / static_cast<float>(water->h);
                float waterWidth = std::min(
                    StageMapView::NATIVE_SIZE,
                    static_cast<float>(water->w) - MAP_WATER_SWAY_PIXELS);

                for (int row = 0; row < water->h; row++)
                {
                    float phaseSeconds = seconds + row * MAP_WATER_ROW_TIME_OFFSET_SECONDS;
                    float wave = (std::sin((phaseSeconds / MAP_WATER_SWAY_SECONDS) * PI * 2.0f) + 1.0f) * 0.5f;
                    float offset = std::round(wave * MAP_WATER_SWAY_PIXELS);
                    SDL_FRect src = {
                        static_cast<float>(water->x) + MAP_WATER_X_OFFSET_PIXELS + offset,
                        static_cast<float>(water->y + row),
                        waterWidth,
                        1.0f};
                    float rowTop = std::floor(view.mapRect.y + row * rowHeight);
                    float rowBottom = std::ceil(view.mapRect.y + (row + 1) * rowHeight);
                    SDL_FRect dst = {
                        std::floor(view.mapRect.x),
                        rowTop,
                        view.mapRect.w,
                        rowBottom - rowTop};

                    _menuAtlas.drawSource(_sdlRenderer, &src, &dst);
                }
            }
            SDL_FRect mapDst = {
                std::floor(view.mapRect.x),
                std::floor(view.mapRect.y),
                view.mapRect.w,
                view.mapRect.h};
            _menuAtlas.draw(_sdlRenderer, "mapLand", &mapDst);
        }

        const char *selectedStageMapSpriteId() const
        {
            switch (_state.stage)
            {
            case fight::Stage::THE_AMARANTH:
                return "ghostShipMap";
            case fight::Stage::DREADWOOD:
                return "dreadwoodMap";
            case fight::Stage::DARKFANG:
                return "darkfangMap";
            case fight::Stage::CATACLYSM:
                return "cataclysmMap";
            default:
                return nullptr;
            }
        }

        bool usesDarkStageSlate(fight::Stage stage) const
        {
            return stage == fight::Stage::THE_AMARANTH ||
                   stage == fight::Stage::DREADWOOD ||
                   stage == fight::Stage::DARKFANG ||
                   stage == fight::Stage::CATACLYSM;
        }

        int animationFrameAt(const SpriteAnimation &animation, float elapsedSeconds) const
        {
            if (animation.frames.empty())
                return 0;

            if (animation.delay <= 0.0f)
                return animation.frames.front();

            int frameIndex = static_cast<int>(elapsedSeconds / animation.delay);
            if (animation.loop)
                frameIndex %= static_cast<int>(animation.frames.size());
            else
                frameIndex = std::min(frameIndex, static_cast<int>(animation.frames.size()) - 1);

            return animation.frames[static_cast<std::size_t>(frameIndex)];
        }

        float animationDuration(const SpriteAnimation &animation) const
        {
            return animation.delay * static_cast<float>(animation.frames.size());
        }

        void updateSelectedMapAnimation(Uint64 now)
        {
            if (_animatedMapStage != _state.stage)
            {
                _animatedMapStage = _state.stage;
                _animatedMapStartTicks = now;
            }
        }

        void updateSunkenCityAnimation(Uint64 now)
        {
            bool targetSelected = _state.stage == fight::Stage::SUNKEN_CITY;
            if (!_sunkenCityAnimationInitialized)
            {
                _sunkenCitySelected = targetSelected;
                _sunkenCityTargetSelected = targetSelected;
                _sunkenCityAnimationStartTicks = now;
                _sunkenCityAnimationInitialized = true;
                return;
            }

            if (_sunkenCityTargetSelected != targetSelected)
            {
                _sunkenCitySelected = _sunkenCityTargetSelected;
                _sunkenCityTargetSelected = targetSelected;
                _sunkenCityAnimationStartTicks = now;
            }
        }

        void drawSunkenCityMap(const StageMapView &view)
        {
            const SpriteDef *sprite = _menuSpriteCatalog.find("sunkenCityMap");
            if (!sprite)
                return;

            Uint64 now = SDL_GetTicks();
            updateSunkenCityAnimation(now);

            float elapsedSeconds = static_cast<float>(now - _sunkenCityAnimationStartTicks) /
                                   1000.0f * SUNKEN_CITY_ANIMATION_SPEED;
            const SpriteAnimation *animation = nullptr;
            float animationElapsedSeconds = elapsedSeconds;

            if (_sunkenCityTargetSelected != _sunkenCitySelected)
            {
                animation = sprite->animation(_sunkenCityTargetSelected ? "up" : "down");
                if (animation && elapsedSeconds >= animationDuration(*animation))
                {
                    _sunkenCitySelected = _sunkenCityTargetSelected;
                    animationElapsedSeconds = 0.0f;
                    animation = nullptr;
                }
            }

            if (!animation)
                animation = sprite->animation(_sunkenCityTargetSelected ? "idleUp" : "idle");
            if (!animation)
                return;

            int frame = animationFrameAt(*animation, animationElapsedSeconds);
            SDL_FPoint center = stageMapPosition(fight::Stage::SUNKEN_CITY);
            SDL_FRect dst = view.worldRectToScreen(
                center.x - sprite->frameWidth * 0.5f - static_cast<float>(sprite->originX),
                center.y - sprite->frameHeight * 0.5f - static_cast<float>(sprite->originY),
                static_cast<float>(sprite->frameWidth),
                static_cast<float>(sprite->frameHeight));

            _menuAtlas.drawFrame(_sdlRenderer, sprite->texture, sprite->frameWidth, sprite->frameHeight,
                                 frame, &dst);
        }

        void drawSelectedStageMapAnimation(const StageMapView &view)
        {
            const char *spriteId = selectedStageMapSpriteId();
            if (!spriteId)
                return;

            const SpriteDef *sprite = _menuSpriteCatalog.find(spriteId);
            if (!sprite)
                return;

            Uint64 now = SDL_GetTicks();
            updateSelectedMapAnimation(now);

            float elapsedSeconds = static_cast<float>(now - _animatedMapStartTicks) / 1000.0f;
            const SpriteAnimation *up = sprite->animation("up");
            const SpriteAnimation *idleUp = sprite->animation("idleUp");
            const SpriteAnimation *animation = idleUp ? idleUp : sprite->animation("idle");
            float animationElapsedSeconds = elapsedSeconds;

            if (up && elapsedSeconds < animationDuration(*up))
            {
                animation = up;
            }
            else if (up)
            {
                animationElapsedSeconds = elapsedSeconds - animationDuration(*up);
            }

            if (!animation)
                return;

            int frame = animationFrameAt(*animation, animationElapsedSeconds);
            SDL_FPoint world = {
                static_cast<float>(sprite->x),
                static_cast<float>(sprite->y)};

            SDL_FRect dst = view.worldRectToScreen(
                world.x - static_cast<float>(sprite->originX),
                world.y - static_cast<float>(sprite->originY),
                static_cast<float>(sprite->frameWidth),
                static_cast<float>(sprite->frameHeight));
            _menuAtlas.drawFrame(_sdlRenderer, sprite->texture, sprite->frameWidth, sprite->frameHeight,
                                 frame, &dst);
        }

        void drawSelectedStagePointer(const StageMapView &view)
        {
            const StageMapEntry *entry = _stageMapCatalog.find(_state.stage);
            if (!entry)
                return;

            const SDL_Rect *cursor = _menuAtlas.getRect("mapCursor");
            if (!cursor)
                return;

            Uint64 now = SDL_GetTicks();
            SDL_FPoint cursorPosition = updateStageMapCursor(now);
            SDL_FPoint target = view.worldToScreen(cursorPosition);
            float bobOffset = mapCursorBobOffset(now) * view.scale;
            SDL_FRect dst = {
                target.x - cursor->w * view.scale * 0.5f,
                target.y - (cursor->h + MAP_CURSOR_TARGET_GAP_PIXELS) * view.scale + bobOffset,
                cursor->w * view.scale,
                cursor->h * view.scale};
            _menuAtlas.draw(_sdlRenderer, "mapCursor", &dst);
        }

        void drawStageIcons(const StageMapView &view)
        {
            drawStageMapActors(view);
            drawSunkenCityMap(view);
            drawSelectedStageMapAnimation(view);
            drawSelectedStagePointer(view);
        }

        int selectedStageIndex() const
        {
            const auto &entries = _stageMapCatalog.entries();
            auto it = std::find_if(entries.begin(), entries.end(),
                                   [&](const StageMapEntry &entry)
                                   {
                                       return entry.stage == _state.stage;
                                   });

            return it == entries.end() ? 0 : static_cast<int>(std::distance(entries.begin(), it));
        }

        void updateStageCarouselAnimation()
        {
            int targetIndex = selectedStageIndex();
            Uint64 now = SDL_GetTicks();
            if (!_stageCarouselInitialized)
            {
                _stageCarouselPosition = static_cast<float>(targetIndex);
                _lastCarouselTicks = now;
                _stageCarouselInitialized = true;
                return;
            }

            float deltaSeconds = static_cast<float>(now - _lastCarouselTicks) / 1000.0f;
            _lastCarouselTicks = now;
            float target = static_cast<float>(targetIndex);
            float blend = std::clamp(deltaSeconds * 14.0f, 0.0f, 1.0f);
            _stageCarouselPosition += (target - _stageCarouselPosition) * blend;
            if (std::fabs(target - _stageCarouselPosition) < 0.01f)
                _stageCarouselPosition = target;
        }

        void drawStageIconCarousel(const SDL_FRect &viewport)
        {
            const auto &entries = _stageMapCatalog.entries();
            if (entries.empty())
                return;

            updateStageCarouselAnimation();

            constexpr float nativeTile = 30.0f;
            float scale = std::clamp(viewport.h / 360.0f, 1.0f, 2.0f);
            float tileSize = nativeTile * scale;
            float spacing = tileSize + 8.0f * scale;
            float centerX = viewport.x + viewport.w * 0.5f;
            float rowCenterY = viewport.y + viewport.h - tileSize * 0.65f;
            float selectedIndex = static_cast<float>(selectedStageIndex());
            int sideVisibleCount = static_cast<int>(std::ceil((viewport.w * 0.5f) / spacing)) + 2;
            int firstVisible = std::max(0,
                                        static_cast<int>(std::floor(_stageCarouselPosition)) - sideVisibleCount);
            int lastVisible = std::min(static_cast<int>(entries.size()) - 1,
                                       static_cast<int>(std::ceil(_stageCarouselPosition)) + sideVisibleCount);

            for (int i = firstVisible; i <= lastVisible; i++)
            {
                const StageMapEntry &entry = entries[static_cast<std::size_t>(i)];
                float offset = (static_cast<float>(i) - _stageCarouselPosition) * spacing;
                float distanceFromSelected = std::fabs(static_cast<float>(i) - selectedIndex);
                float focusLift = std::max(0.0f, 1.0f - distanceFromSelected) * 4.0f * scale;
                float tileScale = scale * (distanceFromSelected < 0.01f ? 1.18f : 1.0f);
                float currentTileSize = nativeTile * tileScale;
                float currentIconSize = 16.0f * tileScale;
                float tileX = centerX + offset - currentTileSize * 0.5f;
                float tileY = rowCenterY - currentTileSize * 0.5f - focusLift;

                if (tileX > viewport.x + viewport.w || tileX + currentTileSize < viewport.x)
                    continue;

                SDL_FRect tileDst = {tileX, tileY, currentTileSize, currentTileSize};
                _menuAtlas.draw(_sdlRenderer,
                                usesDarkStageSlate(entry.stage) ? "darkLevelBlock" : "levelBlock",
                                &tileDst);

                SDL_FRect iconDst = {
                    tileX + (currentTileSize - currentIconSize) * 0.5f,
                    tileY + (currentTileSize - currentIconSize) * 0.5f,
                    currentIconSize,
                    currentIconSize};
                _menuAtlas.draw(_sdlRenderer, entry.iconAtlasName, &iconDst);
            }
        }

        const ArcherSkin *previewSkin(unsigned int character) const
        {
            if (_archerCatalog.validBaseCount() == 0 || character >= _archerCatalog.validBaseCount())
                return nullptr;

            return &_archerCatalog.skinForCharacter(character);
        }

        std::size_t availableCharacterCount() const
        {
            return _archerCatalog.validBaseCount() + _customCharacters.characters().size();
        }

        const charactereditor::CustomCharacter *customCharacter(unsigned int character) const
        {
            const std::size_t builtInCount = _archerCatalog.validBaseCount();
            if (character < builtInCount)
                return nullptr;

            const std::size_t customIndex = static_cast<std::size_t>(character) - builtInCount;
            const auto &characters = _customCharacters.characters();
            return customIndex < characters.size() ? &characters[customIndex] : nullptr;
        }

        const charactereditor::Canvas *customCanvas(
            const charactereditor::CustomCharacter &character, const char *id) const
        {
            return charactereditor::CharacterStore::canvas(character, id);
        }

        std::string customAtlasName(
            const charactereditor::CustomCharacter &character,
            const charactereditor::Canvas &canvas) const
        {
            return "custom/" + character.id + "/" + canvas.id;
        }

        std::string characterName(unsigned int character) const
        {
            if (const auto *custom = customCharacter(character))
                return custom->name;
            if (const auto *skin = previewSkin(character))
                return skin->name;
            return "missing";
        }

        void refreshCustomAssets()
        {
            const std::filesystem::path root = charactereditor::CharacterStore::rootDirectory();
            const std::filesystem::path metadataPath = root / "characters.xml";
            const std::filesystem::path imagePath = root / "customCharacterAtlas.bmp";
            const std::filesystem::path xmlPath = root / "customCharacterAtlas.xml";
            const auto writeTime = [](const std::filesystem::path &path)
            {
                std::error_code error;
                if (!std::filesystem::exists(path, error) || error)
                    return std::filesystem::file_time_type{};
                const auto result = std::filesystem::last_write_time(path, error);
                return error ? std::filesystem::file_time_type{} : result;
            };

            const auto metadataWriteTime = writeTime(metadataPath);
            const auto imageWriteTime = writeTime(imagePath);
            const auto xmlWriteTime = writeTime(xmlPath);
            const bool assetsChanged = !_customAssetsInitialized ||
                metadataWriteTime != _customMetadataWriteTime ||
                imageWriteTime != _customAtlasImageWriteTime ||
                xmlWriteTime != _customAtlasXmlWriteTime;
            if (!assetsChanged)
                return;

            _customCharacters.reload();
            _customAssetsInitialized = true;
            _customMetadataWriteTime = metadataWriteTime;

            std::error_code error;
            if (!std::filesystem::exists(imagePath, error) || error ||
                !std::filesystem::exists(xmlPath, error) || error)
            {
                if (_customAtlasLoaded)
                    _customAtlas.unload();
                _customAtlasLoaded = false;
                _customAtlasImageWriteTime = imageWriteTime;
                _customAtlasXmlWriteTime = xmlWriteTime;
                return;
            }

            _customAtlas.unload();
            _customAtlasLoaded = false;
            try
            {
                _customAtlasLoaded = _customAtlas.load(_sdlRenderer, imagePath.string(), xmlPath.string());
                if (_customAtlasLoaded)
                {
                    _customAtlasImageWriteTime = imageWriteTime;
                    _customAtlasXmlWriteTime = xmlWriteTime;
                }
            }
            catch (const std::exception &)
            {
                _customAtlas.unload();
            }
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
            if (const auto *custom = customCharacter(player.character))
            {
                if (!_customAtlasLoaded)
                    return;

                const charactereditor::AnimationSpec* animation =
                    charactereditor::animationSpec("run");
                if (!animation)
                    return;
                const int frame = animation->firstFrame
                    + static_cast<int>((SDL_GetTicks() / animation->delayMs)
                        % static_cast<Uint64>(animation->frameCount));
                SDL_FRect dst = {
                    anchorX - charactereditor::FRAME_WIDTH * scale * 0.5f,
                    anchorY - charactereditor::FRAME_HEIGHT * scale * 0.5f,
                    charactereditor::FRAME_WIDTH * scale,
                    charactereditor::FRAME_HEIGHT * scale};
                static constexpr std::array<const char*, 3> drawOrder = {"body", "head", "bow"};
                for (const char* id : drawOrder)
                {
                    const auto* canvas = customCanvas(*custom, id);
                    if (!canvas || frame >= canvas->frameCount)
                        continue;
                    _customAtlas.drawFrame(_sdlRenderer, customAtlasName(*custom, *canvas),
                        charactereditor::FRAME_WIDTH, charactereditor::FRAME_HEIGHT,
                        frame, &dst, false);
                }
                return;
            }

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
        TextureAtlas _customAtlas;
        SpriteCatalog _spriteCatalog;
        SpriteCatalog _menuSpriteCatalog;
        ArcherCatalog _archerCatalog;
        charactereditor::CharacterStore _customCharacters;
        StageMapCatalog _stageMapCatalog;
        State _state;
        float _stageCarouselPosition = 0.0f;
        Uint64 _lastCarouselTicks = 0;
        Uint64 _animatedMapStartTicks = 0;
        Uint64 _sunkenCityAnimationStartTicks = 0;
        Uint64 _mapCameraStartTicks = 0;
        Uint64 _mapCursorStartTicks = 0;
        SDL_FPoint _mapCameraStart{StageMapView::NATIVE_SIZE * 0.5f, StageMapView::NATIVE_SIZE * 0.5f};
        SDL_FPoint _mapCameraTarget{StageMapView::NATIVE_SIZE * 0.5f, StageMapView::NATIVE_SIZE * 0.5f};
        SDL_FPoint _mapCursorStart{StageMapView::NATIVE_SIZE * 0.5f, StageMapView::NATIVE_SIZE * 0.5f};
        SDL_FPoint _mapCursorTarget{StageMapView::NATIVE_SIZE * 0.5f, StageMapView::NATIVE_SIZE * 0.5f};
        fight::Stage _animatedMapStage = fight::Stage::MAX_ENUM;
        fight::Stage _mapCameraStage = fight::Stage::MAX_ENUM;
        fight::Stage _mapCursorStage = fight::Stage::MAX_ENUM;
        bool _stageCarouselInitialized = false;
        bool _sunkenCityAnimationInitialized = false;
        bool _sunkenCitySelected = false;
        bool _sunkenCityTargetSelected = false;
        bool _mapCameraInitialized = false;
        bool _mapCursorInitialized = false;
        bool _customAtlasLoaded = false;
        bool _customAssetsInitialized = false;
        std::filesystem::file_time_type _customMetadataWriteTime{};
        std::filesystem::file_time_type _customAtlasImageWriteTime{};
        std::filesystem::file_time_type _customAtlasXmlWriteTime{};
    };

}; // end namespace renderer
