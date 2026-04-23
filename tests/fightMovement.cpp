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

    input::PlayerInput rightInput()
    {
        input::PlayerInput input = 0;
        input::set::horizontalAxis(input, std::numeric_limits<int16_t>::max());
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
