#pragma once
#include <inttypes.h>
#include <fstream>
#include "../constants.hpp"
#include "SDL3/SDL_log.h"



namespace fight
{

    struct Level
    {
        std::size_t width = 0;
        std::size_t height = 0;

        uint64_t* _backgroundBits = nullptr;
        int8_t* _backgroundTiles = nullptr;
        uint64_t* _solidBits = nullptr;
        int8_t* _solidTiles = nullptr;

        std::string path;


        inline Level(const std::string& oelPath) :
            path(oelPath)
        {
            SDL_LogInfo(0, "loading level: %s", oelPath.c_str());

            std::ifstream file(oelPath);
            if (!file.is_open())
                throw std::runtime_error(oelPath + " - failed to open file");

            std::string line;
            auto getAttr = [&](const std::string &key) -> std::string
            {
                size_t pos = line.find(key + "=\"");
                if (pos == std::string::npos)
                    return "";
                pos += key.size() + 2;
                size_t end = line.find("\"", pos);
                return line.substr(pos, end - pos);
            };
            
            // get width and height
            std::getline(file, line);
            width = std::stoi(getAttr("width")) / TILESIZE;
            height = std::stoi(getAttr("height")) / TILESIZE;

            SDL_LogInfo(0, "-> width: %lu, height: %lu", width, height);

            _backgroundBits = new uint64_t[(width + 63) / 64 * height];
            _backgroundTiles = new int8_t[width * height];
            memset(_backgroundTiles, -1, width * height);
            _solidBits = new uint64_t[(width + 63) / 64 * height];
            _solidTiles = new int8_t[width * height];
            memset(_solidTiles, -1, width * height);


            // fill background bitmap
            {
                std::size_t x, y = 0;
                do
                {
                    std::getline(file, line);
                    if (y == 0)
                        line = line.substr(line.find(">") + 1);

                    for (x = 0; x < width; x++)
                        _backgroundBits[(width + 63) / 64 * y + x / 64] |= 
                            (line[x] == '1' ? 1ul : 0ul) << (x % 64);
                    y++;
                }
                while (line.find("</BG>") == std::string::npos);
            }

            // fill background tiles
            {
                std::getline(file, line);
                std::size_t y = 0;
                do
                {
                    std::getline(file, line);
                    std::size_t end = std::min(line.size(), line.find("</BGTiles>"));
                    for (std::size_t from = 0, x = 0; from < end; x++)
                    {
                        std::size_t to = std::min(end, line.find(',', from));
                        _backgroundTiles[width * y + x] = std::stoi(line.substr(from, to - from));
                        from = to + 1;
                    }
                    y++;
                }
                while (line.find("</BGTiles>") == std::string::npos);
            }

            // fill foreground bitmap
            {
                std::size_t x, y = 0;
                do
                {
                    std::getline(file, line);
                    if (y == 0)
                        line = line.substr(line.find(">") + 1);

                    for (x = 0; x < width; x++)
                        _solidBits[(width + 63) / 64 * y + x / 64] |= 
                            (line[x] == '1' ? 1ul : 0ul) << (x % 64);
                    y++;
                }
                while (line.find("</Solids>") == std::string::npos);
            }

            // fill foreground tiles
            {

            }


            file.close();
        }

        inline ~Level()
        {
            SDL_LogInfo(0, "unloading level: %s", path.c_str());
            delete _backgroundBits;
            delete _backgroundTiles;
            delete _solidBits;
            delete _solidTiles;
        }
    };

};  // end namespace fight
