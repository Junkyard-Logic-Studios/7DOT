#include "CharacterEditorRenderer.hpp"
#include "../constants.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace
{
    constexpr float TEXT_SIZE = static_cast<float>(SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE);

    bool contains(const SDL_FRect& rect, float x, float y)
    {
        return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
    }

    void setColor(SDL_Renderer* renderer, std::uint32_t packed)
    {
        SDL_SetRenderDrawColor(renderer,
            static_cast<Uint8>((packed >> 24) & 0xff),
            static_cast<Uint8>((packed >> 16) & 0xff),
            static_cast<Uint8>((packed >> 8) & 0xff),
            static_cast<Uint8>(packed & 0xff));
    }

    std::uint32_t hsvColor(float hue, float saturation, float value, float alpha = 1.0f)
    {
        hue = hue - std::floor(hue);
        const float scaled = hue * 6.0f;
        const int sector = static_cast<int>(std::floor(scaled)) % 6;
        const float fraction = scaled - std::floor(scaled);
        const float p = value * (1.0f - saturation);
        const float q = value * (1.0f - saturation * fraction);
        const float t = value * (1.0f - saturation * (1.0f - fraction));
        float r = value, g = t, b = p;
        switch (sector)
        {
            case 0: r = value; g = t; b = p; break;
            case 1: r = q; g = value; b = p; break;
            case 2: r = p; g = value; b = t; break;
            case 3: r = p; g = q; b = value; break;
            case 4: r = t; g = p; b = value; break;
            default: r = value; g = p; b = q; break;
        }
        const auto byte = [](float component) {
            return static_cast<std::uint32_t>(std::round(std::clamp(component, 0.0f, 1.0f) * 255.0f));
        };
        return (byte(r) << 24) | (byte(g) << 16) | (byte(b) << 8) | byte(alpha);
    }

    std::size_t animationFrame(int frameCount, Uint64 interval)
    {
        if (frameCount <= 0)
            return 0;
        return static_cast<std::size_t>((SDL_GetTicks() / interval) % static_cast<Uint64>(frameCount));
    }
}

renderer::CharacterEditorRenderer::CharacterEditorRenderer(
    SDL_Window* window, SDL_Renderer* renderer) :
    _Renderer(window, renderer)
{
    _atlas.load(_sdlRenderer, ASSET_DIR "Atlas/atlas.bmp", ASSET_DIR "Atlas/atlas.xml");
    _sprites.load(ASSET_DIR "Atlas/SpriteData/spriteData.xml");
    _archers.load(
        ASSET_DIR "Atlas/GameData/archerData.xml",
        _sprites,
        [&](const std::string& texture) { return _atlas.getRect(texture) != nullptr; });

    for (std::size_t i = 0; i < _archers.baseSkins().size(); ++i)
    {
        if (!_archers.isBaseSkinRenderable(i))
            continue;
        const ArcherSkin& skin = _archers.baseSkins()[i];
        const std::string& previewHead = skin.startNoHat && !skin.headNoHatSprite.empty()
            ? skin.headNoHatSprite : skin.headNormalSprite;
        _builtIns.push_back({skin.name, skin.bodySprite, skin.headBackSprite,
            previewHead, skin.bowSprite});
    }
}

renderer::CharacterEditorRenderer::~CharacterEditorRenderer()
{
    _atlas.unload();
}

void renderer::CharacterEditorRenderer::pushState(State state)
{
    _state = std::move(state);
}

const std::vector<charactereditor::BuiltInCharacter>&
renderer::CharacterEditorRenderer::builtInCharacters() const
{
    return _builtIns;
}

