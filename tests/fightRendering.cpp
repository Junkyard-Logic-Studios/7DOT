#include <gtest/gtest.h>
#include "renderer/FightTilemapCache.hpp"
#include "renderer/StageMapView.hpp"
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

    void expectPoint(const SDL_FPoint& point, float x, float y)
    {
        EXPECT_NEAR(point.x, x, EPSILON);
        EXPECT_NEAR(point.y, y, EPSILON);
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


TEST(StageMapViewTest, UsesUniformCoverScaleInsideStageViewport)
{
    renderer::ViewportLayout layout = renderer::computeViewportLayout(1600, 900, 320, 240);
    renderer::StageMapView view = renderer::computeStageMapView(layout.mapRect, {240.0f, 240.0f});

    EXPECT_FLOAT_EQ(view.scale, 2.5f);
    expectRect(view.mapRect, 200.0f, -150.0f, 1200.0f, 1200.0f);
}


TEST(StageMapViewTest, AppliesAdditionalZoomForSelectionMapCamera)
{
    renderer::ViewportLayout layout = renderer::computeViewportLayout(960, 720, 320, 240);
    renderer::StageMapView view = renderer::computeStageMapView(layout.mapRect, {425.0f, 300.0f}, 1.5f);

    EXPECT_FLOAT_EQ(view.scale, 3.0f);
    expectRect(view.mapRect, -480.0f, -540.0f, 1440.0f, 1440.0f);
}


TEST(StageMapViewTest, KeepsCameraTargetAtViewportCenterWhenUnclampedAfterResize)
{
    SDL_FPoint cameraCenter {240.0f, 240.0f};
    renderer::ViewportLayout smallLayout = renderer::computeViewportLayout(960, 720, 320, 240);
    renderer::ViewportLayout largeLayout = renderer::computeViewportLayout(1280, 960, 320, 240);

    renderer::StageMapView smallView = renderer::computeStageMapView(smallLayout.mapRect, cameraCenter);
    renderer::StageMapView largeView = renderer::computeStageMapView(largeLayout.mapRect, cameraCenter);

    expectPoint(smallView.worldToScreen(cameraCenter), 480.0f, 360.0f);
    expectPoint(largeView.worldToScreen(cameraCenter), 640.0f, 480.0f);
}


TEST(StageMapViewTest, ClampsEdgeCameraWithoutExposingEmptySpace)
{
    renderer::ViewportLayout layout = renderer::computeViewportLayout(960, 720, 320, 240);
    renderer::StageMapView view = renderer::computeStageMapView(layout.mapRect, {37.0f, 155.0f});

    EXPECT_LE(view.mapRect.x, layout.mapRect.x);
    EXPECT_LE(view.mapRect.y, layout.mapRect.y);
    EXPECT_GE(view.mapRect.x + view.mapRect.w, layout.mapRect.x + layout.mapRect.w);
    EXPECT_GE(view.mapRect.y + view.mapRect.h, layout.mapRect.y + layout.mapRect.h);
}


TEST(StageMapViewTest, AppliesSameTransformToPointsAndRects)
{
    SDL_FRect viewport {100.0f, 50.0f, 960.0f, 720.0f};
    renderer::StageMapView view = renderer::computeStageMapView(viewport, {240.0f, 240.0f});
    SDL_FPoint point = view.worldToScreen({210.0f, 210.0f});
    SDL_FRect rect = view.worldRectToScreen(210.0f, 210.0f, 60.0f, 60.0f);

    expectPoint(point, rect.x, rect.y);
    EXPECT_FLOAT_EQ(rect.w, 120.0f);
    EXPECT_FLOAT_EQ(rect.h, 120.0f);
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
