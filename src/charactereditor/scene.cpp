#include "scene.hpp"
#include "CharacterEditorRenderer.hpp"
#include "../game.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <thread>

namespace
{
    bool contains(const SDL_FRect& rect, float x, float y)
    {
        return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
    }

    std::uint32_t hsvColor(float hue, float saturation, float value, float alpha)
    {
        hue = hue - std::floor(hue);
        saturation = std::clamp(saturation, 0.0f, 1.0f);
        value = std::clamp(value, 0.0f, 1.0f);
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
}

charactereditor::Scene::Scene(Game& game) :
    _Scene(game),
    _renderer(std::make_unique<renderer::CharacterEditorRenderer>(
        game.getWindow().get(), game.getRenderer().get()))
{
    _state.builtIns = _renderer->builtInCharacters();
    refreshCharacters();
}

charactereditor::Scene::~Scene() = default;

void charactereditor::Scene::activate(SceneContext&)
{
    std::string error;
    _store.reload(&error);
    refreshCharacters();
    _state.view = View::HUB;
    _state.selectedEditorCard = 0;
    _state.notice = error.empty() ? std::string{} : "LOAD FAILED: " + error;

    const bool* keys = SDL_GetKeyboardState(nullptr);
    std::copy(keys, keys + SDL_SCANCODE_COUNT, _previousKeys.begin());
    float x = 0.0f;
    float y = 0.0f;
    _previousMouseButtons = static_cast<std::uint32_t>(SDL_GetMouseState(&x, &y));
}

void charactereditor::Scene::deactivate()
{
}

bool charactereditor::Scene::keyPressed(SDL_Scancode key, const bool* keys) const
{
    return keys[key] && !_previousKeys[static_cast<std::size_t>(key)];
}

void charactereditor::Scene::refreshCharacters()
{
    _state.characters = _store.characters();
    const std::size_t cardCount = _state.builtIns.size() + _state.characters.size() + 1;
    if (_state.selectedCard >= cardCount)
        _state.selectedCard = cardCount - 1;
}

void charactereditor::Scene::createCharacter()
{
    std::size_t suffix = _state.characters.size() + 1;
    std::string name;
    bool exists = false;
    do
    {
        name = "Custom Archer " + std::to_string(suffix++);
        exists = std::any_of(_state.characters.begin(), _state.characters.end(),
            [&](const CustomCharacter& character) { return character.name == name; });
    } while (exists);

    _state.workingCharacter = _store.makeCharacter(name);
    _state.selectedCanvas = 0;
    _state.selectedAnimation = 0;
    _state.selectedFrame = 0;
    _state.selectedPalette = std::min<std::size_t>(4,
        _state.workingCharacter.palette.empty() ? 0 : _state.workingCharacter.palette.size() - 1);
    _state.creating = true;
    _state.dirty = true;
    _state.eraseTool = false;
    _state.previewPlaying = true;
    _state.pausedPreviewFrame = 0;
    if (!_state.workingCharacter.palette.empty())
        setActiveColor(_state.workingCharacter.palette[_state.selectedPalette]);
    _state.notice = "NEW CHARACTER - SAVE TO ADD IT TO THE GAME";
    _state.view = View::EDITOR;
}

void charactereditor::Scene::editCharacter(std::size_t customIndex)
{
    if (customIndex >= _state.characters.size())
        return;

    _state.workingCharacter = _state.characters[customIndex];
    _state.selectedCanvas = 0;
    _state.selectedAnimation = 0;
    _state.selectedFrame = 0;
    _state.selectedPalette = std::min<std::size_t>(4,
        _state.workingCharacter.palette.empty() ? 0 : _state.workingCharacter.palette.size() - 1);
    _state.creating = false;
    _state.dirty = false;
    _state.eraseTool = false;
    _state.previewPlaying = true;
    _state.pausedPreviewFrame = 0;
    if (!_state.workingCharacter.palette.empty())
        setActiveColor(_state.workingCharacter.palette[_state.selectedPalette]);
    _state.notice.clear();
    _state.view = View::EDITOR;
}

void charactereditor::Scene::closeEditor()
{
    _state.view = View::GALLERY;
    _state.notice.clear();
}

void charactereditor::Scene::saveCharacter()
{
    std::string error;
    if (!_store.upsert(_state.workingCharacter, &error))
    {
        _state.notice = "SAVE FAILED: " + error;
        return;
    }
    if (!_store.reload(&error))
    {
        _state.notice = "RELOAD FAILED: " + error;
        return;
    }
    refreshCharacters();
    _state.creating = false;
    _state.dirty = false;
    _state.notice = "SAVED - AVAILABLE IN CHARACTER SELECT";
}

void charactereditor::Scene::eraseCharacter(std::size_t customIndex)
{
    if (customIndex >= _state.characters.size())
        return;

    std::string error;
    if (!_store.erase(_state.characters[customIndex].id, &error))
    {
        _state.notice = "DELETE FAILED: " + error;
        return;
    }
    if (!_store.reload(&error))
    {
        _state.notice = "RELOAD FAILED: " + error;
        return;
    }
    refreshCharacters();
    _state.notice = "CUSTOM CHARACTER DELETED";
}

void charactereditor::Scene::selectCanvas(int offset)
{
    const std::size_t count = _state.workingCharacter.canvases.size();
    if (count == 0)
        return;

    const int wrapped = (static_cast<int>(_state.selectedCanvas) + offset % static_cast<int>(count)
        + static_cast<int>(count)) % static_cast<int>(count);
    _state.selectedCanvas = static_cast<std::size_t>(wrapped);
    _state.selectedFrame = 0;
}

void charactereditor::Scene::selectFrame(int offset)
{
    if (_state.selectedAnimation >= ANIMATIONS.size())
        return;

    const std::size_t count = static_cast<std::size_t>(ANIMATIONS[_state.selectedAnimation].frameCount);
    const int wrapped = (static_cast<int>(_state.selectedFrame) + offset % static_cast<int>(count)
        + static_cast<int>(count)) % static_cast<int>(count);
    _state.selectedFrame = static_cast<std::size_t>(wrapped);
}

void charactereditor::Scene::selectAnimation(int offset)
{
    const int count = static_cast<int>(ANIMATIONS.size());
    const int wrapped = (static_cast<int>(_state.selectedAnimation) + offset % count + count) % count;
    _state.selectedAnimation = static_cast<std::size_t>(wrapped);
    _state.selectedFrame = 0;
    _state.pausedPreviewFrame = 0;
}

void charactereditor::Scene::setActiveColor(std::uint32_t color)
{
    _state.activeColor = color;
    const float r = static_cast<float>((color >> 24) & 0xff) / 255.0f;
    const float g = static_cast<float>((color >> 16) & 0xff) / 255.0f;
    const float b = static_cast<float>((color >> 8) & 0xff) / 255.0f;
    const float maximum = std::max({r, g, b});
    const float minimum = std::min({r, g, b});
    const float delta = maximum - minimum;
    _state.colorValue = maximum;
    _state.colorSaturation = maximum <= 0.0f ? 0.0f : delta / maximum;
    if (delta > 0.0f)
    {
        if (maximum == r)
            _state.colorHue = std::fmod((g - b) / delta, 6.0f) / 6.0f;
        else if (maximum == g)
            _state.colorHue = ((b - r) / delta + 2.0f) / 6.0f;
        else
            _state.colorHue = ((r - g) / delta + 4.0f) / 6.0f;
        if (_state.colorHue < 0.0f)
            _state.colorHue += 1.0f;
    }
    _state.colorAlpha = static_cast<float>(color & 0xff) / 255.0f;
}

void charactereditor::Scene::paintAt(float mouseX, float mouseY, bool erase, const Layout& layout)
{
    if (_state.selectedCanvas >= _state.workingCharacter.canvases.size()
        || !contains(layout.pixelCanvas, mouseX, mouseY) || layout.pixelScale <= 0.0f)
        return;

    auto& canvas = _state.workingCharacter.canvases[_state.selectedCanvas];
    const int pixelX = static_cast<int>((mouseX - layout.pixelCanvas.x) / layout.pixelScale);
    const int pixelY = static_cast<int>((mouseY - layout.pixelCanvas.y) / layout.pixelScale);
    if (pixelX < 0 || pixelX >= FRAME_WIDTH || pixelY < 0 || pixelY >= FRAME_HEIGHT)
        return;

    if (_state.selectedAnimation >= ANIMATIONS.size())
        return;
    const auto& animation = ANIMATIONS[_state.selectedAnimation];
    const std::size_t poseFrame = static_cast<std::size_t>(animation.firstFrame)
        + _state.selectedFrame;
    const std::size_t stripWidth = static_cast<std::size_t>(FRAME_WIDTH * canvas.frameCount);
    const std::size_t index = static_cast<std::size_t>(pixelY) * stripWidth
        + poseFrame * FRAME_WIDTH + static_cast<std::size_t>(pixelX);
    if (index >= canvas.pixels.size())
        return;

    const std::uint32_t color = erase ? 0 : _state.activeColor;

    if (canvas.pixels[index] != color)
    {
        canvas.pixels[index] = color;
        _state.dirty = true;
        _state.notice = "UNSAVED CHANGES";
    }
}

_Scene::UpdateReturnStatus charactereditor::Scene::updateGallery(
    const bool* keys, float mouseX, float mouseY, bool leftPressed, const Layout& layout)
{
    const std::size_t total = _state.builtIns.size() + _state.characters.size() + 1;
    const std::size_t customStart = _state.builtIns.size();
    const std::size_t createIndex = total - 1;

    if (keyPressed(SDL_SCANCODE_ESCAPE, keys)
        || (leftPressed && contains(layout.backButton, mouseX, mouseY)))
    {
        _state.view = View::HUB;
        return UpdateReturnStatus::STAY;
    }

    if (keyPressed(SDL_SCANCODE_LEFT, keys) && _state.selectedCard > 0)
        --_state.selectedCard;
    if (keyPressed(SDL_SCANCODE_RIGHT, keys) && _state.selectedCard + 1 < total)
        ++_state.selectedCard;
    if (keyPressed(SDL_SCANCODE_UP, keys))
    {
        const std::size_t columns = static_cast<std::size_t>(layout.galleryColumns);
        _state.selectedCard = _state.selectedCard >= columns ? _state.selectedCard - columns : 0;
    }
    if (keyPressed(SDL_SCANCODE_DOWN, keys))
    {
        const std::size_t columns = static_cast<std::size_t>(layout.galleryColumns);
        _state.selectedCard = std::min(total - 1, _state.selectedCard + columns);
    }

    if (keyPressed(SDL_SCANCODE_N, keys))
    {
        createCharacter();
        return UpdateReturnStatus::STAY;
    }

    if (leftPressed)
    {
        for (std::size_t i = 0; i < layout.cards.size(); ++i)
        {
            if (i >= customStart && i < createIndex)
            {
                const std::size_t customIndex = i - customStart;
                if (contains(layout.cards[i].edit, mouseX, mouseY))
                {
                    _state.selectedCard = i;
                    editCharacter(customIndex);
                    return UpdateReturnStatus::STAY;
                }
                if (contains(layout.cards[i].remove, mouseX, mouseY))
                {
                    _state.selectedCard = i;
                    eraseCharacter(customIndex);
                    return UpdateReturnStatus::STAY;
                }
            }

            if (contains(layout.cards[i].card, mouseX, mouseY))
            {
                _state.selectedCard = i;
                if (i == createIndex)
                    createCharacter();
                break;
            }
        }
    }

    if (keyPressed(SDL_SCANCODE_DELETE, keys)
        && _state.selectedCard >= customStart && _state.selectedCard < createIndex)
    {
        eraseCharacter(_state.selectedCard - customStart);
        return UpdateReturnStatus::STAY;
    }

    if (keyPressed(SDL_SCANCODE_RETURN, keys) || keyPressed(SDL_SCANCODE_KP_ENTER, keys)
        || keyPressed(SDL_SCANCODE_E, keys))
    {
        if (_state.selectedCard == createIndex)
            createCharacter();
        else if (_state.selectedCard >= customStart)
            editCharacter(_state.selectedCard - customStart);
    }

    return UpdateReturnStatus::STAY;
}

_Scene::UpdateReturnStatus charactereditor::Scene::updateHub(
    const bool* keys, float mouseX, float mouseY, bool leftPressed, const Layout& layout)
{
    if (keyPressed(SDL_SCANCODE_ESCAPE, keys)
        || (leftPressed && contains(layout.backButton, mouseX, mouseY)))
        return UpdateReturnStatus::SWITCH_MAINMENU;

    if (keyPressed(SDL_SCANCODE_LEFT, keys))
        _state.selectedEditorCard = 0;
    if (keyPressed(SDL_SCANCODE_RIGHT, keys))
        _state.selectedEditorCard = 0;

    if (leftPressed)
    {
        for (std::size_t i = 0; i < layout.cards.size(); ++i)
        {
            if (!contains(layout.cards[i].card, mouseX, mouseY))
                continue;
            if (i == 0)
            {
                _state.selectedEditorCard = 0;
                _state.view = View::GALLERY;
            }
            return UpdateReturnStatus::STAY;
        }
    }

    if ((keyPressed(SDL_SCANCODE_RETURN, keys) || keyPressed(SDL_SCANCODE_KP_ENTER, keys))
        && _state.selectedEditorCard == 0)
        _state.view = View::GALLERY;

    return UpdateReturnStatus::STAY;
}

void charactereditor::Scene::updateEditor(
    const bool* keys, float mouseX, float mouseY, bool leftPressed, bool leftDown,
    bool rightDown, const Layout& layout)
{
    if (keyPressed(SDL_SCANCODE_ESCAPE, keys)
        || (leftPressed && contains(layout.backButton, mouseX, mouseY)))
    {
        closeEditor();
        return;
    }

    if (keyPressed(SDL_SCANCODE_S, keys)
        || (leftPressed && contains(layout.saveButton, mouseX, mouseY)))
        saveCharacter();

    if (keyPressed(SDL_SCANCODE_Q, keys)
        || (leftPressed && contains(layout.previousCanvasButton, mouseX, mouseY)))
        selectCanvas(-1);
    if (keyPressed(SDL_SCANCODE_E, keys)
        || (leftPressed && contains(layout.nextCanvasButton, mouseX, mouseY)))
        selectCanvas(1);
    if (keyPressed(SDL_SCANCODE_A, keys))
        selectFrame(-1);
    if (keyPressed(SDL_SCANCODE_D, keys))
        selectFrame(1);
    if (keyPressed(SDL_SCANCODE_UP, keys)
        || (leftPressed && contains(layout.previousAnimationButton, mouseX, mouseY)))
        selectAnimation(-1);
    if (keyPressed(SDL_SCANCODE_DOWN, keys)
        || (leftPressed && contains(layout.nextAnimationButton, mouseX, mouseY)))
        selectAnimation(1);
    if (keyPressed(SDL_SCANCODE_SPACE, keys))
    {
        if (_state.previewPlaying && _state.selectedAnimation < ANIMATIONS.size())
        {
            const AnimationSpec& animation = ANIMATIONS[_state.selectedAnimation];
            _state.pausedPreviewFrame = static_cast<std::size_t>((SDL_GetTicks()
                / std::max(1, animation.delayMs)) % animation.frameCount);
        }
        _state.previewPlaying = !_state.previewPlaying;
    }

    for (int key = SDL_SCANCODE_1; key <= SDL_SCANCODE_9; ++key)
    {
        if (keyPressed(static_cast<SDL_Scancode>(key), keys))
        {
            const std::size_t paletteIndex = static_cast<std::size_t>(key - SDL_SCANCODE_1);
            if (paletteIndex < _state.workingCharacter.palette.size())
            {
                _state.selectedPalette = paletteIndex;
                setActiveColor(_state.workingCharacter.palette[paletteIndex]);
            }
        }
    }

    if (leftDown && contains(layout.colorWheel, mouseX, mouseY))
    {
        const float centerX = layout.colorWheel.x + layout.colorWheel.w * 0.5f;
        const float centerY = layout.colorWheel.y + layout.colorWheel.h * 0.5f;
        const float dx = mouseX - centerX;
        const float dy = mouseY - centerY;
        const float radius = layout.colorWheel.w * 0.5f;
        const float distance = std::sqrt(dx * dx + dy * dy);
        if (distance <= radius)
        {
            constexpr float twoPi = 6.28318530718f;
            _state.colorHue = std::atan2(dy, dx) / twoPi;
            if (_state.colorHue < 0.0f)
                _state.colorHue += 1.0f;
            _state.colorSaturation = std::clamp(distance / radius, 0.0f, 1.0f);
            _state.activeColor = hsvColor(_state.colorHue, _state.colorSaturation,
                _state.colorValue, _state.colorAlpha);
            _state.eraseTool = false;
        }
    }
    if (leftDown && contains(layout.valueSlider, mouseX, mouseY))
    {
        _state.colorValue = std::clamp(1.0f
            - (mouseY - layout.valueSlider.y) / layout.valueSlider.h, 0.0f, 1.0f);
        _state.activeColor = hsvColor(_state.colorHue, _state.colorSaturation,
            _state.colorValue, _state.colorAlpha);
        _state.eraseTool = false;
    }
    if (leftDown && contains(layout.alphaSlider, mouseX, mouseY))
    {
        _state.colorAlpha = std::clamp(1.0f
            - (mouseY - layout.alphaSlider.y) / layout.alphaSlider.h, 0.0f, 1.0f);
        _state.activeColor = hsvColor(_state.colorHue, _state.colorSaturation,
            _state.colorValue, _state.colorAlpha);
        _state.eraseTool = false;
    }

    if (leftPressed)
    {
        if (!layout.toolButtons.empty() && contains(layout.toolButtons[0], mouseX, mouseY))
            _state.eraseTool = false;
        if (layout.toolButtons.size() > 1 && contains(layout.toolButtons[1], mouseX, mouseY))
            _state.eraseTool = true;
        for (std::size_t i = 0; i < layout.canvasTabs.size(); ++i)
        {
            if (contains(layout.canvasTabs[i], mouseX, mouseY))
            {
                _state.selectedCanvas = i;
                _state.selectedFrame = 0;
            }
        }
        for (std::size_t i = 0; i < layout.paletteSwatches.size(); ++i)
        {
            if (contains(layout.paletteSwatches[i], mouseX, mouseY))
            {
                _state.selectedPalette = i;
                _state.eraseTool = false;
                setActiveColor(_state.workingCharacter.palette[i]);
            }
        }
        if (contains(layout.setSwatchButton, mouseX, mouseY)
            && _state.selectedPalette < _state.workingCharacter.palette.size())
        {
            _state.workingCharacter.palette[_state.selectedPalette] = _state.activeColor;
            _state.dirty = true;
            _state.notice = "COLOR SAVED TO SELECTED SWATCH";
        }
        for (std::size_t i = 0; i < layout.frameBoxes.size(); ++i)
        {
            if (contains(layout.frameBoxes[i], mouseX, mouseY))
                _state.selectedFrame = i < layout.frameIndices.size() ? layout.frameIndices[i] : i;
        }
    }

    if (leftDown || rightDown)
        paintAt(mouseX, mouseY, rightDown || (leftDown && _state.eraseTool), layout);
}

_Scene::UpdateReturnStatus charactereditor::Scene::update()
{
    const tick_t currentTick = Game::currentTick();
    const bool* keys = SDL_GetKeyboardState(nullptr);
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    const std::uint32_t mouseButtons = static_cast<std::uint32_t>(SDL_GetMouseState(&mouseX, &mouseY));
    const bool leftDown = (mouseButtons & SDL_BUTTON_LMASK) != 0;
    const bool rightDown = (mouseButtons & SDL_BUTTON_RMASK) != 0;
    const bool leftPressed = leftDown && (_previousMouseButtons & SDL_BUTTON_LMASK) == 0;
    const Layout ui = _renderer->layout(_state);

    UpdateReturnStatus status = UpdateReturnStatus::STAY;
    if (_state.view == View::HUB)
        status = updateHub(keys, mouseX, mouseY, leftPressed, ui);
    else if (_state.view == View::GALLERY)
        status = updateGallery(keys, mouseX, mouseY, leftPressed, ui);
    else
        updateEditor(keys, mouseX, mouseY, leftPressed, leftDown, rightDown, ui);

    std::copy(keys, keys + SDL_SCANCODE_COUNT, _previousKeys.begin());
    _previousMouseButtons = mouseButtons;

    if (status != UpdateReturnStatus::STAY)
        return status;

    _renderer->pushState(_state);
    _renderer->render();
    std::this_thread::sleep_until(Game::nextTickTime(currentTick));
    return UpdateReturnStatus::STAY;
}
