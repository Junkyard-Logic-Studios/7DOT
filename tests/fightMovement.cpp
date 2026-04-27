#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include "constants.hpp"
#include "fight/level.hpp"
#include "fight/movement.hpp"
using namespace fight;



namespace
{
    constexpr float EPSILON = 0.001f;
    constexpr float START_X = 100.0f;
    constexpr float START_Y = 100.0f;

    std::string emptyBitmap(std::size_t width, std::size_t height)
    {
        std::string bitmap;
        for (std::size_t y = 0; y < height; y++)
        {
            bitmap.append(width, '0');
            if (y + 1 < height)
                bitmap.push_back('\n');
        }
        return bitmap;
    }

    std::string ledgeBitmap(std::size_t width, std::size_t height)
    {
        std::string bitmap;
        for (std::size_t y = 0; y < height; y++)
        {
            for (std::size_t x = 0; x < width; x++)
                bitmap.push_back(x == 5 && y >= 10 ? '1' : '0');
            if (y + 1 < height)
                bitmap.push_back('\n');
        }
        return bitmap;
    }

    std::filesystem::path emptyLevelPath()
    {
        constexpr std::size_t width = 80;
        constexpr std::size_t height = 80;
        auto path = std::filesystem::temp_directory_path() / "7dot_empty_movement.oel";

        std::ofstream out(path);
        out << "<level width=\"" << width * TILESIZE
            << "\" height=\"" << height * TILESIZE
            << "\" WrapMode=\"None\">\n"
            << "  <BG exportMode=\"Bitstring\">" << emptyBitmap(width, height) << "</BG>\n"
            << "  <BGTiles tileset=\"CathedralBG\" exportMode=\"TrimmedCSV\"></BGTiles>\n"
            << "  <Solids exportMode=\"Bitstring\">" << emptyBitmap(width, height) << "</Solids>\n"
            << "  <SolidTiles tileset=\"CathedralBG\" exportMode=\"TrimmedCSV\"></SolidTiles>\n"
            << "  <Entities><PlayerSpawn id=\"0\" x=\"" << START_X
            << "\" y=\"" << START_Y << "\" /></Entities>\n"
            << "</level>\n";

        return path;
    }

    std::filesystem::path ledgeLevelPath()
    {
        constexpr std::size_t width = 20;
        constexpr std::size_t height = 20;
        auto path = std::filesystem::temp_directory_path() / "7dot_ledge_movement.oel";
        std::string empty = emptyBitmap(width, height);
        std::string ledge = ledgeBitmap(width, height);

        std::ofstream out(path);
        out << "<level width=\"" << width * TILESIZE
            << "\" height=\"" << height * TILESIZE
            << "\" WrapMode=\"None\">\n"
            << "  <BG exportMode=\"Bitstring\">" << empty << "</BG>\n"
            << "  <BGTiles tileset=\"CathedralBG\" exportMode=\"TrimmedCSV\"></BGTiles>\n"
            << "  <Solids exportMode=\"Bitstring\">" << ledge << "</Solids>\n"
            << "  <SolidTiles tileset=\"CathedralBG\" exportMode=\"TrimmedCSV\"></SolidTiles>\n"
            << "  <Entities><PlayerSpawn id=\"0\" x=\"" << START_X
            << "\" y=\"" << START_Y << "\" /></Entities>\n"
            << "</level>\n";

        return path;
    }

    input::PlayerInput rightInput()
    {
        input::PlayerInput input = 0;
        input::set::horizontalAxis(input, std::numeric_limits<int16_t>::max());
        return input;
    }

    input::PlayerInput neutralInput()
    {
        return 0;
    }

    input::PlayerInput jumpPressed()
    {
        input::PlayerInput input = 0;
        input::set::jump(input, true);
        return input;
    }

    Archer testArcher()
    {
        Archer archer {};
        archer.position = {START_X, START_Y};
        archer.velocity = glm::vec2(0.0f);
        archer.isAlive = true;
        archer.isCrouching = false;
        archer.isFacingRight = true;
        return archer;
    }

    void step(Level& level, Archer& archer, float deltaTime, int count)
    {
        auto input = rightInput();
        for (int i = 0; i < count; i++)
            movement::stepArcher(level, archer, input, 0, 0, deltaTime);
    }

