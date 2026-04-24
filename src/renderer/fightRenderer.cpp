#include "fightRenderer.hpp"
#include "pugixml.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>



namespace
{
    // Renderer-side nudges while the asset metadata/parser settles.
    constexpr float HEAD_X_OFFSET = 0.0f;
    constexpr float HEAD_Y_OFFSET = 0.0f;
    constexpr float BOW_X_OFFSET = 0.0f;
    constexpr float BOW_Y_OFFSET = 0.0f;

    float partWorldX(const renderer::SpriteDef& sprite, const glm::vec2& worldPosition,
        bool fliphoriz, float localXOffset)
    {
        float localX = sprite.x + localXOffset;
        if (fliphoriz)
            return worldPosition.x - localX - (sprite.frameWidth - sprite.originX);

        return worldPosition.x + localX - sprite.originX;
    }
}



struct Background
{
    struct Element
    {
        std::string name;
        float x;
        float y;
        std::string content;
    };

    SDL_Color color;
    std::vector<Element> elements;
};


inline Background& getBackground(fight::Stage stage)
{
    static std::unordered_map<fight::Stage, Background> backgrounds;

    // parse background data XML
    if (backgrounds.empty())
    {
        // load document
        pugi::xml_document doc;
        const char* path = ASSET_DIR "Atlas/GameData/bgData.xml";
        pugi::xml_parse_result result = doc.load_file(path);
        if (!result)
            throw std::runtime_error("Failed to parse file: " + std::string(path));

        // iterate over backgrounds
        for (auto& BG : doc.child("backgrounds").children("BG"))
        {
            auto id = BG.attribute("id").as_string();
            fight::Stage st = fight::stageFromName(id);

            int col = std::stoi(BG.child("Background").attribute("bgColor").as_string(), 0, 16);

            Background background;
            background.color.r = col / 256 / 256;
            background.color.g = col / 256 % 256;
            background.color.b = col % 256;
            background.color.a = 255;

            for (auto& child : BG.child("Background").children())
                background.elements.emplace_back(
                    std::string(child.name()),
                    child.attribute("x").as_float(),
                    child.attribute("y").as_float(),
                    std::string(child.text().as_string()));

            backgrounds[st] = background;
        }
    }

    return backgrounds[stage];
}


renderer::FightRenderer::FightRenderer(
    SDL_Window* const window, 
    SDL_Renderer* const renderer,
    const fight::Scene& scene
) :
    _Renderer(window, renderer),
    _scene(scene)
{
    _atlas.load(_sdlRenderer, ASSET_DIR "Atlas/atlas.bmp", ASSET_DIR "Atlas/atlas.xml");
    _bgAtlas.load(_sdlRenderer, ASSET_DIR "Atlas/bgAtlas.bmp", ASSET_DIR "Atlas/bgAtlas.xml");
    _menuAtlas.load(_sdlRenderer, ASSET_DIR "Atlas/menuAtlas.bmp", ASSET_DIR "Atlas/menuAtlas.xml");
    _spriteCatalog.load(ASSET_DIR "Atlas/SpriteData/spriteData.xml");
    _archerCatalog.load(
        ASSET_DIR "Atlas/GameData/archerData.xml",
        _spriteCatalog,
        [&](const std::string& texture) { return _atlas.getRect(texture) != nullptr; });

    // trigger loading backgrounds
    getBackground(fight::Stage::SACRED_GROUND);
}

renderer::FightRenderer::~FightRenderer()
{
    destroyTexture(_tilemapTexture);
    destroyTexture(_worldTexture);
    _menuAtlas.unload();
    _atlas.unload();
    _bgAtlas.unload();
}

void renderer::FightRenderer::pushState(State state)
    { this->_state = state; }


