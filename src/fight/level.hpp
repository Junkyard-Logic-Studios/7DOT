#pragma once
#include <inttypes.h>
#include <cstddef>



namespace fight
{

    class Level
    {
    public:
        Level(const char* oelPath);
        ~Level();

        std::size_t getWidth() const;
        std::size_t getHeight() const;

        const uint64_t* getBitmapSolids() const;
        const int8_t* getTilesSolids() const;
        int8_t getSolidAt(std::size_t x, std::size_t y) const;
        const uint64_t* getBitmapBackground() const;
        const int8_t* getTilesBackground() const;
        int8_t getBackgroundAt(std::size_t x, std::size_t y) const;

    private:
        std::size_t _width = 0;
        std::size_t _height = 0;
        uint64_t* _solidBits = nullptr;
        int8_t* _solidTiles = nullptr;
        uint64_t* _backgroundBits = nullptr;
        int8_t* _backgroundTiles = nullptr;
    };

};  // end namespace fight