charactereditor::Layout renderer::CharacterEditorRenderer::layout(
    const charactereditor::State& state) const
{
    int windowWidth = 960;
    int windowHeight = 720;
    SDL_GetWindowSize(_sdlWindow, &windowWidth, &windowHeight);
    const float width = static_cast<float>(windowWidth);
    const float height = static_cast<float>(windowHeight);

    charactereditor::Layout ui;
    ui.backButton = {24.0f, 20.0f, 112.0f, 30.0f};

    if (state.view == charactereditor::View::HUB)
    {
        const float cardWidth = std::min(280.0f, width * 0.31f);
        const float cardHeight = std::min(410.0f, height * 0.58f);
        const float gap = std::min(54.0f, width * 0.055f);
        const float startX = (width - cardWidth * 2.0f - gap) * 0.5f;
        const float startY = std::max(126.0f, (height - cardHeight) * 0.45f);
        for (int i = 0; i < 2; ++i)
        {
            charactereditor::CardLayout card;
            card.card = {startX + i * (cardWidth + gap), startY, cardWidth, cardHeight};
            ui.cards.push_back(card);
        }
        return ui;
    }

    if (state.view == charactereditor::View::GALLERY)
    {
        const std::size_t count = state.builtIns.size() + state.characters.size() + 1;
        const float gap = 14.0f;
        const float outerMargin = 32.0f;
        const float idealWidth = 150.0f;
        ui.galleryColumns = std::max(1, std::min(5,
            static_cast<int>((width - outerMargin * 2.0f + gap) / (idealWidth + gap))));
        ui.galleryColumns = std::min(ui.galleryColumns, std::max(1, static_cast<int>(count)));
        const int rows = std::max(1, static_cast<int>((count + ui.galleryColumns - 1) / ui.galleryColumns));
        const float cardWidth = std::min(170.0f,
            (width - outerMargin * 2.0f - gap * (ui.galleryColumns - 1)) / ui.galleryColumns);
        const float cardHeight = std::clamp(
            (height - 116.0f - gap * (rows - 1)) / rows, 108.0f, 164.0f);
        const float gridWidth = cardWidth * ui.galleryColumns + gap * (ui.galleryColumns - 1);
        const float gridHeight = cardHeight * rows + gap * (rows - 1);
        const float startX = (width - gridWidth) * 0.5f;
        const float startY = 82.0f + std::max(0.0f, (height - 104.0f - gridHeight) * 0.35f);

        ui.cards.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            const int column = static_cast<int>(i % static_cast<std::size_t>(ui.galleryColumns));
            const int row = static_cast<int>(i / static_cast<std::size_t>(ui.galleryColumns));
            charactereditor::CardLayout card;
            card.card = {
                startX + column * (cardWidth + gap),
                startY + row * (cardHeight + gap),
                cardWidth,
                cardHeight
            };
            const float buttonGap = 6.0f;
            const float buttonWidth = (cardWidth - 22.0f - buttonGap) * 0.5f;
            card.edit = {card.card.x + 8.0f, card.card.y + card.card.h - 30.0f,
                buttonWidth, 22.0f};
            card.remove = {card.edit.x + buttonWidth + buttonGap, card.edit.y,
                buttonWidth, 22.0f};
            ui.cards.push_back(card);
        }
        return ui;
    }

    ui.saveButton = {width - 136.0f, 20.0f, 112.0f, 30.0f};
    const float margin = 18.0f;
    const float gap = 12.0f;
    const float panelY = 92.0f;
    const float panelHeight = std::max(470.0f, height - 154.0f);
    const float toolsWidth = std::clamp(width * 0.225f, 182.0f, 235.0f);
    const float previewWidth = std::clamp(width * 0.245f, 196.0f, 258.0f);
    const float centerWidth = width - margin * 2.0f - gap * 2.0f - toolsWidth - previewWidth;
    ui.toolsPanel = {margin, panelY, toolsWidth, panelHeight};
    ui.canvasPanel = {ui.toolsPanel.x + ui.toolsPanel.w + gap, panelY, centerWidth, panelHeight};
    ui.previewPanel = {ui.canvasPanel.x + ui.canvasPanel.w + gap, panelY,
        previewWidth, panelHeight};

    const float tabGap = 6.0f;
    const float tabY = panelY + 18.0f;
    const float tabWidth = (centerWidth - 76.0f - tabGap * 2.0f) / 3.0f;
    float tabX = ui.canvasPanel.x + 38.0f;
    for (int i = 0; i < 3; ++i)
    {
        ui.canvasTabs.push_back({tabX, tabY, tabWidth, 31.0f});
        tabX += tabWidth + tabGap;
    }
    ui.previousCanvasButton = {ui.canvasPanel.x + 8.0f, tabY, 24.0f, 31.0f};
    ui.nextCanvasButton = {ui.canvasPanel.x + ui.canvasPanel.w - 32.0f, tabY, 24.0f, 31.0f};
    ui.previousAnimationButton = {ui.canvasPanel.x + 44.0f, panelY + 58.0f, 26.0f, 25.0f};
    ui.nextAnimationButton = {ui.canvasPanel.x + ui.canvasPanel.w - 70.0f,
        panelY + 58.0f, 26.0f, 25.0f};

    const float availableCanvasWidth = centerWidth - 50.0f;
    const float availableCanvasHeight = panelHeight - 205.0f;
    ui.pixelScale = std::floor(std::min(availableCanvasWidth / charactereditor::FRAME_WIDTH,
        availableCanvasHeight / charactereditor::FRAME_HEIGHT));
    ui.pixelScale = std::clamp(ui.pixelScale, 3.0f, 18.0f);
    const float canvasWidth = ui.pixelScale * charactereditor::FRAME_WIDTH;
    const float canvasHeight = ui.pixelScale * charactereditor::FRAME_HEIGHT;
    ui.pixelCanvas = {
        ui.canvasPanel.x + (ui.canvasPanel.w - canvasWidth) * 0.5f,
        panelY + 105.0f,
        canvasWidth,
        canvasHeight
    };

    const std::size_t paletteCount = state.workingCharacter.palette.size();
    const float swatchSize = std::min(34.0f, (toolsWidth - 46.0f) / 4.0f);
    const float swatchGap = 6.0f;
    const float paletteX = ui.toolsPanel.x + 18.0f;
    const float paletteY = ui.toolsPanel.y + 382.0f;
    for (std::size_t i = 0; i < paletteCount; ++i)
    {
        const int column = static_cast<int>(i % 4);
        const int row = static_cast<int>(i / 4);
        ui.paletteSwatches.push_back({paletteX + column * (swatchSize + swatchGap),
            paletteY + row * (swatchSize + swatchGap), swatchSize, swatchSize});
    }

    const float toolSize = std::min(42.0f, (toolsWidth - 54.0f) / 4.0f);
    for (int i = 0; i < 8; ++i)
        ui.toolButtons.push_back({ui.toolsPanel.x + 18.0f + (i % 4) * (toolSize + 8.0f),
            ui.toolsPanel.y + 54.0f + (i / 4) * (toolSize + 8.0f), toolSize, toolSize});

    const float wheelSize = std::min(108.0f, toolsWidth - 104.0f);
    ui.colorWheel = {ui.toolsPanel.x + 18.0f, ui.toolsPanel.y + 184.0f,
        wheelSize, wheelSize};
    ui.valueSlider = {ui.colorWheel.x + ui.colorWheel.w + 10.0f,
        ui.colorWheel.y, 18.0f, wheelSize};
    ui.alphaSlider = {ui.valueSlider.x + ui.valueSlider.w + 10.0f,
        ui.colorWheel.y, 18.0f, wheelSize};
    ui.setSwatchButton = {ui.toolsPanel.x + 84.0f, ui.toolsPanel.y + 312.0f,
        toolsWidth - 102.0f, 28.0f};

    if (state.selectedCanvas < state.workingCharacter.canvases.size()
        && state.selectedAnimation < charactereditor::ANIMATIONS.size())
    {
        const int frameCount = charactereditor::ANIMATIONS[state.selectedAnimation].frameCount;
        const int visibleCount = std::min(8, frameCount);
        const int firstVisible = std::min(static_cast<int>(state.selectedFrame),
            std::max(0, frameCount - visibleCount));
        const float availableWidth = ui.canvasPanel.w - 30.0f;
        const float frameGap = 4.0f;
        const float frameWidth = std::min(36.0f,
            (availableWidth - frameGap * (visibleCount - 1)) / visibleCount);
        const float frameHeight = std::min(56.0f,
            frameWidth * charactereditor::FRAME_HEIGHT / charactereditor::FRAME_WIDTH);
        const float stripWidth = frameWidth * visibleCount + frameGap * (visibleCount - 1);
        float frameX = ui.canvasPanel.x + (ui.canvasPanel.w - stripWidth) * 0.5f;
        const float frameY = ui.canvasPanel.y + ui.canvasPanel.h - frameHeight - 27.0f;
        for (int i = 0; i < visibleCount; ++i)
        {
            ui.frameBoxes.push_back({frameX, frameY, frameWidth, frameHeight});
            ui.frameIndices.push_back(static_cast<std::size_t>(firstVisible + i));
            frameX += frameWidth + frameGap;
        }
    }

    return ui;
}

