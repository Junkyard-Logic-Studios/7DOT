#include "collision.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include "../constants.hpp"



namespace
{
    constexpr float CONTACT_EPSILON = 0.1f;
    constexpr float OVERLAP_EPSILON = 0.001f;

    struct TileRange
    {
        int minX;
        int maxX;
        int minY;
        int maxY;
    };

    int floorToTile(float world)
    {
        return static_cast<int>(std::floor(world / float(TILESIZE)));
    }

    std::size_t wrapTileIndex(int index, std::size_t size)
    {
        int wrapped = index % int(size);
        return std::size_t(wrapped < 0 ? wrapped + int(size) : wrapped);
    }

    bool tileIndexForAxis(int index, std::size_t size, bool wraps, std::size_t& result)
    {
        if (wraps)
        {
            result = wrapTileIndex(index, size);
            return true;
        }

        if (index < 0 || index >= int(size))
            return false;

        result = std::size_t(index);
        return true;
    }

    bool isSolidRepeatedAt(const fight::Level& level, int x, int y)
    {
        std::size_t tileX = 0;
        std::size_t tileY = 0;
        if (!tileIndexForAxis(x, level.getWidth(), level.wrapsHorizontally(), tileX)
            || !tileIndexForAxis(y, level.getHeight(), level.wrapsVertically(), tileY))
            return false;

        return level.getSolidAt(tileX, tileY) != -1;
    }

    TileRange overlappedTiles(const fight::Archer& archer)
    {
        glm::vec2 tl = archer.hitboxTL();
        glm::vec2 br = archer.hitboxBR();

        return {
            floorToTile(tl.x),
            floorToTile(br.x - OVERLAP_EPSILON),
            floorToTile(tl.y),
            floorToTile(br.y - OVERLAP_EPSILON)
        };
    }

    bool tileOverlapsArcher(const fight::Archer& archer, int x, int y)
    {
        glm::vec2 tl = archer.hitboxTL();
        glm::vec2 br = archer.hitboxBR();
        float tileLeft = float(x * TILESIZE);
        float tileRight = float((x + 1) * TILESIZE);
        float tileTop = float(y * TILESIZE);
        float tileBottom = float((y + 1) * TILESIZE);

        return tileLeft < br.x
            && tileRight > tl.x
            && tileTop < br.y
            && tileBottom > tl.y;
    }

    void resolveX(const fight::Level& level, fight::Archer& archer, float dx,
        fight::collision::Contacts& contacts)
    {
        if (dx == 0.0f || !fight::collision::overlapsSolid(level, archer))
            return;

        TileRange range = overlappedTiles(archer);

        if (dx > 0.0f)
        {
            float tileLeft = float(level.getWidth() * TILESIZE);
            for (int y = range.minY; y <= range.maxY; y++)
                for (int x = range.minX; x <= range.maxX; x++)
                    if (isSolidRepeatedAt(level, x, y) && tileOverlapsArcher(archer, x, y))
                        tileLeft = std::min(tileLeft, float(x * TILESIZE));

            archer.position.x = tileLeft - fight::Archer::WIDTH / 2.0f;
            archer.velocity.x = 0.0f;
            contacts.rightWall = true;
        }
        else
        {
            float tileRight = 0.0f;
            for (int y = range.minY; y <= range.maxY; y++)
                for (int x = range.minX; x <= range.maxX; x++)
                    if (isSolidRepeatedAt(level, x, y) && tileOverlapsArcher(archer, x, y))
                        tileRight = std::max(tileRight, float((x + 1) * TILESIZE));

            archer.position.x = tileRight + fight::Archer::WIDTH / 2.0f;
            archer.velocity.x = 0.0f;
            contacts.leftWall = true;
        }
    }

