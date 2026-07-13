#include "CharacterStore.hpp"

#include "../constants.hpp"
#include "pugixml.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace charactereditor
{
namespace
{

constexpr int MAX_CHARACTERS = 256;
constexpr int MAX_CANVASES = 64;
constexpr int MAX_FRAMES = 256;
constexpr std::size_t MAX_NAME_LENGTH = 64;
constexpr std::size_t MAX_LABEL_LENGTH = 64;
constexpr std::size_t MAX_PALETTE_COLORS = 256;

const char* METADATA_FILENAME = "characters.xml";
const char* ATLAS_FILENAME = "customCharacterAtlas.bmp";
const char* ATLAS_XML_FILENAME = "customCharacterAtlas.xml";

using SurfacePtr = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;

void setError(std::string* error, const std::string& message)
{
    if (error)
        *error = message;
}

std::string trim(std::string value)
{
    const auto isSpace = [](unsigned char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
    };
    value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), isSpace));
    value.erase(std::find_if_not(value.rbegin(), value.rend(), isSpace).base(), value.end());
    return value;
}

std::string sanitizeDisplayText(std::string value)
{
    value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char c) {
        return c < 0x20 || c == 0x7f;
    }), value.end());
    return trim(std::move(value));
}

void truncateUtf8Bytes(std::string& value, std::size_t maximumBytes)
{
    if (value.size() <= maximumBytes)
        return;

    std::size_t end = maximumBytes;
    while (end > 0 && (static_cast<unsigned char>(value[end]) & 0xc0) == 0x80)
        --end;
    value.resize(end);
}