    float oneSecondSteadyRunDistance(Level& level, float deltaTime, int ticksPerSecond)
    {
        Archer archer = testArcher();
        step(level, archer, deltaTime, ticksPerSecond);

        // Measure after warm-up so the assertion checks distance scaling, not Euler acceleration error.
        float startX = archer.position.x;
        step(level, archer, deltaTime, ticksPerSecond);
        return archer.position.x - startX;
    }
}


TEST(FightMovementTest, OneSecondRunDistanceIsStableAcrossPhysicsTickRates)
{
    auto path = emptyLevelPath();
    Level level(Stage::SACRED_GROUND, path.c_str());

    float distance100Hz = oneSecondSteadyRunDistance(level, 0.01f, 100);
    float distance10Hz = oneSecondSteadyRunDistance(level, 0.1f, 10);

    EXPECT_GT(distance100Hz, 100.0f);
    EXPECT_NEAR(distance100Hz, distance10Hz, 0.01f);

    std::filesystem::remove(path);
}


TEST(FightMovementTest, WallSlideGrabsCornerUntilWallDirectionReleased)
{
    auto path = ledgeLevelPath();
    Level level(Stage::SACRED_GROUND, path.c_str());
    Archer archer = testArcher();
    archer.position = {44.99f, 101.99f};
    archer.velocity = {20.0f, 0.0f};
    archer.movementState = ArcherMovementState::WALL_SLIDING;

    movement::stepArcher(level, archer, rightInput(), 0, 0, 0.001f);

    EXPECT_EQ(ArcherMovementState::LEDGE_CLINGING, archer.movementState);
    EXPECT_EQ(1, archer.wallGrabDirection);
    EXPECT_EQ(glm::vec2(0.0f), archer.velocity);
    EXPECT_NEAR(10.0f * TILESIZE, archer.headHitbox().br.y, EPSILON);
    glm::vec2 clingingPosition = archer.position;

    movement::stepArcher(level, archer, rightInput(), 0, 0, 0.1f);

    EXPECT_EQ(ArcherMovementState::LEDGE_CLINGING, archer.movementState);
    EXPECT_EQ(clingingPosition, archer.position);
    EXPECT_EQ(glm::vec2(0.0f), archer.velocity);

    movement::stepArcher(level, archer, neutralInput(), 0, 0, 0.01f);

    EXPECT_EQ(ArcherMovementState::AIRBORNE, archer.movementState);
    EXPECT_GT(archer.velocity.y, 0.0f);

    std::filesystem::remove(path);
}


TEST(FightMovementTest, LedgeClingJumpsInHeldDirection)
{
    auto path = ledgeLevelPath();
    Level level(Stage::SACRED_GROUND, path.c_str());
    Archer archer = testArcher();
    archer.position = {44.99f, 101.99f};
    archer.velocity = {20.0f, 0.0f};
    archer.movementState = ArcherMovementState::WALL_SLIDING;

    movement::stepArcher(level, archer, rightInput(), 0, 0, 0.001f);
    ASSERT_EQ(ArcherMovementState::LEDGE_CLINGING, archer.movementState);

    movement::stepArcher(level, archer, rightInput(), jumpPressed(), 0, 0.001f);

    EXPECT_EQ(ArcherMovementState::AIRBORNE, archer.movementState);
    EXPECT_GT(archer.velocity.x, 0.0f);
    EXPECT_LT(archer.velocity.y, 0.0f);
    EXPECT_EQ(1, archer.autoMoveDirection);
    EXPECT_GT(archer.autoMoveTime, 0.0f);
    EXPECT_EQ(0, archer.wallGrabDirection);

    std::filesystem::remove(path);
}


TEST(FightMovementTest, WallSlideDoesNotGrabBelowCornerTile)
{
    auto path = ledgeLevelPath();
    Level level(Stage::SACRED_GROUND, path.c_str());
    Archer archer = testArcher();
    archer.position = {44.99f, 112.0f};
    archer.velocity = {20.0f, 0.0f};
    archer.movementState = ArcherMovementState::WALL_SLIDING;

    movement::stepArcher(level, archer, rightInput(), 0, 0, 0.001f);

    EXPECT_EQ(ArcherMovementState::WALL_SLIDING, archer.movementState);
    EXPECT_EQ(0, archer.wallGrabDirection);

    std::filesystem::remove(path);
}