void renderer::CharacterEditorRenderer::drawText(
    const std::string& text, float x, float y, bool centered) const
{
    if (centered)
        x -= static_cast<float>(text.size()) * TEXT_SIZE * 0.5f;
    SDL_RenderDebugText(_sdlRenderer, x, y, text.c_str());
}

void renderer::CharacterEditorRenderer::drawButton(
    const SDL_FRect& bounds, const std::string& label, bool active, float mouseX, float mouseY)
{
    const bool hovered = contains(bounds, mouseX, mouseY);
    if (active)
        SDL_SetRenderDrawColor(_sdlRenderer, 29, 111, 125, 255);
    else if (hovered)
        SDL_SetRenderDrawColor(_sdlRenderer, 55, 55, 63, 255);
    else
        SDL_SetRenderDrawColor(_sdlRenderer, 25, 25, 30, 255);
    SDL_RenderFillRect(_sdlRenderer, &bounds);
    SDL_SetRenderDrawColor(_sdlRenderer,
        active ? 92 : (hovered ? 180 : 92), active ? 222 : (hovered ? 180 : 92),
        active ? 230 : (hovered ? 180 : 92), 255);
    SDL_RenderRect(_sdlRenderer, &bounds);
    SDL_SetRenderDrawColor(_sdlRenderer, 235, 235, 239, 255);
    drawText(label, bounds.x + bounds.w * 0.5f,
        bounds.y + (bounds.h - TEXT_SIZE) * 0.5f, true);
}

void renderer::CharacterEditorRenderer::drawCustomFrame(
    const charactereditor::Canvas& canvas, std::size_t frame, const SDL_FRect& bounds, bool drawGrid)
{
    if (canvas.frameCount <= 0 || frame >= static_cast<std::size_t>(canvas.frameCount))
        return;

    const float scale = std::min(bounds.w / charactereditor::FRAME_WIDTH,
        bounds.h / charactereditor::FRAME_HEIGHT);
    const float actualWidth = scale * charactereditor::FRAME_WIDTH;
    const float actualHeight = scale * charactereditor::FRAME_HEIGHT;
    const float originX = bounds.x + (bounds.w - actualWidth) * 0.5f;
    const float originY = bounds.y + (bounds.h - actualHeight) * 0.5f;
    const std::size_t stripWidth = static_cast<std::size_t>(
        charactereditor::FRAME_WIDTH * canvas.frameCount);

    if (drawGrid)
    {
        for (int y = 0; y < charactereditor::FRAME_HEIGHT; ++y)
        {
            for (int x = 0; x < charactereditor::FRAME_WIDTH; ++x)
            {
                const Uint8 shade = ((x + y) & 1) ? 22 : 31;
                SDL_SetRenderDrawColor(_sdlRenderer, shade, shade, shade + 3, 255);
                const SDL_FRect cell = {originX + x * scale, originY + y * scale, scale, scale};
                SDL_RenderFillRect(_sdlRenderer, &cell);
            }
        }
    }

    for (int y = 0; y < charactereditor::FRAME_HEIGHT; ++y)
    {
        for (int x = 0; x < charactereditor::FRAME_WIDTH; ++x)
        {
            const std::size_t index = static_cast<std::size_t>(y) * stripWidth
                + frame * charactereditor::FRAME_WIDTH + static_cast<std::size_t>(x);
            if (index >= canvas.pixels.size())
                continue;
            const std::uint32_t color = canvas.pixels[index];
            if ((color & 0xff) == 0)
                continue;
            setColor(_sdlRenderer, color);
            const SDL_FRect pixel = {originX + x * scale, originY + y * scale, scale, scale};
            SDL_RenderFillRect(_sdlRenderer, &pixel);
        }
    }

    if (drawGrid && scale >= 5.0f)
    {
        SDL_SetRenderDrawColor(_sdlRenderer, 65, 65, 72, 155);
        for (int x = 0; x <= charactereditor::FRAME_WIDTH; ++x)
            SDL_RenderLine(_sdlRenderer, originX + x * scale, originY,
                originX + x * scale, originY + actualHeight);
        for (int y = 0; y <= charactereditor::FRAME_HEIGHT; ++y)
            SDL_RenderLine(_sdlRenderer, originX, originY + y * scale,
                originX + actualWidth, originY + y * scale);
    }
}

