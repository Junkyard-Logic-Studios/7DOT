#include "fightRenderer.hpp"
#include "pugixml.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>



namespace
{
    constexpr float RENDER_SCALE = 3.0f;

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
        partWorldX(sprite, worldPosition, fliphoriz, localXOffset) * RENDER_SCALE,
        dstY * RENDER_SCALE,
        sprite.frameWidth * RENDER_SCALE,
        sprite.frameHeight * RENDER_SCALE
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
    glm::vec2 hitboxTL = archer.hitboxTL();
    glm::vec2 hitboxBR = archer.hitboxBR();
    SDL_FRect hitbox = {
        hitboxTL.x * RENDER_SCALE,
        hitboxTL.y * RENDER_SCALE,
        (hitboxBR.x - hitboxTL.x) * RENDER_SCALE,
        (hitboxBR.y - hitboxTL.y) * RENDER_SCALE
    };

    SDL_SetRenderDrawColor(_sdlRenderer, 255, 0, 0, 255);
    SDL_RenderRect(_sdlRenderer, &hitbox);

    SDL_FRect origin = {
        archer.position.x * RENDER_SCALE - 2.0f,
        archer.position.y * RENDER_SCALE - 2.0f,
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


void renderer::FightRenderer::render()
{
    Background& background = getBackground(_scene.getStage());

    // reset drawing
    SDL_SetRenderDrawColor(_sdlRenderer, background.color.r, 
        background.color.g, background.color.b, background.color.a);
    SDL_RenderClear(_sdlRenderer);
    SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 255, 255);

    int winw, winh;
    SDL_GetWindowSize(_sdlWindow, &winw, &winh);
    SDL_FRect screenRect = { 0.0f, 0.0f, (float)winw, (float)winh };

    auto& level = _scene.getLevel(_state.levelIndex);

    // immediate background elements
    for (auto& elem : background.elements)
    {
        SDL_FRect rect = screenRect;
        rect.x = elem.x;
        rect.y = elem.y;
        if (elem.name == "Backdrop" || elem.name == "WavyLayer")
            _bgAtlas.draw(_sdlRenderer, elem.content, &rect);
    }

    // tileset name
    std::string tsname(fight::stageToName(_scene.getStage()));
    tsname[0] = std::tolower(tsname[0]);
    tsname.erase(std::remove_if(tsname.begin(), tsname.end(), isspace), tsname.end());
    
    // background tilemap
    for (int y = 0; y < level.getHeight(); y++)
        for (int x = 0; x < level.getWidth(); x++)
            _atlas.drawTile(_sdlRenderer, "tilesets/" + tsname + "BG",
                level.getBackgroundAt(x, y), x, y);

    // foreground tilemap
    for (int y = 0; y < level.getHeight(); y++)
        for (int x = 0; x < level.getWidth(); x++)
            _atlas.drawTile(_sdlRenderer, "tilesets/" + tsname,
                level.getSolidAt(x, y), x, y);

    // archers
    const auto& players = _scene.getPlayers();
    for (std::size_t i = 0; i < _state.archers.size() && i < players.size(); i++)
    {
        drawArcher(_state.archers[i], players[i]);
        drawArcherDebug(_state.archers[i]);
    }

    drawDevBuildText(winw);

    SDL_RenderPresent(_sdlRenderer);
}
