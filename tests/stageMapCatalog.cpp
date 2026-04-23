#include <gtest/gtest.h>
#include "constants.hpp"
#include "renderer/StageMapCatalog.hpp"
#include <array>



namespace
{

    struct ExpectedStageMapEntry
    {
        fight::Stage stage;
        const char* iconAtlasName;
        float x;
        float y;
    };


    constexpr std::array<ExpectedStageMapEntry, 12> EXPECTED_ENTRIES = {{
        {fight::Stage::SACRED_GROUND,  "towerIcons/sacredGround",  71.0f, 114.0f},
        {fight::Stage::TWILIGHT_SPIRE, "towerIcons/twilightSpire", 140.0f, 174.0f},
        {fight::Stage::BACKFIRE,       "towerIcons/backfire",      216.0f, 98.0f},
        {fight::Stage::FLIGHT,         "towerIcons/flight",        308.0f, 90.0f},
        {fight::Stage::MIRAGE,         "towerIcons/mirage",        425.0f, 176.0f},
        {fight::Stage::THORNWOOD,      "towerIcons/thornwood",     308.0f, 138.0f},
        {fight::Stage::FROSTFANG_KEEP, "towerIcons/frostfangKeep", 244.0f, 66.0f},
        {fight::Stage::KINGS_COURT,    "towerIcons/kingsCourt",    128.0f, 94.0f},
        {fight::Stage::SUNKEN_CITY,    "towerIcons/sunkenCity",    248.0f, 181.0f},
        {fight::Stage::MOONSTONE,      "towerIcons/moonstone",     370.0f, 146.0f},
        {fight::Stage::TOWERFORGE,     "towerIcons/towerForge",    256.0f, 338.0f},
        {fight::Stage::ASCENSION,      "towerIcons/ascension",     413.0f, 300.0f},
    }};


    renderer::StageMapCatalog loadCatalog()
    {
        renderer::StageMapCatalog catalog;
        catalog.load(ASSET_DIR "Atlas/GameData/themeData.xml");
        return catalog;
    }

}


TEST(StageMapCatalogTest, LoadsAllSelectableStagesInEnumOrder)
{
    renderer::StageMapCatalog catalog = loadCatalog();

    ASSERT_EQ(catalog.entries().size(), EXPECTED_ENTRIES.size());
    for (std::size_t i = 0; i < EXPECTED_ENTRIES.size(); i++)
        EXPECT_EQ(catalog.entries()[i].stage, EXPECTED_ENTRIES[i].stage);
}


TEST(StageMapCatalogTest, ReadsExpectedIcons)
{
    renderer::StageMapCatalog catalog = loadCatalog();

    EXPECT_EQ(catalog.find(fight::Stage::SACRED_GROUND)->iconAtlasName, "towerIcons/sacredGround");
    EXPECT_EQ(catalog.find(fight::Stage::FROSTFANG_KEEP)->iconAtlasName, "towerIcons/frostfangKeep");
    EXPECT_EQ(catalog.find(fight::Stage::TOWERFORGE)->iconAtlasName, "towerIcons/towerForge");
    EXPECT_EQ(catalog.find(fight::Stage::ASCENSION)->iconAtlasName, "towerIcons/ascension");
}


TEST(StageMapCatalogTest, ReadsExpectedPositions)
{
    renderer::StageMapCatalog catalog = loadCatalog();

    for (const auto& expected : EXPECTED_ENTRIES)
    {
        const auto* entry = catalog.find(expected.stage);
        ASSERT_NE(entry, nullptr);
        EXPECT_FLOAT_EQ(entry->mapPosition.x, expected.x);
        EXPECT_FLOAT_EQ(entry->mapPosition.y, expected.y);
    }
}


TEST(StageMapCatalogTest, FindReturnsNullForMaxEnum)
{
    renderer::StageMapCatalog catalog = loadCatalog();

    EXPECT_EQ(catalog.find(fight::Stage::MAX_ENUM), nullptr);
}


TEST(StageMapCatalogTest, AllSelectableStagesHaveEntries)
{
    renderer::StageMapCatalog catalog = loadCatalog();

    for (int i = 0; i < static_cast<int>(fight::Stage::MAX_ENUM); i++)
    {
        auto stage = static_cast<fight::Stage>(i);
        EXPECT_NE(catalog.find(stage), nullptr) << fight::stageToName(stage);
    }
}
