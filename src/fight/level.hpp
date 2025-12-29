#pragma once
#include <inttypes.h>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <algorithm>
#include "../constants.hpp"
#include "stage.hpp"
#include "pugixml.hpp"



namespace fight
{

    struct Tileset
    {
        std::vector<int8_t> center;
        std::vector<int8_t> single;
        std::vector<int8_t> singleHorizontalLeft;
        std::vector<int8_t> singleHorizontalCenter;
        std::vector<int8_t> singleHorizontalRight;
        std::vector<int8_t> singleVerticalTop;
        std::vector<int8_t> singleVerticalCenter;
        std::vector<int8_t> singleVerticalBottom;
        std::vector<int8_t> left;
        std::vector<int8_t> right;
        std::vector<int8_t> top;
        std::vector<int8_t> bottom;
        std::vector<int8_t> topLeft;
        std::vector<int8_t> topRight;
        std::vector<int8_t> bottomLeft;
        std::vector<int8_t> bottomRight;
        std::vector<int8_t> insideTopLeft;
        std::vector<int8_t> insideTopRight;
        std::vector<int8_t> insideBottomLeft;
        std::vector<int8_t> insideBottomRight;

        int8_t getTile(uint8_t freeNeighbors)
        {
            std::vector<int8_t>* map[] = 
            {
                &center,
                &bottom,
                &right,
                &bottomRight,
                &left,
                &bottomLeft,
                &singleVerticalCenter,
                &singleVerticalBottom,
                &top,
                &singleHorizontalCenter,
                &topRight,
                &singleHorizontalRight,
                &topLeft,
                &singleHorizontalLeft,
                &singleVerticalTop,
                &single
            };

            auto& v = freeNeighbors != 0b1000'0000 ?
                     (freeNeighbors != 0b0100'0000 ?
                     (freeNeighbors != 0b0010'0000 ?
                     (freeNeighbors != 0b0001'0000 ?
                        *map[freeNeighbors & 0b1111] :
                        insideTopLeft) :
                        insideTopRight) :
                        insideBottomLeft) :
                        insideBottomRight;

            return v[rand() % v.size()];
        }
    };

    inline Tileset& getTileset(std::string tilesetID)
    {
        static std::unordered_map<std::string, Tileset> tilesets;

        // parse tileset data XML
        if (tilesets.empty())
        {
            // load document
            pugi::xml_document doc;
            const char* path = ASSET_DIR "Atlas/GameData/tilesetData.xml";
            pugi::xml_parse_result result = doc.load_file(path);
            if (!result)
                throw std::runtime_error("Failed to parse file: " + std::string(path));

            // iterate over tilesets
            for (auto& child : doc.child("TilesetData").children("Tileset"))
            {
                std::string id = child.attribute("id").as_string();
                for (auto& c : id)
                    c = tolower(c);
                
                auto fnNodeContent = [](const pugi::xml_node node) -> std::vector<int8_t>
                {
                    std::vector<int8_t> result;
                    std::string str = node.text().as_string();
                    std::size_t from = 0;
                    std::size_t to = str.find(',');

                    do
                    {
                        result.push_back(std::stoi(str.substr(from, to - from)));
                        from = to + 1;
                        to = str.find(',', from);
                    }
                    while (to != std::string::npos);
                    
                    return result;
                };

                Tileset tileset;
                tileset.center                 = fnNodeContent(child.child("Center"));
                tileset.single                 = fnNodeContent(child.child("Single"));
                tileset.singleHorizontalLeft   = fnNodeContent(child.child("SingleHorizontalLeft"));
                tileset.singleHorizontalCenter = fnNodeContent(child.child("SingleHorizontalCenter"));
                tileset.singleHorizontalRight  = fnNodeContent(child.child("SingleHorizontalRight"));
                tileset.singleVerticalTop      = fnNodeContent(child.child("SingleVerticalTop"));
                tileset.singleVerticalCenter   = fnNodeContent(child.child("SingleVerticalCenter"));
                tileset.singleVerticalBottom   = fnNodeContent(child.child("SingleVerticalBottom"));
                tileset.left                   = fnNodeContent(child.child("Left"));
                tileset.right                  = fnNodeContent(child.child("Right"));
                tileset.top                    = fnNodeContent(child.child("Top"));
                tileset.bottom                 = fnNodeContent(child.child("Bottom"));
                tileset.topLeft                = fnNodeContent(child.child("TopLeft"));
                tileset.topRight               = fnNodeContent(child.child("TopRight"));
                tileset.bottomLeft             = fnNodeContent(child.child("BottomLeft"));
                tileset.bottomRight            = fnNodeContent(child.child("BottomRight"));
                tileset.insideTopLeft          = fnNodeContent(child.child("InsideTopLeft"));
                tileset.insideTopRight         = fnNodeContent(child.child("InsideTopRight"));
                tileset.insideBottomLeft       = fnNodeContent(child.child("InsideBottomLeft"));
                tileset.insideBottomRight      = fnNodeContent(child.child("InsideBottomRight"));
                
                tilesets[id] = tileset;
            }
        }

        for (auto& c : tilesetID)
            c = tolower(c);

        tilesetID.erase(std::remove_if(tilesetID.begin(), 
            tilesetID.end(), isspace), tilesetID.end());

        return tilesets.at(tilesetID);
    }

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
            memset(_backgroundBits, 0, (width + 63) / 64 * height * 8);

