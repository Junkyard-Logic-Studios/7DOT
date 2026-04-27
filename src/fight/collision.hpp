#pragma once
#include <inttypes.h>
#include "archer.hpp"
#include "level.hpp"



namespace fight::collision
{
    enum class ArcherHitboxPart : uint8_t
    {
        FULL,
        HEAD,
        BODY
    };

    struct Contacts
    {
        bool ground = false;
        bool ceiling = false;
        bool leftWall = false;
        bool rightWall = false;
    };

    bool overlapsSolid(const Level& level, const Archer& archer,
        ArcherHitboxPart part = ArcherHitboxPart::FULL);
    Contacts contactsAt(const Level& level, const Archer& archer,
        ArcherHitboxPart part = ArcherHitboxPart::FULL);
    Contacts moveAndCollide(const Level& level, Archer& archer, float positionScale);
    void wrapIfFullyOutside(const Level& level, Archer& archer);

};  // end namespace fight::collision