std::string renderer::FightRenderer::chooseBodyAnimation(const fight::Archer& archer) const
{
    constexpr float epsilon = 0.01f;

    if (archer.isCrouching)
        return "duck";

    switch (archer.movementState)
    {
        case fight::ArcherMovementState::GROUNDED:
            if (std::abs(archer.velocity.x) > epsilon)
                return "run";
            return "stand";
        case fight::ArcherMovementState::WALL_SLIDING:
            return "glide";
        case fight::ArcherMovementState::LEDGE_CLINGING:
            return "ledge";
        case fight::ArcherMovementState::DODGING:
            return "dodge";
        case fight::ArcherMovementState::DEAD:
            return "fall";
        case fight::ArcherMovementState::AIRBORNE:
            break;
    }

    if (archer.velocity.y < -epsilon)
        return "jump";
    if (archer.velocity.y > epsilon)
        return "fall";

    return "stand";
}


std::string renderer::FightRenderer::chooseHeadAnimation(const std::string& bodyAnimation) const
{
    if (bodyAnimation == "duck")
        return "duck";
    if (bodyAnimation == "jump")
        return "idleJump";
    if (bodyAnimation == "fall" || bodyAnimation == "glide")
        return "idleFall";

    return "idle";
}


int renderer::FightRenderer::animationFrame(const SpriteDef& sprite, const std::string& animationId) const
{
    const SpriteAnimation* animation = sprite.animation(animationId);
    if (!animation || animation->frames.empty())
        return 0;

    if (animation->frames.size() == 1 || animation->delay <= 0.0f)
        return animation->frames.front();

    auto frameDurationMs = std::max<Uint64>(1, static_cast<Uint64>(std::round(animation->delay * 1000.0f)));
    std::size_t index = SDL_GetTicks() / frameDurationMs;
    if (animation->loop)
        index %= animation->frames.size();
    else
        index = std::min(index, animation->frames.size() - 1);

    return animation->frames[index];
}


int renderer::FightRenderer::frameOrigin(const std::vector<int>& origins, int frame, int fallback) const
{
    if (frame >= 0 && static_cast<std::size_t>(frame) < origins.size())
        return origins[frame];

    return fallback;
}


const std::string& renderer::FightRenderer::textureFor(const SpriteDef& sprite, const Player& player) const
{
    if (_scene.getMode() == fight::Mode::TEAM_2)
    {
        if (player.team == 0 && !sprite.blueTexture.empty() && _atlas.getRect(sprite.blueTexture))
            return sprite.blueTexture;
        if (player.team == 1 && !sprite.redTexture.empty() && _atlas.getRect(sprite.redTexture))
            return sprite.redTexture;
    }

    return sprite.texture;
}


int renderer::FightRenderer::drawSpritePart(
    const SpriteDef& sprite,
    const std::string& animationId,
    const glm::vec2& worldPosition,
    const Player& player,
    bool fliphoriz,
    float worldY,
    float localXOffset,
    float localYOffset
) {
    int frame = animationFrame(sprite, animationId);
    const std::string& texture = textureFor(sprite, player);
    if (texture.empty())
        return frame;

    float dstY = std::isnan(worldY)
        ? worldPosition.y + sprite.y + localYOffset - sprite.originY
        : worldY;
    SDL_FRect dst = {
        partWorldX(sprite, worldPosition, fliphoriz, localXOffset),
        dstY,
        static_cast<float>(sprite.frameWidth),
        static_cast<float>(sprite.frameHeight)
    };

    _atlas.drawFrame(_sdlRenderer, texture, sprite.frameWidth, sprite.frameHeight, frame, &dst, fliphoriz);
    return frame;
}


