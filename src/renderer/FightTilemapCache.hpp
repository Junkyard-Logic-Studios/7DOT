#pragma once
#include "../fight/stage.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>



namespace renderer
{

    struct FightTilemapCacheKey
    {
        fight::Stage stage = fight::Stage::MAX_ENUM;
        std::uint8_t levelIndex = 0;
        std::size_t width = 0;
        std::size_t height = 0;
        std::string tilesetName;

        bool operator==(const FightTilemapCacheKey& other) const
        {
            return stage == other.stage
                && levelIndex == other.levelIndex
                && width == other.width
                && height == other.height
                && tilesetName == other.tilesetName;
        }
    };


    class FightTilemapCacheState
    {
    public:
        bool shouldRebuild(const FightTilemapCacheKey& key) const
        {
            return !_key || *_key != key;
        }

        void markValid(const FightTilemapCacheKey& key)
        {
            _key = key;
        }

        void invalidate()
        {
            _key.reset();
        }

    private:
        std::optional<FightTilemapCacheKey> _key;
    };

};  // end namespace renderer
