#include "SpriteCatalog.hpp"
#include "pugixml.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>



namespace
{

    bool parseBool(const char* value, bool defaultValue = false)
    {
        if (!value || !*value)
            return defaultValue;

        std::string normalized(value);
        std::transform(normalized.begin(), normalized.end(), normalized.begin(),
            [](unsigned char c) { return std::tolower(c); });

        return normalized == "true" || normalized == "1";
    }

    std::vector<int> parseIntList(const std::string& values)
    {
        std::vector<int> result;
        std::stringstream stream(values);
        std::string item;

        while (std::getline(stream, item, ','))
        {
            auto first = std::find_if_not(item.begin(), item.end(),
                [](unsigned char c) { return std::isspace(c); });
            auto last = std::find_if_not(item.rbegin(), item.rend(),
                [](unsigned char c) { return std::isspace(c); }).base();

            if (first < last)
                result.push_back(std::stoi(std::string(first, last)));
        }

        return result;
    }

    std::string childText(const pugi::xml_node& node, const char* childName)
    {
        return node.child(childName).text().as_string();
    }

}


const renderer::SpriteAnimation* renderer::SpriteDef::animation(const std::string& id) const
{
    auto it = animations.find(id);
    return it == animations.end() ? nullptr : &it->second;
}


renderer::SpriteCatalog::SpriteCatalog(const std::string& xmlPath)
{
    load(xmlPath);
}


void renderer::SpriteCatalog::load(const std::string& xmlPath)
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(xmlPath.c_str());
    if (!result)
        throw std::runtime_error("Failed to parse file: " + xmlPath);

    _sprites.clear();
    for (auto spriteNode : doc.child("SpriteData").children())
    {
        std::string tag(spriteNode.name());
        if (tag != "sprite_string" && tag != "sprite_int")
            continue;

        SpriteDef sprite;
        sprite.id = spriteNode.attribute("id").as_string();
        sprite.texture = childText(spriteNode, "Texture");
        sprite.redTexture = childText(spriteNode, "RedTexture");
        sprite.blueTexture = childText(spriteNode, "BlueTexture");
        sprite.frameWidth = spriteNode.child("FrameWidth").text().as_int();
        sprite.frameHeight = spriteNode.child("FrameHeight").text().as_int();
        sprite.originX = spriteNode.child("OriginX").text().as_int();
        sprite.originY = spriteNode.child("OriginY").text().as_int();
        sprite.x = spriteNode.child("X").text().as_int();
        sprite.y = spriteNode.child("Y").text().as_int();
        sprite.downY = spriteNode.child("DownY").text().as_int();
        sprite.hideBowIdle = parseBool(spriteNode.child("HideBowIdle").text().as_string());

        std::string headXOrigins = childText(spriteNode, "HeadXOrigins");
        if (!headXOrigins.empty())
            sprite.headXOrigins = parseIntList(headXOrigins);

        std::string headYOrigins = childText(spriteNode, "HeadYOrigins");
        if (!headYOrigins.empty())
            sprite.headYOrigins = parseIntList(headYOrigins);

        for (auto animNode : spriteNode.child("Animations").children("Anim"))
        {
            SpriteAnimation anim;
            anim.id = animNode.attribute("id").as_string();
            anim.delay = animNode.attribute("delay").as_float();
            anim.loop = parseBool(animNode.attribute("loop").as_string(), true);
            anim.frames = parseIntList(animNode.attribute("frames").as_string());
            sprite.animations[anim.id] = anim;
        }

        if (!sprite.id.empty())
            _sprites[sprite.id] = sprite;
    }
}


const renderer::SpriteDef* renderer::SpriteCatalog::find(const std::string& id) const
{
    auto it = _sprites.find(id);
    return it == _sprites.end() ? nullptr : &it->second;
}


const renderer::SpriteDef& renderer::SpriteCatalog::get(const std::string& id) const
{
    const SpriteDef* sprite = find(id);
    if (!sprite)
        throw std::runtime_error("Sprite not found: " + id);

    return *sprite;
}


std::size_t renderer::SpriteCatalog::size() const
{
    return _sprites.size();
}