void renderer::FightRenderer::drawArcher(const fight::Archer& archer, const Player& player)
{
    if (_archerCatalog.validBaseCount() == 0)
        return;

    const ArcherSkin& skin = _archerCatalog.skinForCharacter(player.character);
    const SpriteDef* body = _spriteCatalog.find(skin.bodySprite);
    const SpriteDef* head = _spriteCatalog.find(skin.headNormalSprite);
    const SpriteDef* bow = _spriteCatalog.find(skin.bowSprite);
    if (!body || !head || !bow)
        return;

    bool fliphoriz = !archer.isFacingRight;
    std::string bodyAnimation = chooseBodyAnimation(archer);
    int bodyFrame = drawSpritePart(*body, bodyAnimation, archer.position, player, fliphoriz, NAN);

    int headXOrigin = frameOrigin(body->headXOrigins, bodyFrame, body->originX);
    int headYOrigin = frameOrigin(body->headYOrigins, bodyFrame, body->originY);

    std::string headAnimation = chooseHeadAnimation(bodyAnimation);

    auto drawHead = [&](const std::string& spriteId)
    {
        if (spriteId.empty())
            return;

        const SpriteDef* headPart = _spriteCatalog.find(spriteId);
        if (!headPart)
            return;

        float headLocalX = headXOrigin - body->originX + HEAD_X_OFFSET;
        float headY = archer.position.y + headPart->y - headYOrigin - headPart->originY + HEAD_Y_OFFSET;
        drawSpritePart(*headPart, headAnimation, archer.position, player, fliphoriz, headY, headLocalX);
    };

    drawHead(skin.headBackSprite);
    drawHead(skin.headNormalSprite);

    if (!bow->hideBowIdle)
        drawSpritePart(*bow, "idle", archer.position, player, fliphoriz, NAN, BOW_X_OFFSET, BOW_Y_OFFSET);
}


void renderer::FightRenderer::drawArcherDebug(const fight::Archer& archer)
{
    auto drawHitbox = [&](const fight::Archer::Hitbox& hitbox, SDL_Color color)
    {
        SDL_FRect rect = {
            hitbox.tl.x,
            hitbox.tl.y,
            hitbox.br.x - hitbox.tl.x,
            hitbox.br.y - hitbox.tl.y
        };

        SDL_SetRenderDrawColor(_sdlRenderer, color.r, color.g, color.b, color.a);
        SDL_RenderRect(_sdlRenderer, &rect);
    };

    fight::Archer::Hitbox full = archer.fullHitbox();
    fight::Archer::Hitbox head = archer.headHitbox();
    fight::Archer::Hitbox body = archer.bodyHitbox();

    drawHitbox(full, SDL_Color {255, 0, 0, 255});
    drawHitbox(head, SDL_Color {0, 220, 255, 255});
    drawHitbox(body, SDL_Color {255, 220, 0, 255});

    SDL_SetRenderDrawColor(_sdlRenderer, 255, 0, 255, 255);
    SDL_RenderLine(_sdlRenderer, full.tl.x, head.br.y, full.br.x, head.br.y);

    SDL_FRect origin = {
        archer.position.x - 2.0f,
        archer.position.y - 2.0f,
        4.0f,
        4.0f
    };
    SDL_SetRenderDrawColor(_sdlRenderer, 0, 255, 0, 255);
    SDL_RenderFillRect(_sdlRenderer, &origin);

    SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 255, 255);
}


void renderer::FightRenderer::drawDevBuildText(int winw)
{
    constexpr float padding = 8.0f;
    const char* text = "DEV BUILD";
    float textWidth = SDL_strlen(text) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
    float textHeight = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
    float boxWidth = textWidth + padding * 2.0f;
    float boxHeight = textHeight + padding;
    SDL_FRect background = {
        winw - boxWidth - padding,
        padding,
        boxWidth,
        boxHeight
    };

    SDL_SetRenderDrawColor(_sdlRenderer, 0, 0, 0, 255);
    SDL_RenderFillRect(_sdlRenderer, &background);
    SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 0, 255);
    SDL_RenderDebugText(_sdlRenderer, background.x + padding, background.y + padding * 0.5f, text);
    SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 255, 255);
}