    void resolveY(const fight::Level& level, fight::Archer& archer, float dy,
        fight::collision::Contacts& contacts)
    {
        if (dy == 0.0f || !fight::collision::overlapsSolid(level, archer))
            return;

        TileRange range = overlappedTiles(archer);

        if (dy > 0.0f)
        {
            float tileTop = float(level.getHeight() * TILESIZE);
            for (int y = range.minY; y <= range.maxY; y++)
                for (int x = range.minX; x <= range.maxX; x++)
                    if (isSolidRepeatedAt(level, x, y) && tileOverlapsArcher(archer, x, y))
                        tileTop = std::min(tileTop, float(y * TILESIZE));

            archer.position.y = tileTop - fight::Archer::HEIGHT / 2.0f;
            archer.velocity.y = 0.0f;
            contacts.ground = true;
        }
        else
        {
            float tileBottom = 0.0f;
            for (int y = range.minY; y <= range.maxY; y++)
                for (int x = range.minX; x <= range.maxX; x++)
                    if (isSolidRepeatedAt(level, x, y) && tileOverlapsArcher(archer, x, y))
                        tileBottom = std::max(tileBottom, float((y + 1) * TILESIZE));

            archer.position.y = tileBottom + fight::Archer::HEIGHT / 2.0f;
            archer.velocity.y = 0.0f;
            contacts.ceiling = true;
        }
    }
}


bool fight::collision::overlapsSolid(const Level& level, const Archer& archer)
{
    TileRange range = overlappedTiles(archer);

    for (int y = range.minY; y <= range.maxY; y++)
        for (int x = range.minX; x <= range.maxX; x++)
            if (isSolidRepeatedAt(level, x, y) && tileOverlapsArcher(archer, x, y))
                return true;

    return false;
}


fight::collision::Contacts fight::collision::contactsAt(const Level& level, const Archer& archer)
{
    Contacts contacts;
    Archer probe = archer;

    probe.position.y += CONTACT_EPSILON;
    contacts.ground = overlapsSolid(level, probe);

    probe = archer;
    probe.position.y -= CONTACT_EPSILON;
    contacts.ceiling = overlapsSolid(level, probe);

    probe = archer;
    probe.position.x -= CONTACT_EPSILON;
    contacts.leftWall = overlapsSolid(level, probe);

    probe = archer;
    probe.position.x += CONTACT_EPSILON;
    contacts.rightWall = overlapsSolid(level, probe);

    return contacts;
}


fight::collision::Contacts fight::collision::moveAndCollide(
    const Level& level, Archer& archer, float positionScale)
{
    Contacts contacts;
    float totalDx = archer.velocity.x * positionScale;
    float totalDy = archer.velocity.y * positionScale;
    float maxDisplacement = std::max(std::abs(totalDx), std::abs(totalDy));
    int steps = std::max(1, static_cast<int>(std::ceil(maxDisplacement / (TILESIZE / 2.0f))));
    float stepScale = positionScale / steps;

    for (int i = 0; i < steps; i++)
    {
        float dx = archer.velocity.x * stepScale;
        archer.position.x += dx;
        resolveX(level, archer, dx, contacts);

        float dy = archer.velocity.y * stepScale;
        archer.position.y += dy;
        resolveY(level, archer, dy, contacts);
    }

    wrapIfFullyOutside(level, archer);
    return contacts;
}


void fight::collision::wrapIfFullyOutside(const Level& level, Archer& archer)
{
    float worldWidth = float(level.getWidth() * TILESIZE);
    float worldHeight = float(level.getHeight() * TILESIZE);
    glm::vec2 tl = archer.hitboxTL();
    glm::vec2 br = archer.hitboxBR();

    if (level.wrapsHorizontally() && br.x < 0.0f)
        archer.position.x += worldWidth;
    else if (level.wrapsHorizontally() && tl.x > worldWidth)
        archer.position.x -= worldWidth;

    tl = archer.hitboxTL();
    br = archer.hitboxBR();

    if (level.wrapsVertically() && br.y < 0.0f)
        archer.position.y += worldHeight;
    else if (level.wrapsVertically() && tl.y > worldHeight)
        archer.position.y -= worldHeight;
}
