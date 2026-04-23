#pragma once
#include "archer.hpp"
#include "level.hpp"



namespace fight::collision
{

    struct Contacts
    {
        bool ground = false;
        bool ceiling = false;
        bool leftWall = false;
        bool rightWall = false;
    };

    bool overlapsSolid(const Level& level, const Archer& archer);
    Contacts contactsAt(const Level& level, const Archer& archer);
    Contacts moveAndCollide(const Level& level, Archer& archer, float positionScale);
    void wrapIfFullyOutside(const Level& level, Archer& archer);

};  // end namespace fight::collision