std::string renderer::FightRenderer::tilesetName() const
{
    std::string tsname(fight::stageToName(_scene.getStage()));
    tsname[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(tsname[0])));
    tsname.erase(std::remove_if(tsname.begin(), tsname.end(),
        [](unsigned char c) { return std::isspace(c); }), tsname.end());
    return tsname;
}


void renderer::FightRenderer::destroyTexture(SDL_Texture*& texture)
{
    if (texture)
    {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}


void renderer::FightRenderer::ensureWorldTexture(int width, int height)
{
    if (_worldTexture && _worldWidth == width && _worldHeight == height)
        return;

    destroyTexture(_worldTexture);
    _worldTexture = SDL_CreateTexture(
        _sdlRenderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width,
        height);
    if (!_worldTexture)
        throw std::runtime_error("Failed to create fight world texture: " + std::string(SDL_GetError()));

    SDL_SetTextureScaleMode(_worldTexture, SDL_ScaleMode::SDL_SCALEMODE_PIXELART);
    _worldWidth = width;
    _worldHeight = height;
}


void renderer::FightRenderer::ensureTilemapCache(
    const fight::Level& level,
    const FightTilemapCacheKey& key)
{
    if (!_tilemapCacheState.shouldRebuild(key) && _tilemapTexture)
        return;

    rebuildTilemapCache(level, key);
}


void renderer::FightRenderer::rebuildTilemapCache(
    const fight::Level& level,
    const FightTilemapCacheKey& key)
{
    int width = static_cast<int>(level.getWidth() * TILESIZE);
    int height = static_cast<int>(level.getHeight() * TILESIZE);

    destroyTexture(_tilemapTexture);
    _tilemapTexture = SDL_CreateTexture(
        _sdlRenderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width,
        height);
    if (!_tilemapTexture)
        throw std::runtime_error("Failed to create fight tilemap texture: " + std::string(SDL_GetError()));

    SDL_SetTextureScaleMode(_tilemapTexture, SDL_ScaleMode::SDL_SCALEMODE_PIXELART);
    SDL_SetTextureBlendMode(_tilemapTexture, SDL_BLENDMODE_BLEND);

    if (!SDL_SetRenderTarget(_sdlRenderer, _tilemapTexture))
        throw std::runtime_error("Failed to set fight tilemap render target: " + std::string(SDL_GetError()));

    SDL_SetRenderDrawColor(_sdlRenderer, 0, 0, 0, 0);
    SDL_RenderClear(_sdlRenderer);
    SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 255, 255);

    const std::string bgTileset = "tilesets/" + key.tilesetName + "BG";
    const std::string fgTileset = "tilesets/" + key.tilesetName;
    for (std::size_t y = 0; y < level.getHeight(); y++)
    {
        for (std::size_t x = 0; x < level.getWidth(); x++)
        {
            SDL_FRect dst = {
                static_cast<float>(x * TILESIZE),
                static_cast<float>(y * TILESIZE),
                static_cast<float>(TILESIZE),
                static_cast<float>(TILESIZE)
            };
            _atlas.drawTile(_sdlRenderer, bgTileset, level.getBackgroundAt(x, y), dst);
            _atlas.drawTile(_sdlRenderer, fgTileset, level.getSolidAt(x, y), dst);
        }
    }

    if (!SDL_SetRenderTarget(_sdlRenderer, nullptr))
        throw std::runtime_error("Failed to reset fight tilemap render target: " + std::string(SDL_GetError()));
    _tilemapCacheState.markValid(key);
}


