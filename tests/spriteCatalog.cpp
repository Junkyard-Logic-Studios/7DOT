#include <gtest/gtest.h>
#include "constants.hpp"
#include "renderer/SpriteCatalog.hpp"



TEST(SpriteCatalogTest, LoadsPlayerBodyMetadata)
{
    renderer::SpriteCatalog catalog(ASSET_DIR "Atlas/SpriteData/spriteData.xml");

    const auto& body = catalog.get("PlayerBody0");
    EXPECT_EQ(body.texture, "player/body0");
    EXPECT_EQ(body.frameWidth, 12);
    EXPECT_EQ(body.frameHeight, 20);

    const auto* stand = body.animation("stand");
    ASSERT_NE(stand, nullptr);
    ASSERT_EQ(stand->frames.size(), 1);
    EXPECT_EQ(stand->frames[0], 0);

    const auto* run = body.animation("run");
    ASSERT_NE(run, nullptr);
    ASSERT_GE(run->frames.size(), 5);
    EXPECT_EQ(run->frames[0], 2);
}


TEST(SpriteCatalogTest, LoadsHeadOrigins)
{
    renderer::SpriteCatalog catalog(ASSET_DIR "Atlas/SpriteData/spriteData.xml");

    const auto& body = catalog.get("Green_Alt");
    ASSERT_FALSE(body.headXOrigins.empty());
    ASSERT_FALSE(body.headYOrigins.empty());
    EXPECT_EQ(body.headXOrigins[0], 5);
    EXPECT_EQ(body.headYOrigins[0], 19);
}


TEST(SpriteCatalogTest, LoadsMenuMapAnimations)
{
    renderer::SpriteCatalog catalog(ASSET_DIR "Atlas/SpriteData/menuSpriteData.xml");

    EXPECT_NE(catalog.get("boat").animation("idle"), nullptr);
    EXPECT_NE(catalog.get("twilightSpire").animation("notSelected"), nullptr);
    EXPECT_NE(catalog.get("twilightSpire").animation("selected"), nullptr);
    EXPECT_NE(catalog.get("sunkenCityMap").animation("idleUp"), nullptr);
    EXPECT_NE(catalog.get("towerForgeMap").animation("unlockSelected"), nullptr);
    EXPECT_NE(catalog.get("ascensionUnlock").animation("unlockSelected"), nullptr);
    EXPECT_NE(catalog.get("ghostShipMap").animation("up"), nullptr);
}
