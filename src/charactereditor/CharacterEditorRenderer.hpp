#pragma once

#include "../renderer/ArcherCatalog.hpp"
#include "../renderer/SpriteCatalog.hpp"
#include "../renderer/TextureAtlas.hpp"
#include "../renderer/renderer.hpp"
#include "scene.hpp"
#include <string>
#include <vector>

namespace renderer
{

    class CharacterEditorRenderer : public _Renderer<charactereditor::State>
    {
    public:
        CharacterEditorRenderer(SDL_Window* window, SDL_Renderer* renderer);
        ~CharacterEditorRenderer() override;

        void pushState(State state) override;
        void render() override;

        const std::vector<charactereditor::BuiltInCharacter>& builtInCharacters() const;
        charactereditor::Layout layout(const charactereditor::State& state) const;

    private:
        void renderHub(const charactereditor::Layout& ui, float mouseX, float mouseY);
        void renderGallery(const charactereditor::Layout& ui, float mouseX, float mouseY);
        void renderEditor(const charactereditor::Layout& ui, float mouseX, float mouseY);
        void drawBuiltIn(const charactereditor::BuiltInCharacter& character, const SDL_FRect& bounds);
        void drawCustomFrame(const charactereditor::Canvas& canvas, std::size_t frame,
            const SDL_FRect& bounds, bool drawGrid = false);
        void drawCustomCharacter(const charactereditor::CustomCharacter& character,
            std::size_t frame, const SDL_FRect& bounds);
        void drawButton(const SDL_FRect& bounds, const std::string& label, bool active,
            float mouseX, float mouseY);
        void drawText(const std::string& text, float x, float y, bool centered = false) const;

        TextureAtlas _atlas;
        SpriteCatalog _sprites;
        ArcherCatalog _archers;
        std::vector<charactereditor::BuiltInCharacter> _builtIns;
        State _state;
    };

} // namespace renderer
