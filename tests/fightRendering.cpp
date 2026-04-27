#include <gtest/gtest.h>
#include "renderer/FightTilemapCache.hpp"
#include "renderer/ViewportLayout.hpp"



namespace
{

    constexpr float EPSILON = 0.001f;

    void expectRect(const SDL_FRect& rect, float x, float y, float w, float h)
    {
        EXPECT_NEAR(rect.x, x, EPSILON);
        EXPECT_NEAR(rect.y, y, EPSILON);
        EXPECT_NEAR(rect.w, w, EPSILON);
        EXPECT_NEAR(rect.h, h, EPSILON);
    }

}


TEST(ViewportLayoutTest, ExactFourByThreeWindowUsesFullWindow)
{
    renderer::ViewportLayout layout = renderer::computeViewportLayout(960, 720, 320, 240);

    EXPECT_FLOAT_EQ(layout.scale, 3.0f);
    expectRect(layout.mapRect, 0.0f, 0.0f, 960.0f, 720.0f);
    expectRect(layout.deadSpace[0], 0.0f, 0.0f, 0.0f, 720.0f);
    expectRect(layout.deadSpace[1], 960.0f, 0.0f, 0.0f, 720.0f);
    expectRect(layout.deadSpace[2], 0.0f, 0.0f, 960.0f, 0.0f);
    expectRect(layout.deadSpace[3], 0.0f, 720.0f, 960.0f, 0.0f);
}


TEST(ViewportLayoutTest, WideWindowCentersMapAndReportsSideDeadSpace)
{
    renderer::ViewportLayout layout = renderer::computeViewportLayout(1280, 720, 320, 240);

    EXPECT_FLOAT_EQ(layout.scale, 3.0f);
    expectRect(layout.mapRect, 160.0f, 0.0f, 960.0f, 720.0f);
    expectRect(layout.deadSpace[0], 0.0f, 0.0f, 160.0f, 720.0f);
    expectRect(layout.deadSpace[1], 1120.0f, 0.0f, 160.0f, 720.0f);
    expectRect(layout.deadSpace[2], 160.0f, 0.0f, 960.0f, 0.0f);
    expectRect(layout.deadSpace[3], 160.0f, 720.0f, 960.0f, 0.0f);
}


TEST(ViewportLayoutTest, TallWindowCentersMapAndReportsTopBottomDeadSpace)
{
    renderer::ViewportLayout layout = renderer::computeViewportLayout(720, 960, 320, 240);

    EXPECT_FLOAT_EQ(layout.scale, 2.25f);
    expectRect(layout.mapRect, 0.0f, 210.0f, 720.0f, 540.0f);
    expectRect(layout.deadSpace[0], 0.0f, 0.0f, 0.0f, 960.0f);
    expectRect(layout.deadSpace[1], 720.0f, 0.0f, 0.0f, 960.0f);
    expectRect(layout.deadSpace[2], 0.0f, 0.0f, 720.0f, 210.0f);
    expectRect(layout.deadSpace[3], 0.0f, 750.0f, 720.0f, 210.0f);
}


TEST(ViewportLayoutTest, OddWindowSizesStayCenteredWithoutNegativeDeadSpace)
{
    renderer::ViewportLayout layout = renderer::computeViewportLayout(1111, 777, 320, 240);

    EXPECT_NEAR(layout.scale, 3.2375f, EPSILON);
    expectRect(layout.mapRect, 37.5f, 0.0f, 1036.0f, 777.0f);

    for (const auto& rect : layout.deadSpace)
    {
        EXPECT_GE(rect.w, 0.0f);
        EXPECT_GE(rect.h, 0.0f);
    }
}


TEST(FightTilemapCacheTest, SameKeyDoesNotRequestRebuildAfterMarkingValid)
{
    renderer::FightTilemapCacheState cache;
    renderer::FightTilemapCacheKey key {
        fight::Stage::SACRED_GROUND,
        0,
        32,
        24,
        "sacredGround"
    };

    EXPECT_TRUE(cache.shouldRebuild(key));
    cache.markValid(key);
    EXPECT_FALSE(cache.shouldRebuild(key));
}


TEST(FightTilemapCacheTest, ChangedLevelIndexOrSizeRequestsRebuild)
{
    renderer::FightTilemapCacheState cache;
    renderer::FightTilemapCacheKey key {
        fight::Stage::SACRED_GROUND,
        0,
        32,
        24,
        "sacredGround"
    };
    cache.markValid(key);

    renderer::FightTilemapCacheKey nextLevel = key;
    nextLevel.levelIndex = 1;
    EXPECT_TRUE(cache.shouldRebuild(nextLevel));

    renderer::FightTilemapCacheKey resized = key;
    resized.width = 40;
    EXPECT_TRUE(cache.shouldRebuild(resized));
}