void renderer::CharacterEditorRenderer::drawCustomCharacter(
    const charactereditor::CustomCharacter& character, std::size_t frame, const SDL_FRect& bounds)
{
    static constexpr std::array<const char*, 3> drawOrder = {"body", "head", "bow"};
    for (const char* id : drawOrder)
    {
        const auto* part = charactereditor::CharacterStore::canvas(character, id);
        if (!part || part->frameCount <= 0)
            continue;
        drawCustomFrame(*part, std::min(frame,
            static_cast<std::size_t>(part->frameCount - 1)), bounds);
    }
}

void renderer::CharacterEditorRenderer::drawBuiltIn(
    const charactereditor::BuiltInCharacter& character, const SDL_FRect& bounds)
{
    const SpriteDef* body = _sprites.find(character.bodySprite);
    const SpriteDef* bow = _sprites.find(character.bowSprite);
    if (!body || !bow || body->frameWidth <= 0 || body->frameHeight <= 0)
        return;

    int bodyFrame = 0;
    if (const SpriteAnimation* stand = body->animation("stand"); stand && !stand->frames.empty())
        bodyFrame = stand->frames.front();
    const float scale = std::max(1.0f, std::min(bounds.w / 20.0f, bounds.h / 22.0f));
    const float anchorX = bounds.x + bounds.w * 0.5f;
    const float anchorY = bounds.y + (bounds.h - 21.0f * scale) * 0.5f + 13.0f * scale;
    constexpr bool flip = false;
    const auto frameOrigin = [](const std::vector<int>& origins, int frame, int fallback)
    {
        return frame >= 0 && static_cast<std::size_t>(frame) < origins.size()
            ? origins[frame] : fallback;
    };
    const auto partX = [&](const SpriteDef& sprite, float localOffset = 0.0f)
    {
        return anchorX + (sprite.x + localOffset - sprite.originX) * scale;
    };
    const auto drawFrame = [&](const SpriteDef& sprite, int frame, float y,
        float localX = 0.0f)
    {
        SDL_FRect dst{partX(sprite, localX), y, sprite.frameWidth * scale,
            sprite.frameHeight * scale};
        _atlas.drawFrame(_sdlRenderer, sprite.texture, sprite.frameWidth,
            sprite.frameHeight, frame, &dst, flip);
    };

    const int headXOrigin = frameOrigin(body->headXOrigins, bodyFrame, body->originX);
    const int headYOrigin = frameOrigin(body->headYOrigins, bodyFrame, body->originY);
    const auto drawHead = [&](const std::string& spriteId)
    {
        const SpriteDef* head = spriteId.empty() ? nullptr : _sprites.find(spriteId);
        if (!head)
            return;
        int frame = 0;
        if (const SpriteAnimation* idle = head->animation("idle"); idle && !idle->frames.empty())
            frame = idle->frames.front();
        const float localX = static_cast<float>(headXOrigin - body->originX);
        const float y = anchorY + (head->y - headYOrigin - head->originY) * scale;
        drawFrame(*head, frame, y, localX);
    };

    drawHead(character.headBackSprite);
    drawFrame(*body, bodyFrame, anchorY + (body->y - body->originY) * scale);
    drawHead(character.headSprite);
    if (!bow->hideBowIdle)
    {
        int bowFrame = 0;
        if (const SpriteAnimation* idle = bow->animation("idle"); idle && !idle->frames.empty())
            bowFrame = idle->frames.front();
        drawFrame(*bow, bowFrame, anchorY + (bow->y - bow->originY) * scale);
    }
}

void renderer::CharacterEditorRenderer::renderHub(
    const charactereditor::Layout& ui, float mouseX, float mouseY)
{
    int width = 960;
    int height = 720;
    SDL_GetWindowSize(_sdlWindow, &width, &height);
    constexpr SDL_Color gold{226, 170, 36, 255};

    SDL_SetRenderDrawColor(_sdlRenderer, 245, 245, 245, 255);
    drawText("+  ...   EDITOR   ...  +", width * 0.5f, 56.0f, true);
    drawButton(ui.backButton, "< MAIN MENU", false, mouseX, mouseY);

    for (std::size_t i = 0; i < ui.cards.size(); ++i)
    {
        const SDL_FRect& card = ui.cards[i].card;
        const bool enabled = i == 0;
        const bool selected = i == _state.selectedEditorCard;
        const bool hovered = contains(card, mouseX, mouseY);
        SDL_SetRenderDrawColor(_sdlRenderer, enabled ? 8 : 10, enabled ? 8 : 10,
            enabled ? 8 : 10, 255);
        SDL_RenderFillRect(_sdlRenderer, &card);
        SDL_SetRenderDrawColor(_sdlRenderer,
            enabled && (selected || hovered) ? gold.r : 70,
            enabled && (selected || hovered) ? gold.g : 70,
            enabled && (selected || hovered) ? gold.b : 74, 255);
        SDL_RenderRect(_sdlRenderer, &card);
        SDL_FRect inner{card.x + 4.0f, card.y + 4.0f, card.w - 8.0f, card.h - 8.0f};
        SDL_RenderRect(_sdlRenderer, &inner);

        SDL_SetRenderDrawColor(_sdlRenderer, enabled ? 245 : 82,
            enabled ? 245 : 82, enabled ? 245 : 86, 255);
        drawText(enabled ? "CHARACTER" : "LEVEL", card.x + card.w * 0.5f,
            card.y + 45.0f, true);
        drawText("EDITOR", card.x + card.w * 0.5f, card.y + 64.0f, true);

        const SDL_FRect preview{card.x + 32.0f, card.y + 102.0f,
            card.w - 64.0f, card.h - 150.0f};
        if (enabled && !_state.builtIns.empty())
            drawBuiltIn(_state.builtIns.front(), preview);
        else
        {
            SDL_SetRenderDrawColor(_sdlRenderer, 31, 31, 34, 255);
            SDL_RenderFillRect(_sdlRenderer, &preview);
            SDL_SetRenderDrawColor(_sdlRenderer, 66, 66, 70, 255);
            for (int step = 0; step < 5; ++step)
            {
                const float y = preview.y + 22.0f + step * (preview.h - 44.0f) / 4.0f;
                SDL_RenderLine(_sdlRenderer, preview.x + 12.0f, y,
                    preview.x + preview.w - 12.0f, y + (step % 2 ? -18.0f : 16.0f));
            }
            drawText("[ LOCKED ]", card.x + card.w * 0.5f,
                card.y + card.h - 40.0f, true);
        }
    }

    SDL_SetRenderDrawColor(_sdlRenderer, gold.r, gold.g, gold.b, 255);
    drawText("--  +  SELECT OPTION  +  --", width * 0.5f,
        static_cast<float>(height) - 62.0f, true);
    SDL_SetRenderDrawColor(_sdlRenderer, 120, 120, 124, 255);
    drawText("LEFT / RIGHT NAVIGATE    ENTER SELECT    ESC BACK", width * 0.5f,
        static_cast<float>(height) - 30.0f, true);
}