            // get tilesets
            std::string tilesetID = std::filesystem::path(oelPath).parent_path().filename();
            tilesetID = tilesetID.substr(tilesetID.find('-') + 1);
            auto& solidTileset = getTileset(tilesetID);
            auto& backgroundTileset = getTileset(tilesetID + "BG");

            auto fnFreeBitmapNeighbors = [&](uint64_t* bitmap, std::size_t xc, std::size_t yc) -> uint8_t
            {
                std::size_t xl = (xc + width - 1) % width;
                std::size_t xr = (xc + 1) % width;
                std::size_t yu = (yc + height - 1) % height;
                std::size_t yd = (yc + 1) % height;
                
                std::size_t stride = (width + 63) / 64;
                uint8_t occupied = 0;
                occupied |= bitmap[stride * yu + xl / 64] & (1ul << (xl % 64)) ? 1 << 7 : 0;     // 7 | 6
                occupied |= bitmap[stride * yu + xr / 64] & (1ul << (xr % 64)) ? 1 << 6 : 0;     // --+--
                occupied |= bitmap[stride * yd + xl / 64] & (1ul << (xl % 64)) ? 1 << 5 : 0;     // 5 | 4
                occupied |= bitmap[stride * yd + xr / 64] & (1ul << (xr % 64)) ? 1 << 4 : 0;
                occupied |= bitmap[stride * yu + xc / 64] & (1ul << (xc % 64)) ? 1 << 3 : 0;     //   3  
                occupied |= bitmap[stride * yc + xl / 64] & (1ul << (xl % 64)) ? 1 << 2 : 0;     // 2 + 1
                occupied |= bitmap[stride * yc + xr / 64] & (1ul << (xr % 64)) ? 1 << 1 : 0;     //   0  
                occupied |= bitmap[stride * yd + xc / 64] & (1ul << (xc % 64)) ? 1 << 0 : 0;
                return ~occupied;
            };

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
            for (std::size_t y = 0; y < height; y++)
                for (std::size_t x = 0; x < width; x++)
                {                    
                    uint8_t freeN = fnFreeBitmapNeighbors(_solidBits, x, y);
                    _solidTiles[width * y + x] = solidTileset.getTile(freeN);
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
            for (std::size_t y = 0; y < height; y++)
                for (std::size_t x = 0; x < width; x++)
                {
                    uint8_t freeN = fnFreeBitmapNeighbors(_solidBits, x, y)
                                  & fnFreeBitmapNeighbors(_backgroundBits, x, y);
                    _backgroundTiles[width * y + x] = backgroundTileset.getTile(freeN);
                }

            // add special background tiles
            p = level.child("BGTiles").text().as_string();
            const pugi::char_t* lastSep = p;
            for (std::size_t x = 0, y = 0; *p != '\0'; p++)
            {
                if (*p == ',' || *p == '\n')
                {
                    if (p - lastSep > 1)
                    {
                        int val = std::atoi(lastSep + 1);
                        if (val != -1)
                            _backgroundTiles[width * y + x] = val;
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
            delete[] _backgroundBits;
            delete[] _backgroundTiles;
            delete[] _solidBits;
            delete[] _solidTiles;
        }
    };

};  // end namespace fight
