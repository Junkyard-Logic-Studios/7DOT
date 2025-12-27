#pragma once
#include <inttypes.h>
#include <fstream>
#include "../constants.hpp"
#include "pugixml.hpp"



namespace fight
{

    struct Level
    {
        std::size_t width = 0;
        std::size_t height = 0;

        uint64_t* _solidBits = nullptr;
        int8_t* _solidTiles = nullptr;
        uint64_t* _backgroundBits = nullptr;
        int8_t* _backgroundTiles = nullptr;


        inline Level(const std::string& oelPath)
        {
            pugi::xml_document doc;
            pugi::xml_parse_result result = doc.load_file(oelPath.c_str());
            if (!result)
                throw std::runtime_error("Failed to parse file: " + oelPath);
            
            auto level = doc.child("level");

            width = level.attribute("width").as_ullong() / TILESIZE;
            height = level.attribute("height").as_ullong() / TILESIZE;

            _solidBits = new uint64_t[(width + 63) / 64 * height];
            _solidTiles = new int8_t[width * height];
            _backgroundBits = new uint64_t[(width + 63) / 64 * height];
            _backgroundTiles = new int8_t[width * height];
            
            memset(_solidBits, 0, (width + 63) / 64 * height * 8);
            memset(_solidTiles, -1, width * height);
            memset(_backgroundBits, 0, (width + 63) / 64 * height * 8);
            memset(_backgroundTiles, -1, width * height);

            const pugi::char_t* p;

            // fill foreground bitmap
            p = level.child("Solids").text().as_string();
            for (std::size_t x = 0, y = 0; *p != '\0'; x++, p++)
            {
                if (*p == '\n')
                {
                    x = 0;
                    p++;
                    y++;
                }
                _solidBits[(width + 63) / 64 * y + x / 64] |= 
                    (*p == '1' ? 1ul : 0ul) << (x % 64);
            }

            // fill foreground tiles
            {
            }

            // fill background bitmap
            p = level.child("BG").text().as_string();
            for (std::size_t x = 0, y = 0; *p != '\0'; x++, p++)
            {
                if (*p == '\n')
                {
                    x = 0;
                    p++;
                    y++;
                }
                _backgroundBits[(width + 63) / 64 * y + x / 64] |= 
                    (*p == '1' ? 1ul : 0ul) << (x % 64);
            }

            // fill background tiles
            p = level.child("BGTiles").text().as_string();
            const pugi::char_t* lastSep = p;
            for (std::size_t x = 0, y = 0; *p != '\0'; p++)
            {
                if (*p == ',' || *p == '\n')
                {
                    if (p - lastSep > 1)
                    {
                        _backgroundTiles[width * y + x] = std::atoi(lastSep + 1);
                        x++;
                    }
                    lastSep = p;
                }
                if (*p == '\n')
                {
                    x = 0;
                    y++;
                }
            }
        }

        inline ~Level()
        {
            delete _backgroundBits;
            delete _backgroundTiles;
            delete _solidBits;
            delete _solidTiles;
        }
    };

};  // end namespace fight