void renderer::CharacterEditorRenderer::renderGallery(
    const charactereditor::Layout& ui, float mouseX, float mouseY)
{
    int width = 960;
    int height = 720;
    SDL_GetWindowSize(_sdlWindow, &width, &height);
    SDL_SetRenderDrawColor(_sdlRenderer, 240, 240, 244, 255);
    drawText("CHARACTER EDITOR", width * 0.5f, 24.0f, true);
    SDL_SetRenderDrawColor(_sdlRenderer, 116, 116, 126, 255);
    drawText("BUILT-IN ARCHERS ARE LOCKED. CREATE YOUR OWN TO EDIT PIXELS.",
        width * 0.5f, 48.0f, true);
    drawButton(ui.backButton, "< EDITOR MENU", false, mouseX, mouseY);

    const std::size_t builtInCount = _state.builtIns.size();
    const std::size_t customCount = _state.characters.size();
    for (std::size_t i = 0; i < ui.cards.size(); ++i)
    {
        const auto& card = ui.cards[i];
        const bool selected = i == _state.selectedCard;
        const bool hovered = contains(card.card, mouseX, mouseY);
        SDL_SetRenderDrawColor(_sdlRenderer,
            selected ? 24 : (hovered ? 23 : 15),
            selected ? 68 : (hovered ? 26 : 15),
            selected ? 76 : (hovered ? 31 : 18), 255);
        SDL_RenderFillRect(_sdlRenderer, &card.card);
        SDL_SetRenderDrawColor(_sdlRenderer,
            selected ? 78 : 48, selected ? 210 : 48, selected ? 220 : 54, 255);
        SDL_RenderRect(_sdlRenderer, &card.card);

        if (i < builtInCount)
        {
            const SDL_FRect preview = {card.card.x + 12.0f, card.card.y + 28.0f,
                card.card.w - 24.0f, std::max(28.0f, card.card.h - 76.0f)};
            drawBuiltIn(_state.builtIns[i], preview);
            SDL_SetRenderDrawColor(_sdlRenderer, 235, 235, 239, 255);
            drawText(_state.builtIns[i].name, card.card.x + card.card.w * 0.5f,
                card.card.y + 10.0f, true);
            SDL_SetRenderDrawColor(_sdlRenderer, 155, 112, 78, 255);
            drawText("BUILT-IN / LOCKED", card.card.x + card.card.w * 0.5f,
                card.card.y + card.card.h - 18.0f, true);
        }
        else if (i < builtInCount + customCount)
        {
            const auto& character = _state.characters[i - builtInCount];
            SDL_SetRenderDrawColor(_sdlRenderer, 235, 235, 239, 255);
            drawText(character.name, card.card.x + card.card.w * 0.5f,
                card.card.y + 10.0f, true);
            if (!character.canvases.empty())
            {
                const SDL_FRect preview = {card.card.x + 12.0f, card.card.y + 26.0f,
                    card.card.w - 24.0f, std::max(24.0f, card.card.h - 64.0f)};
                drawCustomCharacter(character, charactereditor::IDLE_FRAME, preview);
            }
            drawButton(card.edit, "EDIT", false, mouseX, mouseY);
            drawButton(card.remove, "DELETE", false, mouseX, mouseY);
        }
        else
        {
            SDL_SetRenderDrawColor(_sdlRenderer, 78, 210, 220, 255);
            drawText("+", card.card.x + card.card.w * 0.5f, card.card.y + card.card.h * 0.38f, true);
            drawText("CREATE CHARACTER", card.card.x + card.card.w * 0.5f,
                card.card.y + card.card.h * 0.58f, true);
            SDL_SetRenderDrawColor(_sdlRenderer, 120, 120, 130, 255);
            drawText("N / ENTER", card.card.x + card.card.w * 0.5f,
                card.card.y + card.card.h - 22.0f, true);
        }
    }

    SDL_SetRenderDrawColor(_sdlRenderer, 112, 112, 122, 255);
    drawText("ARROWS SELECT   ENTER/E EDIT   N NEW   DELETE REMOVE   ESC BACK",
        width * 0.5f, static_cast<float>(height) - 18.0f, true);
    if (!_state.notice.empty())
    {
        SDL_SetRenderDrawColor(_sdlRenderer, 78, 210, 220, 255);
        drawText(_state.notice, width * 0.5f, static_cast<float>(height) - 36.0f, true);
    }
}