bool isSafeId(std::string_view id)
{
    if (id.empty() || id.size() > 64)
        return false;

    return std::all_of(id.begin(), id.end(), [](unsigned char c) {
        const bool asciiAlphaNumeric = (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        return asciiAlphaNumeric || c == '-' || c == '_';
    });
}

std::string slugify(std::string_view text)
{
    std::string slug;
    bool needsSeparator = false;
    for (const unsigned char c : text)
    {
        const bool asciiAlphaNumeric = (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        if (asciiAlphaNumeric)
        {
            if (needsSeparator && !slug.empty())
                slug.push_back('-');
            slug.push_back(static_cast<char>(std::tolower(c)));
            needsSeparator = false;
        }
        else
        {
            needsSeparator = true;
        }

        if (slug.size() >= 40)
            break;
    }
    if (slug.empty())
        slug = "character";
    return slug;
}

std::string uniqueId(std::string_view name, const std::vector<CustomCharacter>& characters)
{
    const std::string base = slugify(name);
    const auto exists = [&characters](std::string_view candidate) {
        return std::any_of(characters.begin(), characters.end(), [candidate](const CustomCharacter& item) {
            return item.id == candidate;
        });
    };

    if (!exists(base))
        return base;

    for (unsigned int suffix = 2; suffix < 1000000; ++suffix)
    {
        const std::string candidate = base + '-' + std::to_string(suffix);
        if (!exists(candidate))
            return candidate;
    }

    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return base + '-' + std::to_string(stamp);
}

std::string hexColor(std::uint32_t color)
{
    std::ostringstream output;
    output << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << color;
    return output.str();
}

bool parseColor(std::string_view text, std::uint32_t& color)
{
    if (text.size() != 8)
        return false;

    std::uint32_t result = 0;
    for (const unsigned char c : text)
    {
        result <<= 4;
        if (c >= '0' && c <= '9')
            result |= c - '0';
        else if (c >= 'a' && c <= 'f')
            result |= c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            result |= c - 'A' + 10;
        else
            return false;
    }
    color = result;
    return true;
}

bool checkedPixelCount(int frameCount, std::size_t& pixelCount)
{
    if (frameCount < 1 || frameCount > MAX_FRAMES)
        return false;
    pixelCount = static_cast<std::size_t>(FRAME_WIDTH) * FRAME_HEIGHT
        * static_cast<std::size_t>(frameCount);
    return true;
}

bool validateAndNormalize(CustomCharacter& character, std::string& error)
{
    character.name = sanitizeDisplayText(std::move(character.name));
    if (!isSafeId(character.id))
    {
        error = "Character id must contain only letters, numbers, '-' or '_' (maximum 64 characters)";
        return false;
    }
    if (character.name.empty() || character.name.size() > MAX_NAME_LENGTH)
    {
        error = "Character name must be between 1 and 64 characters";
        return false;
    }
    if (character.canvases.empty() || character.canvases.size() > MAX_CANVASES)
    {
        error = "A character must contain between 1 and 64 canvases";
        return false;
    }
    if (character.palette.empty() || character.palette.size() > MAX_PALETTE_COLORS)
    {
        error = "A character palette must contain between 1 and 256 colors";
        return false;
    }

    std::unordered_set<std::string> canvasIds;
    for (Canvas& item : character.canvases)
    {
        item.label = sanitizeDisplayText(std::move(item.label));
        if (!isSafeId(item.id))
        {
            error = "Canvas id must contain only letters, numbers, '-' or '_' (maximum 64 characters)";
            return false;
        }
        if (!canvasIds.emplace(item.id).second)
        {
            error = "Canvas ids must be unique within a character";
            return false;
        }
        if (item.label.empty() || item.label.size() > MAX_LABEL_LENGTH)
        {
            error = "Canvas label must be between 1 and 64 characters";
            return false;
        }

        std::size_t expectedPixels = 0;
        if (!checkedPixelCount(item.frameCount, expectedPixels))
        {
            error = "Canvas frame count must be between 1 and 256";
            return false;
        }
        if (item.pixels.size() != expectedPixels)
        {
            error = "Canvas pixel count does not match its frame count";
            return false;
        }
    }

    static constexpr std::array<const char*, 3> requiredParts = {"bow", "body", "head"};
    if (character.canvases.size() != requiredParts.size())
    {
        error = "A character must contain exactly the Bow, Body, and Head canvases";
        return false;
    }
    for (const char* part : requiredParts)
    {
        const auto found = std::find_if(character.canvases.begin(), character.canvases.end(),
            [part](const Canvas& item) { return item.id == part; });
        if (found == character.canvases.end() || found->frameCount != POSE_FRAME_COUNT)
        {
            error = "Bow, Body, and Head must each contain the complete animation frame set";
            return false;
        }
    }
    std::sort(character.canvases.begin(), character.canvases.end(),
        [](const Canvas& left, const Canvas& right) {
            const auto rank = [](const std::string& id) {
                if (id == "bow") return 0;
                if (id == "body") return 1;
                return 2;
            };
            return rank(left.id) < rank(right.id);
        });
    return true;
}

std::uint8_t red(std::uint32_t pixel) { return static_cast<std::uint8_t>(pixel >> 24); }
std::uint8_t green(std::uint32_t pixel) { return static_cast<std::uint8_t>(pixel >> 16); }
std::uint8_t blue(std::uint32_t pixel) { return static_cast<std::uint8_t>(pixel >> 8); }
std::uint8_t alpha(std::uint32_t pixel) { return static_cast<std::uint8_t>(pixel); }

std::uint32_t pack(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
    return (static_cast<std::uint32_t>(r) << 24)
        | (static_cast<std::uint32_t>(g) << 16)
        | (static_cast<std::uint32_t>(b) << 8)
        | a;
}

bool replaceFile(const std::filesystem::path& temporary, const std::filesystem::path& target,
    std::string& error)
{
    std::error_code code;
    std::filesystem::rename(temporary, target, code);
    if (!code)
        return true;

    // Windows does not replace an existing destination. This fallback is less atomic,
    // but the metadata file is installed last so readers never see new metadata early.
    std::error_code existenceError;
    if (!std::filesystem::exists(temporary, existenceError) || existenceError)
    {
        error = "Could not install '" + target.string() + "': " + code.message();
        return false;
    }
    std::filesystem::remove(target, code);
    code.clear();
    std::filesystem::rename(temporary, target, code);
    if (!code)
        return true;

    error = "Could not install '" + target.string() + "': " + code.message();
    return false;
}

void cleanupTemporaryFiles(const std::array<std::filesystem::path, 3>& files)
{
    for (const auto& file : files)
    {
        std::error_code ignored;
        std::filesystem::remove(file, ignored);
    }
}

void paintRect(Canvas& target, int frame, int x, int y, int width, int height,
    std::uint32_t color)
{
    const int stripWidth = FRAME_WIDTH * target.frameCount;
    for (int py = std::max(0, y); py < std::min(FRAME_HEIGHT, y + height); ++py)
    {
        for (int px = std::max(0, x); px < std::min(FRAME_WIDTH, x + width); ++px)
        {
            target.pixels[static_cast<std::size_t>(py * stripWidth + frame * FRAME_WIDTH + px)] = color;
        }
    }
}

void drawStarterBody(Canvas& target, int frame, int bob, int leftLeg, int rightLeg,
    bool armsRaised)
{
    constexpr std::uint32_t outline = 0x211A2BFF;
    constexpr std::uint32_t skin = 0xF0B58AFF;
    constexpr std::uint32_t tunic = 0xD94C63FF;
    constexpr std::uint32_t tunicLight = 0xF56C79FF;
    constexpr std::uint32_t boots = 0x45304FFF;

    const int bodyY = 8 + bob;
    paintRect(target, frame, 3, bodyY, 6, 7, outline);
    paintRect(target, frame, 4, bodyY, 4, 6, tunic);
    paintRect(target, frame, 4, bodyY, 1, 5, tunicLight);
    if (armsRaised)
    {
        paintRect(target, frame, 1, bodyY - 2, 2, 5, outline);
        paintRect(target, frame, 9, bodyY - 2, 2, 5, outline);
        paintRect(target, frame, 1, bodyY - 2, 1, 2, skin);
        paintRect(target, frame, 10, bodyY - 2, 1, 2, skin);
    }
    else
    {
        paintRect(target, frame, 1, bodyY + 1, 2, 6, outline);
        paintRect(target, frame, 9, bodyY + 1, 2, 6, outline);
        paintRect(target, frame, 1, bodyY + 5, 1, 1, skin);
        paintRect(target, frame, 10, bodyY + 5, 1, 1, skin);
    }

    const int legY = 15 + bob;
    paintRect(target, frame, 3 + leftLeg, legY, 3, 4, outline);
    paintRect(target, frame, 6 + rightLeg, legY, 3, 4, outline);
    paintRect(target, frame, 3 + leftLeg, legY + 1, 2, 2, boots);
    paintRect(target, frame, 7 + rightLeg, legY + 1, 2, 2, boots);
}

void drawStarterHead(Canvas& target, int frame, int bob, int look = 0)
{
    constexpr std::uint32_t outline = 0x211A2BFF;
    constexpr std::uint32_t hair = 0x633F2AFF;
    constexpr std::uint32_t skin = 0xF0B58AFF;

    const int headY = 2 + bob;
    paintRect(target, frame, 3, headY, 6, 1, outline);
    paintRect(target, frame, 2, headY + 1, 8, 5, outline);
    paintRect(target, frame, 3, headY + 1, 6, 4, skin);
    paintRect(target, frame, 3, headY + 1, 6, 1, hair);
    paintRect(target, frame, 3, headY + 2, 1, 2, hair);
    if (look == 2)
    {
        paintRect(target, frame, 3, headY + 2, 5, 2, hair);
    }
    else
    {
        const int eyeY = headY + 3 + (look > 0 ? 1 : (look < 0 ? -1 : 0));
        paintRect(target, frame, 4, eyeY, 1, 1, outline);
        paintRect(target, frame, 7, eyeY, 1, 1, outline);
    }
}

void drawStarterBow(Canvas& target, int frame, int bob, bool airborne,
    int drawAmount = 0, bool empty = false)
{
    constexpr std::uint32_t outline = 0x211A2BFF;
    constexpr std::uint32_t wood = 0xB76A2AFF;
    constexpr std::uint32_t woodLight = 0xE2A24AFF;
    constexpr std::uint32_t string = 0xF7E7B6FF;

    const int top = 6 + bob - (airborne ? 1 : 0);
    paintRect(target, frame, 9, top, 1, 2, outline);
    paintRect(target, frame, 10, top + 1, 1, 3, woodLight);
    paintRect(target, frame, 11, top + 3, 1, 5, wood);
    paintRect(target, frame, 10, top + 8, 1, 3, woodLight);
    paintRect(target, frame, 9, top + 10, 1, 2, outline);
    if (!empty)
    {
        const int stringX = std::max(7, 9 - drawAmount);
        for (int y = top + 1; y <= top + 10; ++y)
            paintRect(target, frame, stringX, y, 1, 1, string);
        if (drawAmount > 0)
            paintRect(target, frame, stringX, top + 5, 9 - stringX, 1, string);
    }
}

void drawStarterAnimations(Canvas& bow, Canvas& body, Canvas& head)
{
    constexpr std::array<int, 6> runBob = {0, -1, 0, 0, -1, 0};
    constexpr std::array<int, 6> runLeft = {0, -1, -1, 0, 1, 1};
    constexpr std::array<int, 6> runRight = {0, 1, 1, 0, -1, -1};

    for (const AnimationSpec& animation : ANIMATIONS)
    {
        for (int localFrame = 0; localFrame < animation.frameCount; ++localFrame)
        {
            const int frame = animation.firstFrame + localFrame;
            int bob = 0;
            int leftLeg = 0;
            int rightLeg = 0;
            int look = 0;
            int bowDraw = 0;
            bool armsRaised = false;
            bool airborne = false;
            bool emptyBow = false;

            if (animation.id == "run" || animation.id == "run_aim")
            {
                bob = runBob[localFrame % runBob.size()];
                leftLeg = runLeft[localFrame % runLeft.size()];
                rightLeg = runRight[localFrame % runRight.size()];
            }
            else if (animation.id == "jump" || animation.id == "jump_aim"
                || animation.id.starts_with("jump_look"))
            {
                bob = -1;
                leftLeg = localFrame == 0 ? -1 : 0;
                rightLeg = localFrame == 0 ? 1 : -1;
                armsRaised = true;
                airborne = true;
            }
            else if (animation.id == "fall" || animation.id == "fall_aim"
                || animation.id == "glide" || animation.id == "glide_aim"
                || animation.id.starts_with("fall_look"))
            {
                bob = (localFrame & 1) ? 0 : -1;
                leftLeg = 1;
                rightLeg = -1;
                armsRaised = true;
                airborne = true;
            }
            else if (animation.id == "ledge")
            {
                bob = localFrame & 1;
                leftLeg = -1;
                armsRaised = true;
            }
            else if (animation.id == "dodge")
            {
                bob = 1 + (localFrame == 1);
                leftLeg = -1;
                rightLeg = 1;
                armsRaised = true;
            }
            else if (animation.id == "duck" || animation.id == "slide")
            {
                bob = 2;
                leftLeg = (localFrame & 1) ? -1 : 0;
                rightLeg = (localFrame & 1) ? 1 : 0;
            }

            if (animation.id.find("look_up") != std::string_view::npos)
                look = -1;
            else if (animation.id.find("look_down") != std::string_view::npos)
                look = 1;
            else if (animation.id.find("look_back") != std::string_view::npos)
                look = 2;

            if (animation.id.find("aim") != std::string_view::npos)
            {
                armsRaised = true;
                bowDraw = 2;
            }
            if (animation.id == "bow_draw")
            {
                armsRaised = true;
                bowDraw = localFrame;
            }
            if (animation.id == "bow_empty")
                emptyBow = true;

            drawStarterBody(body, frame, bob, leftLeg, rightLeg, armsRaised);
            drawStarterHead(head, frame, bob, look);
            drawStarterBow(bow, frame, bob, airborne, bowDraw, emptyBow);
        }
    }
}

void copyFrame(const Canvas& source, int sourceFrame, Canvas& target, int targetFrame)
{
    const int sourceWidth = FRAME_WIDTH * source.frameCount;
    const int targetWidth = FRAME_WIDTH * target.frameCount;
    for (int y = 0; y < FRAME_HEIGHT; ++y)
        for (int x = 0; x < FRAME_WIDTH; ++x)
            target.pixels[static_cast<std::size_t>(y * targetWidth + targetFrame * FRAME_WIDTH + x)]
                = source.pixels[static_cast<std::size_t>(y * sourceWidth + sourceFrame * FRAME_WIDTH + x)];
}

Canvas makeStarterCanvas(std::string id, std::string label, int frames);

int legacyFrameFor(const AnimationSpec& animation, int localFrame)
{
    if (animation.id == "run" || animation.id == "run_aim")
        return WALK_FIRST_FRAME + localFrame % WALK_FRAME_COUNT;
    if (animation.id == "jump" || animation.id == "jump_aim"
        || animation.id.starts_with("jump_look") || animation.id == "dodge")
        return 7;
    if (animation.id == "fall" || animation.id == "fall_aim"
        || animation.id == "glide" || animation.id == "glide_aim"
        || animation.id.starts_with("fall_look"))
        return 8;
    return 0;
}

void migrateVersion2(CustomCharacter& character)
{
    for (Canvas& oldCanvas : character.canvases)
    {
        if (oldCanvas.frameCount != 9)
            continue;
        Canvas expanded = makeStarterCanvas(oldCanvas.id, oldCanvas.label, POSE_FRAME_COUNT);
        for (const AnimationSpec& animation : ANIMATIONS)
            for (int localFrame = 0; localFrame < animation.frameCount; ++localFrame)
                copyFrame(oldCanvas, legacyFrameFor(animation, localFrame), expanded,
                    animation.firstFrame + localFrame);
        oldCanvas = std::move(expanded);
    }
}

Canvas makeStarterCanvas(std::string id, std::string label, int frames)
{
    Canvas result{std::move(id), std::move(label), frames, {}};
    result.pixels.assign(static_cast<std::size_t>(FRAME_WIDTH * frames * FRAME_HEIGHT), 0x00000000);
    return result;
}

} // namespace

CharacterStore::CharacterStore()
{
    std::string ignored;
    reload(&ignored);
}

std::filesystem::path CharacterStore::rootDirectory()
{
    return std::filesystem::path(CUSTOM_ASSET_DIR) / "Characters";
}

bool CharacterStore::reload(std::string* error)
{
    if (error)
        error->clear();

    const auto root = rootDirectory();
    const auto metadataPath = root / METADATA_FILENAME;
    const auto atlasPath = root / ATLAS_FILENAME;
    const auto atlasXmlPath = root / ATLAS_XML_FILENAME;

    std::error_code fileError;
    if (!std::filesystem::exists(metadataPath, fileError))
    {
        if (fileError)
        {
            setError(error, "Could not inspect custom character metadata: " + fileError.message());
            return false;
        }

        characters_.clear();
        return true;
    }

    pugi::xml_document metadata;
    const pugi::xml_parse_result metadataResult = metadata.load_file(metadataPath.string().c_str());
    if (!metadataResult)
    {
        setError(error, "Could not parse custom character metadata: " + std::string(metadataResult.description()));
        return false;
    }

    const pugi::xml_node rootNode = metadata.child("CustomCharacters");
    if (!rootNode)
    {
        setError(error, "Custom character metadata has no <CustomCharacters> root");
        return false;
    }
    const int metadataVersion = rootNode.attribute("version").as_int(1);
    if (metadataVersion < 2 && rootNode.child("Character"))
    {
        setError(error, "Legacy custom characters use the retired whole-character canvas format");
        return false;
    }

    pugi::xml_document atlasXml;
    const pugi::xml_parse_result atlasXmlResult = atlasXml.load_file(atlasXmlPath.string().c_str());
    if (!atlasXmlResult || !atlasXml.child("TextureAtlas"))
    {
        setError(error, "Could not parse the custom character atlas XML");
        return false;
    }

    SurfacePtr surface(SDL_LoadBMP(atlasPath.string().c_str()), SDL_DestroySurface);
    if (!surface)
    {
        setError(error, "Could not load the custom character atlas: " + std::string(SDL_GetError()));
        return false;
    }

    struct AtlasRect { int x; int y; int width; int height; };
    std::unordered_map<std::string, AtlasRect> atlasRects;
    for (const pugi::xml_node entry : atlasXml.child("TextureAtlas").children("SubTexture"))
    {
        const std::string name = entry.attribute("name").as_string();
        if (name.empty() || !atlasRects.emplace(name, AtlasRect{
            entry.attribute("x").as_int(-1), entry.attribute("y").as_int(-1),
            entry.attribute("width").as_int(-1), entry.attribute("height").as_int(-1)}).second)
        {
            setError(error, "Custom character atlas contains an invalid or duplicate SubTexture name");
            return false;
        }
    }

    std::vector<CustomCharacter> loaded;
    std::unordered_set<std::string> characterIds;
    for (const pugi::xml_node characterNode : rootNode.children("Character"))
    {
        if (loaded.size() >= MAX_CHARACTERS)
        {
            setError(error, "Custom character file exceeds the 256 character limit");
            return false;
        }

        CustomCharacter character;
        character.id = characterNode.attribute("id").as_string();
        character.name = characterNode.attribute("name").as_string();
        if (!characterIds.emplace(character.id).second)
        {
            setError(error, "Custom character ids must be unique");
            return false;
        }

        const pugi::xml_node paletteNode = characterNode.child("Palette");
        for (const pugi::xml_node colorNode : paletteNode.children("Color"))
        {
            std::uint32_t color = 0;
            if (!parseColor(colorNode.attribute("value").as_string(), color))
            {
                setError(error, "Custom character palette contains an invalid color");
                return false;
            }
            character.palette.push_back(color);
        }

        for (const pugi::xml_node canvasNode : characterNode.children("Canvas"))
        {
            Canvas item;
            item.id = canvasNode.attribute("id").as_string();
            item.label = canvasNode.attribute("label").as_string();
            item.frameCount = canvasNode.attribute("frameCount").as_int(0);

            std::size_t pixelCount = 0;
            if (!checkedPixelCount(item.frameCount, pixelCount))
            {
                setError(error, "Custom character canvas contains an invalid frame count");
                return false;
            }

            const std::string atlasName = "custom/" + character.id + "/" + item.id;
            const auto rectIt = atlasRects.find(atlasName);
            if (rectIt == atlasRects.end())
            {
                setError(error, "Custom character canvas is missing from the atlas: " + atlasName);
                return false;
            }
            const AtlasRect& rect = rectIt->second;
            const int expectedWidth = FRAME_WIDTH * item.frameCount;
            if (rect.x < 0 || rect.y < 0 || rect.width != expectedWidth || rect.height != FRAME_HEIGHT
                || rect.x > surface->w - rect.width || rect.y > surface->h - rect.height)
            {
                setError(error, "Custom character canvas has invalid atlas bounds: " + atlasName);
                return false;
            }

            item.pixels.resize(pixelCount);
            for (int y = 0; y < FRAME_HEIGHT; ++y)
            {
                for (int x = 0; x < expectedWidth; ++x)
                {
                    Uint8 r = 0, g = 0, b = 0, a = 0;
                    if (!SDL_ReadSurfacePixel(surface.get(), rect.x + x, rect.y + y, &r, &g, &b, &a))
                    {
                        setError(error, "Could not read a custom character atlas pixel: " + std::string(SDL_GetError()));
                        return false;
                    }
                    item.pixels[static_cast<std::size_t>(y * expectedWidth + x)] = pack(r, g, b, a);
                }
            }
            character.canvases.push_back(std::move(item));
        }

        if (metadataVersion == 2)
            migrateVersion2(character);

        std::string validationError;
        if (!validateAndNormalize(character, validationError))
        {
            setError(error, "Invalid custom character '" + character.id + "': " + validationError);
            return false;
        }
        loaded.push_back(std::move(character));
    }

    if (metadataVersion == 2 && !loaded.empty())
    {
        if (!persist(loaded, error))
            return false;
    }
    characters_ = std::move(loaded);
    return true;
}

CustomCharacter CharacterStore::makeCharacter(const std::string& requestedName) const
{
    CustomCharacter character;
    character.name = sanitizeDisplayText(requestedName);
    if (character.name.empty())
        character.name = "New Character";
    truncateUtf8Bytes(character.name, MAX_NAME_LENGTH);
    character.id = uniqueId(character.name, characters_);
    character.palette = {
        0x00000000, 0x211A2BFF, 0x633F2AFF, 0xF0B58AFF,
        0xD94C63FF, 0xF56C79FF, 0x45304FFF, 0xB76A2AFF,
        0xE2A24AFF, 0xF7E7B6FF,
    };

    Canvas bow = makeStarterCanvas("bow", "Bow", POSE_FRAME_COUNT);
    Canvas body = makeStarterCanvas("body", "Body", POSE_FRAME_COUNT);
    Canvas head = makeStarterCanvas("head", "Head", POSE_FRAME_COUNT);
    drawStarterAnimations(bow, body, head);

    character.canvases = {std::move(bow), std::move(body), std::move(head)};
    return character;
}

bool CharacterStore::upsert(CustomCharacter character, std::string* error)
{
    if (error)
        error->clear();

    std::string validationError;
    if (!validateAndNormalize(character, validationError))
    {
        setError(error, validationError);
        return false;
    }

    std::vector<CustomCharacter> updated = characters_;
    const auto existing = std::find_if(updated.begin(), updated.end(), [&character](const CustomCharacter& item) {
        return item.id == character.id;
    });
    if (existing != updated.end())
        *existing = std::move(character);
    else
    {
        if (updated.size() >= MAX_CHARACTERS)
        {
            setError(error, "The custom character limit of 256 has been reached");
            return false;
        }
        updated.push_back(std::move(character));
    }

    if (!persist(updated, error))
        return false;
    characters_ = std::move(updated);
    return true;
}

bool CharacterStore::erase(std::string_view id, std::string* error)
{
    if (error)
        error->clear();

    std::vector<CustomCharacter> updated = characters_;
    const auto oldSize = updated.size();
    updated.erase(std::remove_if(updated.begin(), updated.end(), [id](const CustomCharacter& item) {
        return item.id == id;
    }), updated.end());
    if (updated.size() == oldSize)
    {
        setError(error, "Custom character not found: " + std::string(id));
        return false;
    }

    if (!persist(updated, error))
        return false;
    characters_ = std::move(updated);
    return true;
}

const CustomCharacter* CharacterStore::find(std::string_view id) const
{
    const auto found = std::find_if(characters_.begin(), characters_.end(), [id](const CustomCharacter& item) {
        return item.id == id;
    });
    return found == characters_.end() ? nullptr : &*found;
}

Canvas* CharacterStore::canvas(CustomCharacter& character, std::string_view id)
{
    const auto found = std::find_if(character.canvases.begin(), character.canvases.end(), [id](const Canvas& item) {
        return item.id == id;
    });
    return found == character.canvases.end() ? nullptr : &*found;
}

const Canvas* CharacterStore::canvas(const CustomCharacter& character, std::string_view id)
{
    const auto found = std::find_if(character.canvases.begin(), character.canvases.end(), [id](const Canvas& item) {
        return item.id == id;
    });
    return found == character.canvases.end() ? nullptr : &*found;
}

bool CharacterStore::persist(const std::vector<CustomCharacter>& characters, std::string* error) const
{
    std::error_code directoryError;
    std::filesystem::create_directories(rootDirectory(), directoryError);
    if (directoryError)
    {
        setError(error, "Could not create custom character directory: " + directoryError.message());
        return false;
    }

    int atlasWidth = 1;
    int atlasHeight = 0;
    for (const CustomCharacter& character : characters)
    {
        for (const Canvas& item : character.canvases)
        {
            atlasWidth = std::max(atlasWidth, FRAME_WIDTH * item.frameCount);
            if (atlasHeight > std::numeric_limits<int>::max() - FRAME_HEIGHT)
            {
                setError(error, "Custom character atlas is too tall");
                return false;
            }
            atlasHeight += FRAME_HEIGHT;
        }
    }
    atlasHeight = std::max(1, atlasHeight);

    SurfacePtr surface(SDL_CreateSurface(atlasWidth, atlasHeight, SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
    if (!surface)
    {
        setError(error, "Could not create the custom character atlas: " + std::string(SDL_GetError()));
        return false;
    }
    if (!SDL_ClearSurface(surface.get(), 0.0f, 0.0f, 0.0f, 0.0f))
    {
        setError(error, "Could not clear the custom character atlas: " + std::string(SDL_GetError()));
        return false;
    }

    pugi::xml_document metadata;
    pugi::xml_node metadataRoot = metadata.append_child("CustomCharacters");
    metadataRoot.append_attribute("version") = 3;

    pugi::xml_document atlasXml;
    pugi::xml_node atlasRoot = atlasXml.append_child("TextureAtlas");
    atlasRoot.append_attribute("imagePath") = ATLAS_FILENAME;

    int atlasY = 0;
    for (const CustomCharacter& character : characters)
    {
        pugi::xml_node characterNode = metadataRoot.append_child("Character");
        characterNode.append_attribute("id") = character.id.c_str();
        characterNode.append_attribute("name") = character.name.c_str();

        pugi::xml_node paletteNode = characterNode.append_child("Palette");
        for (const std::uint32_t color : character.palette)
            paletteNode.append_child("Color").append_attribute("value") = hexColor(color).c_str();

        for (const Canvas& item : character.canvases)
        {
            pugi::xml_node canvasNode = characterNode.append_child("Canvas");
            canvasNode.append_attribute("id") = item.id.c_str();
            canvasNode.append_attribute("label") = item.label.c_str();
            canvasNode.append_attribute("frameCount") = item.frameCount;

            const int stripWidth = FRAME_WIDTH * item.frameCount;
            pugi::xml_node atlasNode = atlasRoot.append_child("SubTexture");
            const std::string atlasName = "custom/" + character.id + "/" + item.id;
            atlasNode.append_attribute("name") = atlasName.c_str();
            atlasNode.append_attribute("x") = 0;
            atlasNode.append_attribute("y") = atlasY;
            atlasNode.append_attribute("width") = stripWidth;
            atlasNode.append_attribute("height") = FRAME_HEIGHT;

            for (int y = 0; y < FRAME_HEIGHT; ++y)
            {
                for (int x = 0; x < stripWidth; ++x)
                {
                    const std::uint32_t pixel = item.pixels[static_cast<std::size_t>(y * stripWidth + x)];
                    if (!SDL_WriteSurfacePixel(surface.get(), x, atlasY + y,
                        red(pixel), green(pixel), blue(pixel), alpha(pixel)))
                    {
                        setError(error, "Could not write a custom character atlas pixel: " + std::string(SDL_GetError()));
                        return false;
                    }
                }
            }
            atlasY += FRAME_HEIGHT;
        }
    }

    const auto root = rootDirectory();
    const std::array<std::filesystem::path, 3> temporary = {
        root / (std::string(ATLAS_FILENAME) + ".tmp"),
        root / (std::string(ATLAS_XML_FILENAME) + ".tmp"),
        root / (std::string(METADATA_FILENAME) + ".tmp"),
    };
    cleanupTemporaryFiles(temporary);

    if (!SDL_SaveBMP(surface.get(), temporary[0].string().c_str()))
    {
        setError(error, "Could not save the custom character atlas: " + std::string(SDL_GetError()));
        cleanupTemporaryFiles(temporary);
        return false;
    }
    if (!atlasXml.save_file(temporary[1].string().c_str(), "  ", pugi::format_default, pugi::encoding_utf8))
    {
        setError(error, "Could not save custom character atlas XML");
        cleanupTemporaryFiles(temporary);
        return false;
    }
    if (!metadata.save_file(temporary[2].string().c_str(), "  ", pugi::format_default, pugi::encoding_utf8))
    {
        setError(error, "Could not save custom character metadata");
        cleanupTemporaryFiles(temporary);
        return false;
    }

    std::string installError;
    if (!replaceFile(temporary[0], root / ATLAS_FILENAME, installError)
        || !replaceFile(temporary[1], root / ATLAS_XML_FILENAME, installError)
        || !replaceFile(temporary[2], root / METADATA_FILENAME, installError))
    {
        setError(error, installError);
        cleanupTemporaryFiles(temporary);
        return false;
    }
    return true;
}

} // namespace charactereditor
