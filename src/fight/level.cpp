#include "level.hpp"
#include <cstring>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <algorithm>
#include "../constants.hpp"
#include "stage.hpp"
#include "pugixml.hpp"



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


inline Tileset& getTileset(fight::Stage stage, bool background = false)
{
    static std::unordered_map<fight::Stage, Tileset> fgTilesets;
    static std::unordered_map<fight::Stage, Tileset> bgTilesets;

    // parse tileset data XML
    if (fgTilesets.empty())
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
            auto id = child.attribute("id").as_string();
            fight::Stage st = fight::stageFromName(id);
            
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
            
            if (strstr(id, "BG") == nullptr)
                fgTilesets[st] = tileset;
            else
                bgTilesets[st] = tileset;
        }
    }

    return background ? bgTilesets[stage] : fgTilesets[stage];
}


fight::Level::Level(Stage stage, const char* oelPath)
{
    // load level XML
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(oelPath);
    if (!result)
        throw std::runtime_error("Failed to parse file: " + std::string(oelPath));
    
    auto level = doc.child("level");

    // get width and height
    _width = level.attribute("width").as_ullong() / TILESIZE;
    _height = level.attribute("height").as_ullong() / TILESIZE;
    
    // initialize arrays
    _solidBits = new uint64_t[(_width + 63) / 64 * _height];
    _solidTiles = new int8_t[_width * _height];
    _backgroundBits = new uint64_t[(_width + 63) / 64 * _height];
    _backgroundTiles = new int8_t[_width * _height];
    
    std::fill(_solidBits,      _solidBits      + (_width + 63) / 64 * _height, 0);
    std::fill(_backgroundBits, _backgroundBits + (_width + 63) / 64 * _height, 0);

    // get tilesets
    auto& solidTileset = getTileset(stage, false);
    auto& backgroundTileset = getTileset(stage, true);

    // function to check occupation of neighboring cells in a bitmap
    auto fnFreeBitmapNeighbors = [&](uint64_t* bitmap, std::size_t xc, std::size_t yc) -> uint8_t
    {
        std::size_t xl = (xc + _width - 1) % _width;
        std::size_t xr = (xc + 1) % _width;
        std::size_t yu = (yc + _height - 1) % _height;
        std::size_t yd = (yc + 1) % _height;
        
        std::size_t stride = (_width + 63) / 64;
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
        _solidBits[(_width + 63) / 64 * y + x / 64] |= 
            (*p == '1' ? 1ul : 0ul) << (x % 64);
    }
    
    // fill foreground tiles
    for (std::size_t y = 0; y < _height; y++)
        for (std::size_t x = 0; x < _width; x++)
        {
            uint8_t freeN = fnFreeBitmapNeighbors(_solidBits, x, y);
            _solidTiles[_width * y + x] = solidTileset.getTile(freeN);
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
        _backgroundBits[(_width + 63) / 64 * y + x / 64] |= 
            (*p == '1' ? 1ul : 0ul) << (x % 64);
    }

    // fill background tiles
    for (std::size_t y = 0; y < _height; y++)
        for (std::size_t x = 0; x < _width; x++)
        {
            uint8_t freeN = fnFreeBitmapNeighbors(_solidBits, x, y)
                            & fnFreeBitmapNeighbors(_backgroundBits, x, y);
            _backgroundTiles[_width * y + x] = backgroundTileset.getTile(freeN);
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
                    _backgroundTiles[_width * y + x] = val;
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

    // parse player spawn locations
    for (auto child : doc.child("level").child("Entities").children("TeamSpawnA"))
        _playerSpawns.push_back(glm::vec2(
            child.attribute("x").as_float(), 
            child.attribute("y").as_float()));

    for (auto child : doc.child("level").child("Entities").children("TeamSpawnB"))
        _playerSpawns.push_back(glm::vec2(
            child.attribute("x").as_float(), 
            child.attribute("y").as_float()));
    
    for (auto child : doc.child("level").child("Entities").children("PlayerSpawn"))
        _playerSpawns.push_back(glm::vec2(
            child.attribute("x").as_float(), 
            child.attribute("y").as_float()));
}


fight::Level::~Level()
{
    delete[] _backgroundBits;
    delete[] _backgroundTiles;
    delete[] _solidBits;
    delete[] _solidTiles;
}


std::size_t fight::Level::getWidth() const
    { return _width; }


std::size_t fight::Level::getHeight() const
    { return _height; }


const uint64_t* fight::Level::getBitmapSolids() const
    { return _solidBits; }


const int8_t* fight::Level::getTilesSolids() const
    { return _solidTiles; }


int8_t fight::Level::getSolidAt(std::size_t x, std::size_t y) const
{
    return _solidBits[(_width + 63) / 64 * y + x / 64] & (1ul << (x % 64)) ?
        _solidTiles[_width * y + x] : -1;
}


const uint64_t* fight::Level::getBitmapBackground() const
    { return _backgroundBits; }


const int8_t* fight::Level::getTilesBackground() const
    { return _backgroundTiles; }


int8_t fight::Level::getBackgroundAt(std::size_t x, std::size_t y) const
{
    return _backgroundBits[(_width + 63) / 64 * y + x / 64] & (1ul << (x % 64)) ?
        _backgroundTiles[_width * y + x] : -1;
}

glm::vec2 fight::Level::getPlayerSpawnLocation(std::size_t index) const
    { return _playerSpawns.at(index % _playerSpawns.size()); }
