#include "charactereditor/CharacterStore.hpp"
#include <gtest/gtest.h>
#include <unordered_set>

TEST(CharacterStoreTest, StarterCoversTheCompleteAnimationSchema)
{
    charactereditor::CharacterStore store;
    const charactereditor::CustomCharacter character = store.makeCharacter("Schema Test Archer");

    ASSERT_EQ(character.canvases.size(), 3u);
    const std::unordered_set<std::string> required = {"bow", "body", "head"};
    for (const charactereditor::Canvas& canvas : character.canvases)
    {
        EXPECT_TRUE(required.contains(canvas.id));
        EXPECT_EQ(canvas.frameCount, charactereditor::POSE_FRAME_COUNT);
        EXPECT_EQ(canvas.pixels.size(), static_cast<std::size_t>(
            charactereditor::FRAME_WIDTH * charactereditor::FRAME_HEIGHT
            * charactereditor::POSE_FRAME_COUNT));
    }

    int expectedFirstFrame = 0;
    for (const charactereditor::AnimationSpec& animation : charactereditor::ANIMATIONS)
    {
        EXPECT_EQ(animation.firstFrame, expectedFirstFrame);
        EXPECT_GT(animation.frameCount, 0);
        expectedFirstFrame += animation.frameCount;
    }
    EXPECT_EQ(expectedFirstFrame, charactereditor::POSE_FRAME_COUNT);
}

TEST(CharacterStoreTest, IncludesDashAndFullBowAtlasRanges)
{
    const auto* dodge = charactereditor::animationSpec("dodge");
    const auto* slide = charactereditor::animationSpec("slide");
    const auto* bowDraw = charactereditor::animationSpec("bow_draw");
    const auto* bowEmpty = charactereditor::animationSpec("bow_empty");

    ASSERT_NE(dodge, nullptr);
    ASSERT_NE(slide, nullptr);
    ASSERT_NE(bowDraw, nullptr);
    ASSERT_NE(bowEmpty, nullptr);
    EXPECT_EQ(dodge->frameCount, 3);
    EXPECT_EQ(slide->frameCount, 8);
    EXPECT_EQ(bowDraw->frameCount, 48);
    EXPECT_EQ(bowEmpty->frameCount, 4);
}