void renderer::CharacterEditorRenderer::renderEditor(
    const charactereditor::Layout& ui, float mouseX, float mouseY)
{
    int width = 960;
    int height = 720;
    SDL_GetWindowSize(_sdlWindow, &width, &height);
    drawButton(ui.backButton, "< CANCEL", false, mouseX, mouseY);
    drawButton(ui.saveButton, _state.dirty ? "SAVE *" : "SAVED", _state.dirty, mouseX, mouseY);

    constexpr SDL_Color gold{226, 170, 36, 255};
    SDL_SetRenderDrawColor(_sdlRenderer, 242, 242, 244, 255);
    drawText("+  ...   CHARACTER EDITOR   ...  +", width * 0.5f, 27.0f, true);
    SDL_SetRenderDrawColor(_sdlRenderer, 116, 116, 122, 255);
    drawText(_state.workingCharacter.name, width * 0.5f, 51.0f, true);

    for (const SDL_FRect panel : {ui.toolsPanel, ui.canvasPanel, ui.previewPanel})
    {
        SDL_SetRenderDrawColor(_sdlRenderer, 7, 7, 8, 255);
        SDL_RenderFillRect(_sdlRenderer, &panel);
        SDL_SetRenderDrawColor(_sdlRenderer, 82, 82, 88, 255);
        SDL_RenderRect(_sdlRenderer, &panel);
        const SDL_FRect inner{panel.x + 3.0f, panel.y + 3.0f, panel.w - 6.0f, panel.h - 6.0f};
        SDL_RenderRect(_sdlRenderer, &inner);
    }

    if (_state.selectedCanvas >= _state.workingCharacter.canvases.size()
        || _state.selectedAnimation >= charactereditor::ANIMATIONS.size())
        return;
    const auto& canvas = _state.workingCharacter.canvases[_state.selectedCanvas];
    const auto& selectedAnimation = charactereditor::ANIMATIONS[_state.selectedAnimation];

    SDL_SetRenderDrawColor(_sdlRenderer, gold.r, gold.g, gold.b, 255);
    drawText("TOOLS", ui.toolsPanel.x + 18.0f, ui.toolsPanel.y + 18.0f);
    static constexpr std::array<const char*, 8> toolLabels = {
        "P", "E", "SEL", "I", "F", "MV", "U", "R"
    };
    for (std::size_t i = 0; i < ui.toolButtons.size(); ++i)
    {
        const SDL_FRect& tool = ui.toolButtons[i];
        const bool active = (i == 0 && !_state.eraseTool) || (i == 1 && _state.eraseTool);
        const bool enabled = i < 2;
        SDL_SetRenderDrawColor(_sdlRenderer, active ? 36 : 20, active ? 31 : 20,
            active ? 15 : 23, 255);
        SDL_RenderFillRect(_sdlRenderer, &tool);
        SDL_SetRenderDrawColor(_sdlRenderer, active ? gold.r : 72,
            active ? gold.g : 72, active ? gold.b : 78, 255);
        SDL_RenderRect(_sdlRenderer, &tool);
        SDL_SetRenderDrawColor(_sdlRenderer, active ? 245 : (enabled ? 132 : 70),
            active ? 245 : (enabled ? 132 : 70), active ? 245 : (enabled ? 138 : 74), 255);
        drawText(toolLabels[i], tool.x + tool.w * 0.5f,
            tool.y + (tool.h - TEXT_SIZE) * 0.5f, true);
    }

    SDL_SetRenderDrawColor(_sdlRenderer, gold.r, gold.g, gold.b, 255);
    drawText("COLOR WHEEL", ui.toolsPanel.x + 18.0f, ui.toolsPanel.y + 166.0f);
    constexpr int wheelSegments = 48;
    std::array<SDL_Vertex, wheelSegments + 1> wheelVertices{};
    std::array<int, wheelSegments * 3> wheelIndices{};
    const float wheelCenterX = ui.colorWheel.x + ui.colorWheel.w * 0.5f;
    const float wheelCenterY = ui.colorWheel.y + ui.colorWheel.h * 0.5f;
    const float wheelRadius = ui.colorWheel.w * 0.5f;
    wheelVertices[0].position = {wheelCenterX, wheelCenterY};
    wheelVertices[0].color = {1.0f, 1.0f, 1.0f, 1.0f};
    for (int i = 0; i < wheelSegments; ++i)
    {
        const float hue = static_cast<float>(i) / wheelSegments;
        const float angle = hue * 6.28318530718f;
        wheelVertices[i + 1].position = {wheelCenterX + std::cos(angle) * wheelRadius,
            wheelCenterY + std::sin(angle) * wheelRadius};
        const std::uint32_t color = hsvColor(hue, 1.0f, 1.0f);
        wheelVertices[i + 1].color = {
            static_cast<float>((color >> 24) & 0xff) / 255.0f,
            static_cast<float>((color >> 16) & 0xff) / 255.0f,
            static_cast<float>((color >> 8) & 0xff) / 255.0f, 1.0f};
        wheelIndices[i * 3] = 0;
        wheelIndices[i * 3 + 1] = i + 1;
        wheelIndices[i * 3 + 2] = (i + 1) % wheelSegments + 1;
    }
    SDL_RenderGeometry(_sdlRenderer, nullptr, wheelVertices.data(), wheelVertices.size(),
        wheelIndices.data(), wheelIndices.size());
    const float selectedAngle = _state.colorHue * 6.28318530718f;
    const SDL_FRect wheelCursor{
        wheelCenterX + std::cos(selectedAngle) * wheelRadius * _state.colorSaturation - 3.0f,
        wheelCenterY + std::sin(selectedAngle) * wheelRadius * _state.colorSaturation - 3.0f,
        6.0f, 6.0f};
    SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 255, 255);
    SDL_RenderRect(_sdlRenderer, &wheelCursor);

    for (int y = 0; y < static_cast<int>(ui.valueSlider.h); ++y)
    {
        const float value = 1.0f - static_cast<float>(y) / ui.valueSlider.h;
        setColor(_sdlRenderer, hsvColor(_state.colorHue, _state.colorSaturation, value));
        SDL_RenderLine(_sdlRenderer, ui.valueSlider.x, ui.valueSlider.y + y,
            ui.valueSlider.x + ui.valueSlider.w, ui.valueSlider.y + y);
        setColor(_sdlRenderer, hsvColor(_state.colorHue, _state.colorSaturation,
            _state.colorValue, value));
        SDL_RenderLine(_sdlRenderer, ui.alphaSlider.x, ui.alphaSlider.y + y,
            ui.alphaSlider.x + ui.alphaSlider.w, ui.alphaSlider.y + y);
    }
    SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 255, 255);
    SDL_RenderRect(_sdlRenderer, &ui.valueSlider);
    SDL_RenderRect(_sdlRenderer, &ui.alphaSlider);
    drawText("V", ui.valueSlider.x + ui.valueSlider.w * 0.5f,
        ui.valueSlider.y + ui.valueSlider.h + 5.0f, true);
    drawText("A", ui.alphaSlider.x + ui.alphaSlider.w * 0.5f,
        ui.alphaSlider.y + ui.alphaSlider.h + 5.0f, true);
    const SDL_FRect activeColorRect{ui.toolsPanel.x + 18.0f,
        ui.toolsPanel.y + 312.0f, 58.0f, 28.0f};
    setColor(_sdlRenderer, _state.activeColor);
    SDL_RenderFillRect(_sdlRenderer, &activeColorRect);
    SDL_SetRenderDrawColor(_sdlRenderer, 220, 220, 224, 255);
    SDL_RenderRect(_sdlRenderer, &activeColorRect);
    drawButton(ui.setSwatchButton, "SET SWATCH", false, mouseX, mouseY);
    SDL_SetRenderDrawColor(_sdlRenderer, 112, 112, 118, 255);
    drawText("PALETTE / CLICK", ui.toolsPanel.x + 18.0f, ui.toolsPanel.y + 358.0f);

    drawButton(ui.previousCanvasButton, "<", false, mouseX, mouseY);
    drawButton(ui.nextCanvasButton, ">", false, mouseX, mouseY);
    for (std::size_t i = 0; i < ui.canvasTabs.size(); ++i)
    {
        const SDL_FRect& tab = ui.canvasTabs[i];
        const bool selected = i == _state.selectedCanvas;
        SDL_SetRenderDrawColor(_sdlRenderer, selected ? 37 : 22,
            selected ? 31 : 22, selected ? 12 : 25, 255);
        SDL_RenderFillRect(_sdlRenderer, &tab);
        SDL_SetRenderDrawColor(_sdlRenderer, selected ? gold.r : 74,
            selected ? gold.g : 74, selected ? gold.b : 80, 255);
        SDL_RenderRect(_sdlRenderer, &tab);
        SDL_SetRenderDrawColor(_sdlRenderer, selected ? gold.r : 178,
            selected ? gold.g : 178, selected ? gold.b : 182, 255);
        const std::string label = i < _state.workingCharacter.canvases.size()
            ? _state.workingCharacter.canvases[i].label : "PART";
        drawText(label, tab.x + tab.w * 0.5f,
            tab.y + (tab.h - TEXT_SIZE) * 0.5f, true);
    }

    drawButton(ui.previousAnimationButton, "<", false, mouseX, mouseY);
    drawButton(ui.nextAnimationButton, ">", false, mouseX, mouseY);
    SDL_SetRenderDrawColor(_sdlRenderer, gold.r, gold.g, gold.b, 255);
    drawText(std::string(selectedAnimation.label),
        ui.canvasPanel.x + ui.canvasPanel.w * 0.5f, ui.canvasPanel.y + 66.0f, true);
    SDL_SetRenderDrawColor(_sdlRenderer, 130, 130, 136, 255);
    drawText(canvas.label + "  |  12x20", ui.canvasPanel.x + ui.canvasPanel.w * 0.5f,
        ui.canvasPanel.y + 87.0f, true);

    const std::size_t selectedPose = static_cast<std::size_t>(selectedAnimation.firstFrame)
        + _state.selectedFrame;
    drawCustomFrame(canvas, selectedPose, ui.pixelCanvas, true);
    SDL_SetRenderDrawColor(_sdlRenderer, gold.r, gold.g, gold.b, 255);
    SDL_RenderRect(_sdlRenderer, &ui.pixelCanvas);
    drawText("FRAME " + std::to_string(_state.selectedFrame + 1) + " / "
        + std::to_string(selectedAnimation.frameCount),
        ui.pixelCanvas.x + ui.pixelCanvas.w * 0.5f, ui.pixelCanvas.y - 18.0f, true);

    const std::size_t animatedLocal = _state.previewPlaying
        ? animationFrame(selectedAnimation.frameCount, selectedAnimation.delayMs)
        : std::min(_state.pausedPreviewFrame,
            static_cast<std::size_t>(selectedAnimation.frameCount - 1));
    const std::size_t animated = static_cast<std::size_t>(selectedAnimation.firstFrame)
        + animatedLocal;
    for (std::size_t i = 0; i < ui.frameBoxes.size(); ++i)
    {
        const std::size_t localFrame = i < ui.frameIndices.size() ? ui.frameIndices[i] : i;
        const std::size_t pose = static_cast<std::size_t>(selectedAnimation.firstFrame) + localFrame;
        drawCustomFrame(canvas, pose, ui.frameBoxes[i]);
        SDL_SetRenderDrawColor(_sdlRenderer,
            localFrame == animatedLocal ? gold.r : (localFrame == _state.selectedFrame ? 230 : 62),
            localFrame == animatedLocal ? gold.g : (localFrame == _state.selectedFrame ? 230 : 62),
            localFrame == animatedLocal ? gold.b : (localFrame == _state.selectedFrame ? 230 : 70), 255);
        SDL_RenderRect(_sdlRenderer, &ui.frameBoxes[i]);
        drawText("F" + std::to_string(localFrame + 1),
            ui.frameBoxes[i].x + ui.frameBoxes[i].w * 0.5f,
            ui.frameBoxes[i].y - 12.0f, true);
    }

    SDL_SetRenderDrawColor(_sdlRenderer, gold.r, gold.g, gold.b, 255);
    drawText("PREVIEW", ui.previewPanel.x + ui.previewPanel.w * 0.5f,
        ui.previewPanel.y + 18.0f, true);
    SDL_SetRenderDrawColor(_sdlRenderer, 145, 145, 151, 255);
    drawText(std::string(selectedAnimation.label), ui.previewPanel.x + ui.previewPanel.w * 0.5f,
        ui.previewPanel.y + 48.0f, true);
    const float previewHeight = std::min(260.0f, ui.previewPanel.h * 0.48f);
    const SDL_FRect previewBounds{ui.previewPanel.x + 18.0f, ui.previewPanel.y + 80.0f,
        ui.previewPanel.w - 36.0f, previewHeight};
    drawCustomCharacter(_state.workingCharacter, animated, previewBounds);
    const float floorY = previewBounds.y + previewBounds.h;
    SDL_SetRenderDrawColor(_sdlRenderer, 62, 62, 68, 255);
    SDL_RenderLine(_sdlRenderer, ui.previewPanel.x + 14.0f, floorY,
        ui.previewPanel.x + ui.previewPanel.w - 14.0f, floorY);

    const int thumbCount = std::min(4, selectedAnimation.frameCount);
    const int thumbStart = std::min(static_cast<int>(animatedLocal),
        std::max(0, selectedAnimation.frameCount - thumbCount));
    const float thumbGap = 5.0f;
    const float thumbWidth = (ui.previewPanel.w - 28.0f
        - thumbGap * (thumbCount - 1)) / thumbCount;
    const float thumbY = ui.previewPanel.y + ui.previewPanel.h - 145.0f;
    for (int i = 0; i < thumbCount; ++i)
    {
        SDL_FRect thumb{ui.previewPanel.x + 14.0f + i * (thumbWidth + thumbGap),
            thumbY, thumbWidth, 75.0f};
        const int pose = selectedAnimation.firstFrame + thumbStart + i;
        drawCustomCharacter(_state.workingCharacter, pose, thumb);
        SDL_SetRenderDrawColor(_sdlRenderer, pose == static_cast<int>(animated) ? gold.r : 64,
            pose == static_cast<int>(animated) ? gold.g : 64,
            pose == static_cast<int>(animated) ? gold.b : 70, 255);
        SDL_RenderRect(_sdlRenderer, &thumb);
    }
    SDL_SetRenderDrawColor(_sdlRenderer, 120, 120, 126, 255);
    drawText("SPACE  PLAY / PAUSE",
        ui.previewPanel.x + ui.previewPanel.w * 0.5f,
        ui.previewPanel.y + ui.previewPanel.h - 38.0f, true);

    for (std::size_t i = 0; i < ui.paletteSwatches.size(); ++i)
    {
        const SDL_FRect& swatch = ui.paletteSwatches[i];
        const std::uint32_t color = _state.workingCharacter.palette[i];
        if ((color & 0xff) == 0)
        {
            SDL_SetRenderDrawColor(_sdlRenderer, 38, 38, 44, 255);
            SDL_RenderFillRect(_sdlRenderer, &swatch);
            SDL_SetRenderDrawColor(_sdlRenderer, 180, 78, 90, 255);
            SDL_RenderLine(_sdlRenderer, swatch.x + 4.0f, swatch.y + 4.0f,
                swatch.x + swatch.w - 4.0f, swatch.y + swatch.h - 4.0f);
            SDL_RenderLine(_sdlRenderer, swatch.x + swatch.w - 4.0f, swatch.y + 4.0f,
                swatch.x + 4.0f, swatch.y + swatch.h - 4.0f);
        }
        else
        {
            setColor(_sdlRenderer, color);
            SDL_RenderFillRect(_sdlRenderer, &swatch);
        }
        SDL_SetRenderDrawColor(_sdlRenderer, i == _state.selectedPalette ? gold.r : 82,
            i == _state.selectedPalette ? gold.g : 82,
            i == _state.selectedPalette ? gold.b : 90, 255);
        SDL_RenderRect(_sdlRenderer, &swatch);
    }

    SDL_SetRenderDrawColor(_sdlRenderer, 112, 112, 122, 255);
    drawText("Q/E PART   UP/DOWN ANIMATION   A/D FRAME   LEFT PAINT   RIGHT ERASE   S SAVE",
        width * 0.5f, static_cast<float>(height) - 18.0f, true);
    if (!_state.notice.empty())
    {
        SDL_SetRenderDrawColor(_sdlRenderer, gold.r, gold.g, gold.b, 255);
        drawText(_state.notice, width * 0.5f, static_cast<float>(height) - 37.0f, true);
    }
}

void renderer::CharacterEditorRenderer::render()
{
    SDL_SetRenderDrawBlendMode(_sdlRenderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(_sdlRenderer, 0, 0, 0, 255);
    SDL_RenderClear(_sdlRenderer);

    float mouseX = 0.0f;
    float mouseY = 0.0f;
    SDL_GetMouseState(&mouseX, &mouseY);
    const charactereditor::Layout ui = layout(_state);
    if (_state.view == charactereditor::View::HUB)
        renderHub(ui, mouseX, mouseY);
    else if (_state.view == charactereditor::View::GALLERY)
        renderGallery(ui, mouseX, mouseY);
    else
        renderEditor(ui, mouseX, mouseY);

    SDL_RenderPresent(_sdlRenderer);
}
