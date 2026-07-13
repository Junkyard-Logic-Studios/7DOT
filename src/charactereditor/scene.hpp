#pragma once

#include "../scene.hpp"
#include "CharacterStore.hpp"
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_scancode.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class SceneContext;

namespace renderer
{
    class CharacterEditorRenderer;
}

namespace charactereditor
{

    enum class View
    {
        HUB,
        GALLERY,
        EDITOR
    };

    struct BuiltInCharacter
    {
        std::string name;
        std::string bodySprite;
        std::string headBackSprite;
        std::string headSprite;
        std::string bowSprite;
    };

    struct State
    {
        View view = View::HUB;
        std::vector<BuiltInCharacter> builtIns;
        std::vector<CustomCharacter> characters;
        std::size_t selectedEditorCard = 0;
        std::size_t selectedCard = 0;

        CustomCharacter workingCharacter;
        std::size_t selectedCanvas = 0;
        std::size_t selectedAnimation = 0;
        std::size_t selectedFrame = 0;
        std::size_t selectedPalette = 0;
        bool creating = false;
        bool dirty = false;
        bool eraseTool = false;
        bool previewPlaying = true;
        std::size_t pausedPreviewFrame = 0;
        std::uint32_t activeColor = 0xffffffff;
        float colorHue = 0.0f;
        float colorSaturation = 0.0f;
        float colorValue = 1.0f;
        float colorAlpha = 1.0f;
        std::string notice;
    };

    struct CardLayout
    {
        SDL_FRect card{};
        SDL_FRect edit{};
        SDL_FRect remove{};
    };

    struct Layout
    {
        SDL_FRect backButton{};
        SDL_FRect saveButton{};
        SDL_FRect previousCanvasButton{};
        SDL_FRect nextCanvasButton{};
        SDL_FRect previousAnimationButton{};
        SDL_FRect nextAnimationButton{};
        SDL_FRect toolsPanel{};
        SDL_FRect canvasPanel{};
        SDL_FRect pixelCanvas{};
        SDL_FRect previewPanel{};
        SDL_FRect colorWheel{};
        SDL_FRect valueSlider{};
        SDL_FRect alphaSlider{};
        SDL_FRect setSwatchButton{};
        std::vector<SDL_FRect> toolButtons;
        std::vector<CardLayout> cards;
        std::vector<SDL_FRect> canvasTabs;
        std::vector<SDL_FRect> paletteSwatches;
        std::vector<SDL_FRect> frameBoxes;
        std::vector<std::size_t> frameIndices;
        int galleryColumns = 1;
        float pixelScale = 1.0f;
    };

    class Scene : public _Scene
    {
    public:
        explicit Scene(Game& game);
        ~Scene();

        void activate(SceneContext& context) override;
        void deactivate() override;
        UpdateReturnStatus update() override;

    private:
        bool keyPressed(SDL_Scancode key, const bool* keys) const;
        void refreshCharacters();
        void createCharacter();
        void editCharacter(std::size_t customIndex);
        void closeEditor();
        void saveCharacter();
        void eraseCharacter(std::size_t customIndex);
        void selectCanvas(int offset);
        void selectAnimation(int offset);
        void selectFrame(int offset);
        void setActiveColor(std::uint32_t color);
        void paintAt(float mouseX, float mouseY, bool erase, const Layout& layout);
        UpdateReturnStatus updateHub(const bool* keys, float mouseX, float mouseY,
            bool leftPressed, const Layout& layout);
        UpdateReturnStatus updateGallery(const bool* keys, float mouseX, float mouseY,
            bool leftPressed, const Layout& layout);
        void updateEditor(const bool* keys, float mouseX, float mouseY,
            bool leftPressed, bool leftDown, bool rightDown, const Layout& layout);

        CharacterStore _store;
        std::unique_ptr<renderer::CharacterEditorRenderer> _renderer;
        State _state;
        std::array<bool, SDL_SCANCODE_COUNT> _previousKeys{};
        std::uint32_t _previousMouseButtons = 0;
    };

} // namespace charactereditor
