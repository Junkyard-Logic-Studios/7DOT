#pragma once
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>



namespace renderer
{

    struct SpriteAnimation
    {
        std::string id;
        std::vector<int> frames;
        float delay = 0.0f;
        bool loop = true;
    };

    struct SpriteDef
    {
        std::string id;
        std::string texture;
        std::string redTexture;
        std::string blueTexture;

        int frameWidth = 0;
        int frameHeight = 0;
        int originX = 0;
        int originY = 0;
        int x = 0;
        int y = 0;
        int downY = 0;
        bool hideBowIdle = false;

        std::unordered_map<std::string, SpriteAnimation> animations;
        std::vector<int> headXOrigins;
        std::vector<int> headYOrigins;

        const SpriteAnimation* animation(const std::string& id) const;
    };

    class SpriteCatalog
    {
    public:
        SpriteCatalog() = default;
        explicit SpriteCatalog(const std::string& xmlPath);

        void load(const std::string& xmlPath);
        const SpriteDef* find(const std::string& id) const;
        const SpriteDef& get(const std::string& id) const;
        std::size_t size() const;

    private:
        std::unordered_map<std::string, SpriteDef> _sprites;
    };

};  // end namespace renderer