void renderer::FightRenderer::drawDeadSpace(const ViewportLayout& layout)
{
    const SDL_FRect& left = layout.deadSpace[0];
    if (left.w > 0.0f && left.h > 0.0f)
        drawFillerCoverOutward(_atlas, "aspectBarLeft", left, 1, 0);

    const SDL_FRect& right = layout.deadSpace[1];
    if (right.w > 0.0f && right.h > 0.0f)
        drawFillerCoverOutward(_atlas, "aspectBarRight", right, -1, 0);

    const SDL_FRect& top = layout.deadSpace[2];
    if (top.w > 0.0f && top.h > 0.0f)
        drawFillerCoverOutward(_menuAtlas, "towerTile", top, 0, 1);

    const SDL_FRect& bottom = layout.deadSpace[3];
    if (bottom.w > 0.0f && bottom.h > 0.0f)
        drawFillerCoverOutward(_menuAtlas, "towerTile", bottom, 0, -1);
}


void renderer::FightRenderer::drawFillerCoverOutward(
    TextureAtlas& atlas,
    const std::string& asset,
    const SDL_FRect& rect,
    int anchorX,
    int anchorY)
{
    const SDL_Rect* source = atlas.getRect(asset);
    if (!source || rect.w <= 0.0f || rect.h <= 0.0f || source->w <= 0 || source->h <= 0)
        return;

    float scale = std::max(
        rect.w / static_cast<float>(source->w),
        rect.h / static_cast<float>(source->h));
    SDL_FRect dst = {
        rect.x + (rect.w - source->w * scale) * 0.5f,
        rect.y + (rect.h - source->h * scale) * 0.5f,
        source->w * scale,
        source->h * scale
    };

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


void renderer::FightRenderer::render()
{
    Background& background = getBackground(_scene.getStage());

    int winw, winh;
    SDL_GetWindowSize(_sdlWindow, &winw, &winh);
    auto& level = _scene.getLevel(_state.levelIndex);
    int worldWidth = static_cast<int>(level.getWidth() * TILESIZE);
    int worldHeight = static_cast<int>(level.getHeight() * TILESIZE);
    ViewportLayout layout = computeViewportLayout(winw, winh, worldWidth, worldHeight);

    ensureWorldTexture(worldWidth, worldHeight);
    FightTilemapCacheKey tilemapKey {
        _scene.getStage(),
        _state.levelIndex,
        level.getWidth(),
        level.getHeight(),
        tilesetName()
    };
    ensureTilemapCache(level, tilemapKey);

    if (!SDL_SetRenderTarget(_sdlRenderer, _worldTexture))
        throw std::runtime_error("Failed to set fight world render target: " + std::string(SDL_GetError()));

    // reset native world drawing
    SDL_SetRenderDrawColor(_sdlRenderer, background.color.r,
        background.color.g, background.color.b, background.color.a);
    SDL_RenderClear(_sdlRenderer);
    SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 255, 255);

    SDL_FRect worldRect = { 0.0f, 0.0f, static_cast<float>(worldWidth), static_cast<float>(worldHeight) };

    // immediate background elements
    for (auto& elem : background.elements)
    {
        SDL_FRect rect = worldRect;
        rect.x = elem.x;
        rect.y = elem.y;
        if (elem.name == "Backdrop" || elem.name == "WavyLayer")
            _bgAtlas.draw(_sdlRenderer, elem.content, &rect);
    }

    SDL_RenderTexture(_sdlRenderer, _tilemapTexture, nullptr, &worldRect);

    // archers
    const auto& players = _scene.getPlayers();
    for (std::size_t i = 0; i < _state.archers.size() && i < players.size(); i++)
    {
        drawArcher(_state.archers[i], players[i]);
        drawArcherDebug(_state.archers[i]);
    }

    if (!SDL_SetRenderTarget(_sdlRenderer, nullptr))
        throw std::runtime_error("Failed to reset fight render target: " + std::string(SDL_GetError()));

    SDL_SetRenderDrawColor(_sdlRenderer, 19, 8, 31, 255);
    SDL_RenderClear(_sdlRenderer);
    drawDeadSpace(layout);
    SDL_RenderTexture(_sdlRenderer, _worldTexture, nullptr, &layout.mapRect);

    drawDevBuildText(winw);

    SDL_RenderPresent(_sdlRenderer);
}
