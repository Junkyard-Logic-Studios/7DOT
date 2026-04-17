#include <gtest/gtest.h>
#include "constants.hpp"
#include "pugixml.hpp"
#include "renderer/ArcherCatalog.hpp"
#include "renderer/SpriteCatalog.hpp"
#include <stdexcept>
#include <string>
#include <unordered_set>



namespace
{

    std::unordered_set<std::string> loadAtlasTextureNames()
    {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(ASSET_DIR "Atlas/atlas.xml");
        if (!result)
            throw std::runtime_error("Failed to parse atlas.xml");

        std::unordered_set<std::string> names;
        for (auto tex : doc.child("TextureAtlas").children("SubTexture"))
            names.insert(tex.attribute("name").as_string());

        return names;
    }

    renderer::ArcherCatalog loadArcherCatalog(
        const renderer::SpriteCatalog& sprites,
        const std::unordered_set<std::string>& atlasNames
    ) {
        return renderer::ArcherCatalog(
            ASSET_DIR "Atlas/GameData/archerData.xml",
            sprites,
            [&](const std::string& texture) { return atlasNames.count(texture) > 0; });
    }

    bool requiredSpriteTextureExists(
        const renderer::SpriteCatalog& sprites,
        const std::unordered_set<std::string>& atlasNames,
        const std::string& spriteId
    ) {
        const auto* sprite = sprites.find(spriteId);
        return sprite && !sprite->texture.empty() && atlasNames.count(sprite->texture) > 0;
    }

}


TEST(ArcherCatalogTest, MarksRenderableBaseSkins)
{
    renderer::SpriteCatalog sprites(ASSET_DIR "Atlas/SpriteData/spriteData.xml");
    auto atlasNames = loadAtlasTextureNames();
    renderer::ArcherCatalog archers = loadArcherCatalog(sprites, atlasNames);

    ASSERT_GE(archers.baseSkins().size(), 9);
    EXPECT_EQ(archers.validBaseCount(), 8);

    for (std::size_t i = 0; i < 8; i++)
        EXPECT_TRUE(archers.isBaseSkinRenderable(i)) << "base archer " << i;

    EXPECT_FALSE(archers.isBaseSkinRenderable(8));
}


TEST(ArcherCatalogTest, WrapsCharacterSelectionWithoutUnderflow)
{
    EXPECT_EQ(renderer::ArcherCatalog::offsetCharacter(0, -1, 8), 7);
    EXPECT_EQ(renderer::ArcherCatalog::offsetCharacter(7, 1, 8), 0);
    EXPECT_EQ(renderer::ArcherCatalog::offsetCharacter(9, -1, 8), 0);
    EXPECT_EQ(renderer::ArcherCatalog::offsetCharacter(0, -1, 0), 0);
}


TEST(ArcherCatalogTest, RenderableSkinsHaveRequiredAtlasTextures)
{
    renderer::SpriteCatalog sprites(ASSET_DIR "Atlas/SpriteData/spriteData.xml");
    auto atlasNames = loadAtlasTextureNames();
    renderer::ArcherCatalog archers = loadArcherCatalog(sprites, atlasNames);

    for (const auto& skin : archers.baseSkins())
    {
        if (!skin.renderable)
            continue;

        EXPECT_TRUE(requiredSpriteTextureExists(sprites, atlasNames, skin.bodySprite)) << skin.bodySprite;
        EXPECT_TRUE(requiredSpriteTextureExists(sprites, atlasNames, skin.headNormalSprite)) << skin.headNormalSprite;
        EXPECT_TRUE(requiredSpriteTextureExists(sprites, atlasNames, skin.bowSprite)) << skin.bowSprite;
        if (!skin.headBackSprite.empty())
            EXPECT_TRUE(requiredSpriteTextureExists(sprites, atlasNames, skin.headBackSprite)) << skin.headBackSprite;
    }
}
