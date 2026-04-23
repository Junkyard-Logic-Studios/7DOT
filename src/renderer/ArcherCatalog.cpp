#include "ArcherCatalog.hpp"
#include "../constants.hpp"
#include "pugixml.hpp"
#include <stdexcept>
#include <unordered_set>



namespace
{

    std::string childText(const pugi::xml_node& node, const char* childName)
    {
        return node.child(childName).text().as_string();
    }

    bool parseBool(const char* value)
    {
        return std::string(value) == "True" || std::string(value) == "true" || std::string(value) == "1";
    }

    std::unordered_set<std::string> loadAtlasTextureNames(const std::string& xmlPath)
    {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(xmlPath.c_str());
        if (!result)
            throw std::runtime_error("Failed to parse file: " + xmlPath);

        std::unordered_set<std::string> names;
        for (auto tex : doc.child("TextureAtlas").children("SubTexture"))
            names.insert(tex.attribute("name").as_string());

        return names;
    }

    bool spriteTextureExists(
        const renderer::SpriteCatalog& sprites,
        const std::string& spriteId,
        const renderer::ArcherCatalog::TextureExists& textureExists
    ) {
        const auto* sprite = sprites.find(spriteId);
        return sprite && !sprite->texture.empty() && (!textureExists || textureExists(sprite->texture));
    }

    bool isRenderable(
        const renderer::ArcherSkin& skin,
        const renderer::SpriteCatalog& sprites,
        const renderer::ArcherCatalog::TextureExists& textureExists
    ) {
        if (!spriteTextureExists(sprites, skin.bodySprite, textureExists))
            return false;
        if (!spriteTextureExists(sprites, skin.headNormalSprite, textureExists))
            return false;
        if (!spriteTextureExists(sprites, skin.bowSprite, textureExists))
            return false;
        if (!skin.headBackSprite.empty() && !spriteTextureExists(sprites, skin.headBackSprite, textureExists))
            return false;

        return true;
    }

}


renderer::ArcherCatalog::ArcherCatalog(
    const std::string& xmlPath,
    const SpriteCatalog& sprites,
    TextureExists textureExists
) {
    load(xmlPath, sprites, textureExists);
}


void renderer::ArcherCatalog::load(
    const std::string& xmlPath,
    const SpriteCatalog& sprites,
    TextureExists textureExists
) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(xmlPath.c_str());
    if (!result)
        throw std::runtime_error("Failed to parse file: " + xmlPath);

    _baseSkins.clear();
    _validBaseSkinIndexes.clear();

    for (auto archerNode : doc.child("Archers").children("Archer"))
    {
        ArcherSkin skin;
        std::string name0 = childText(archerNode, "Name0");
        std::string name1 = childText(archerNode, "Name1");
        skin.name = name0.empty() ? name1 : (name1.empty() ? name0 : name0 + " " + name1);
        skin.startNoHat = parseBool(archerNode.child("StartNoHat").text().as_string());

        auto spritesNode = archerNode.child("Sprites");
        skin.bodySprite = childText(spritesNode, "Body");
        skin.headBackSprite = childText(spritesNode, "HeadBack");
        skin.headNormalSprite = childText(spritesNode, "HeadNormal");
        skin.headNoHatSprite = childText(spritesNode, "HeadNoHat");
        skin.headCrownSprite = childText(spritesNode, "HeadCrown");
        skin.bowSprite = childText(spritesNode, "Bow");
        skin.renderable = isRenderable(skin, sprites, textureExists);

        if (skin.renderable)
            _validBaseSkinIndexes.push_back(_baseSkins.size());
        _baseSkins.push_back(skin);
    }
}


const std::vector<renderer::ArcherSkin>& renderer::ArcherCatalog::baseSkins() const
{
    return _baseSkins;
}


const renderer::ArcherSkin& renderer::ArcherCatalog::skinForCharacter(unsigned int character) const
{
    if (_validBaseSkinIndexes.empty())
    {
        if (_baseSkins.empty())
            throw std::runtime_error("No archer skins loaded");
        return _baseSkins.front();
    }

    unsigned int wrapped = wrapCharacter(character, _validBaseSkinIndexes.size());
    return _baseSkins.at(_validBaseSkinIndexes.at(wrapped));
}


std::size_t renderer::ArcherCatalog::validBaseCount() const
{
    return _validBaseSkinIndexes.size();
}


bool renderer::ArcherCatalog::isBaseSkinRenderable(std::size_t index) const
{
    return index < _baseSkins.size() && _baseSkins[index].renderable;
}


unsigned int renderer::ArcherCatalog::wrapCharacter(unsigned int character, std::size_t validCount)
{
    return validCount == 0 ? 0 : character % validCount;
}


unsigned int renderer::ArcherCatalog::offsetCharacter(unsigned int character, int offset, std::size_t validCount)
{
    if (validCount == 0)
        return 0;

    int count = static_cast<int>(validCount);
    int wrapped = static_cast<int>(character % validCount);
    return static_cast<unsigned int>((wrapped + offset % count + count) % count);
}


std::size_t renderer::ArcherCatalog::defaultValidBaseCount()
{
    static std::size_t count = [] {
        SpriteCatalog sprites(ASSET_DIR "Atlas/SpriteData/spriteData.xml");
        auto atlasNames = loadAtlasTextureNames(ASSET_DIR "Atlas/atlas.xml");
        ArcherCatalog archers(
            ASSET_DIR "Atlas/GameData/archerData.xml",
            sprites,
            [&](const std::string& texture) { return atlasNames.count(texture) > 0; });
        return archers.validBaseCount();
    }();

    return count;
}
