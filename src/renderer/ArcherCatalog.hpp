#pragma once
#include "SpriteCatalog.hpp"
#include <functional>
#include <string>
#include <vector>



namespace renderer
{

    struct ArcherSkin
    {
        std::string name;
        std::string bodySprite;
        std::string headBackSprite;
        std::string headNormalSprite;
        std::string headNoHatSprite;
        std::string headCrownSprite;
        std::string bowSprite;
        bool startNoHat = false;
        bool renderable = false;
    };

    class ArcherCatalog
    {
    public:
        using TextureExists = std::function<bool(const std::string&)>;

        ArcherCatalog() = default;
        ArcherCatalog(const std::string& xmlPath, const SpriteCatalog& sprites, TextureExists textureExists);

        void load(const std::string& xmlPath, const SpriteCatalog& sprites, TextureExists textureExists);

        const std::vector<ArcherSkin>& baseSkins() const;
        const ArcherSkin& skinForCharacter(unsigned int character) const;
        std::size_t validBaseCount() const;
        bool isBaseSkinRenderable(std::size_t index) const;

        static unsigned int wrapCharacter(unsigned int character, std::size_t validCount);
        static unsigned int offsetCharacter(unsigned int character, int offset, std::size_t validCount);
        static std::size_t defaultValidBaseCount();

    private:
        std::vector<ArcherSkin> _baseSkins;
        std::vector<std::size_t> _validBaseSkinIndexes;
    };

};  // end namespace renderer
