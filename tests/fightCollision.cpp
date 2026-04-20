#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "constants.hpp"
#include "fight/collision.hpp"
#include "fight/level.hpp"
using namespace fight;



namespace
{
    constexpr float EPSILON = 0.001f;

    Level sacredGround()
    {
        return Level(Stage::SACRED_GROUND, ASSET_DIR "Levels/00 - Sacred Ground/00.oel");
    }

    std::filesystem::path sacredGroundWithWrapMode(const std::string& wrapMode)
    {
        std::ifstream source(ASSET_DIR "Levels/00 - Sacred Ground/00.oel");
        std::string xml{
            std::istreambuf_iterator<char>(source),
            std::istreambuf_iterator<char>()};

        std::string from = "WrapMode=\"Both\"";
        std::string to = "WrapMode=\"" + wrapMode + "\"";
        xml.replace(xml.find(from), from.size(), to);

        auto path = std::filesystem::temp_directory_path() / ("7dot_wrap_" + wrapMode + ".oel");
        std::ofstream out(path);
        out << xml;
        return path;
    }
}


TEST(FightCollisionTest, StopsFlushAgainstRightWall)
{
    auto level = sacredGround();
    Archer archer {};
    archer.position = {270.0f, 45.0f};
    archer.velocity = {30.0f, 0.0f};

    auto contacts = collision::moveAndCollide(level, archer, 1.0f);

    EXPECT_NEAR(30.0f * TILESIZE, archer.hitboxBR().x, EPSILON);
    EXPECT_FLOAT_EQ(0.0f, archer.velocity.x);
    EXPECT_TRUE(contacts.rightWall);
}


TEST(FightCollisionTest, StopsFlushAgainstCeiling)
{
    auto level = sacredGround();
    Archer archer {};
    archer.position = {105.0f, 115.0f};
    archer.velocity = {0.0f, -25.0f};

    auto contacts = collision::moveAndCollide(level, archer, 1.0f);

    EXPECT_NEAR(9.0f * TILESIZE, archer.hitboxTL().y, EPSILON);
    EXPECT_FLOAT_EQ(0.0f, archer.velocity.y);
    EXPECT_TRUE(contacts.ceiling);
}


TEST(FightCollisionTest, FallsOntoGroundFlush)
{
    auto level = sacredGround();
    Archer archer {};
    archer.position = {85.0f, 160.0f};
    archer.velocity = {0.0f, 25.0f};

    auto contacts = collision::moveAndCollide(level, archer, 1.0f);

    EXPECT_NEAR(18.0f * TILESIZE, archer.hitboxBR().y, EPSILON);
    EXPECT_FLOAT_EQ(0.0f, archer.velocity.y);
    EXPECT_TRUE(contacts.ground);
}


TEST(FightCollisionTest, SubstepsPreventJumpTunneling)
{
    auto level = sacredGround();
    Archer archer {};
    archer.position = {105.0f, 115.0f};
    archer.velocity = {0.0f, -80.0f};

    auto contacts = collision::moveAndCollide(level, archer, 1.0f);

    EXPECT_NEAR(9.0f * TILESIZE, archer.hitboxTL().y, EPSILON);
    EXPECT_FALSE(collision::overlapsSolid(level, archer));
    EXPECT_TRUE(contacts.ceiling);
}


TEST(FightCollisionTest, ContactsDetectGroundAndWalls)
{
    auto level = sacredGround();

    Archer grounded {};
    grounded.position = {85.0f, 171.0f};
    auto groundContacts = collision::contactsAt(level, grounded);
    EXPECT_TRUE(groundContacts.ground);
    EXPECT_FALSE(groundContacts.ceiling);

    Archer leftWall {};
    leftWall.position = {25.0f, 45.0f};
    auto leftContacts = collision::contactsAt(level, leftWall);
    EXPECT_TRUE(leftContacts.leftWall);
    EXPECT_FALSE(leftContacts.rightWall);

    Archer rightWall {};
    rightWall.position = {295.0f, 45.0f};
    auto rightContacts = collision::contactsAt(level, rightWall);
    EXPECT_TRUE(rightContacts.rightWall);
    EXPECT_FALSE(rightContacts.leftWall);
}


TEST(FightCollisionTest, ContactsUseWrappedTilesAtHorizontalSeam)
{
    auto level = sacredGround();

    Archer leftSeam {};
    leftSeam.position = {-6.0f, 211.0f};
    auto leftContacts = collision::contactsAt(level, leftSeam);
    EXPECT_TRUE(leftContacts.ground);

    Archer rightSeam {};
    rightSeam.position = {326.0f, 211.0f};
    auto rightContacts = collision::contactsAt(level, rightSeam);
    EXPECT_TRUE(rightContacts.ground);
}


TEST(FightCollisionTest, WrapsWhenFullyPastEachEdge)
{
    auto level = sacredGround();
    glm::vec2 velocity = {3.0f, -4.0f};

    Archer left {};
    left.position = {-6.0f, 100.0f};
    left.velocity = velocity;
    collision::wrapIfFullyOutside(level, left);
    EXPECT_NEAR(314.0f, left.position.x, EPSILON);
    EXPECT_EQ(velocity, left.velocity);

    Archer right {};
    right.position = {326.0f, 100.0f};
    right.velocity = velocity;
    collision::wrapIfFullyOutside(level, right);
    EXPECT_NEAR(6.0f, right.position.x, EPSILON);
    EXPECT_EQ(velocity, right.velocity);

    Archer top {};
    top.position = {100.0f, -10.0f};
    top.velocity = velocity;
    collision::wrapIfFullyOutside(level, top);
    EXPECT_NEAR(230.0f, top.position.y, EPSILON);
    EXPECT_EQ(velocity, top.velocity);

    Archer bottom {};
    bottom.position = {100.0f, 250.0f};
    bottom.velocity = velocity;
    collision::wrapIfFullyOutside(level, bottom);
    EXPECT_NEAR(10.0f, bottom.position.y, EPSILON);
    EXPECT_EQ(velocity, bottom.velocity);
}


TEST(FightCollisionTest, DoesNotWrapWhilePartiallyVisible)
{
    auto level = sacredGround();
    glm::vec2 velocity = {3.0f, -4.0f};

    Archer archer {};
    archer.position = {-4.0f, -8.0f};
    archer.velocity = velocity;
    collision::wrapIfFullyOutside(level, archer);

    EXPECT_NEAR(-4.0f, archer.position.x, EPSILON);
    EXPECT_NEAR(-8.0f, archer.position.y, EPSILON);
    EXPECT_EQ(velocity, archer.velocity);
}


TEST(FightCollisionTest, DisabledWrapModeDoesNotRepeatTilesOrTeleport)
{
    auto path = sacredGroundWithWrapMode("None");
    Level level(Stage::SACRED_GROUND, path.c_str());

    Archer leftSeam {};
    leftSeam.position = {-6.0f, 211.0f};
    auto leftContacts = collision::contactsAt(level, leftSeam);
    EXPECT_FALSE(leftContacts.ground);

    glm::vec2 velocity = {3.0f, -4.0f};
    Archer outside {};
    outside.position = {-6.0f, 100.0f};
    outside.velocity = velocity;
    collision::wrapIfFullyOutside(level, outside);

    EXPECT_NEAR(-6.0f, outside.position.x, EPSILON);
    EXPECT_NEAR(100.0f, outside.position.y, EPSILON);
    EXPECT_EQ(velocity, outside.velocity);

    std::filesystem::remove(path);
}
