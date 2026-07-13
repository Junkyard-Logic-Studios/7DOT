#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <array>
#include <vector>

namespace charactereditor
{

inline constexpr int FRAME_WIDTH = 12;
inline constexpr int FRAME_HEIGHT = 20;
struct AnimationSpec
{
    std::string_view id;
    std::string_view label;
    int firstFrame;
    int frameCount;
    int delayMs;
    bool loop;
};

inline constexpr std::array<AnimationSpec, 25> ANIMATIONS = {{
    {"stand", "Stand / Idle", 0, 1, 120, true},
    {"run", "Run Forward", 1, 6, 60, true},
    {"jump", "Jump", 7, 3, 60, false},
    {"fall", "Fall", 10, 4, 60, true},
    {"glide", "Wall Glide", 14, 4, 60, true},
    {"ledge", "Ledge Cling", 18, 4, 100, true},
    {"dodge", "Dodge / Dash", 22, 3, 100, false},
    {"duck", "Duck", 25, 5, 80, false},
    {"slide", "Dodge Slide", 30, 8, 50, true},
    {"stand_aim", "Aim - Stand", 38, 1, 120, true},
    {"run_aim", "Aim - Run", 39, 6, 80, true},
    {"jump_aim", "Aim - Jump", 45, 3, 150, false},
    {"fall_aim", "Aim - Fall", 48, 4, 80, true},
    {"glide_aim", "Aim - Glide", 52, 4, 80, true},
    {"look_up", "Look Up", 56, 1, 120, true},
    {"look_down", "Look Down", 57, 1, 120, true},
    {"look_back", "Look Back", 58, 1, 120, true},
    {"fall_look_up", "Fall - Look Up", 59, 3, 80, true},
    {"fall_look_down", "Fall - Look Down", 62, 3, 80, true},
    {"fall_look_back", "Fall - Look Back", 65, 3, 80, true},
    {"jump_look_up", "Jump - Look Up", 68, 3, 120, true},
    {"jump_look_down", "Jump - Look Down", 71, 3, 120, true},
    {"jump_look_back", "Jump - Look Back", 74, 3, 120, true},
    {"bow_draw", "Bow Draw", 77, 48, 40, false},
    {"bow_empty", "Bow Empty", 125, 4, 120, true},
}};

inline constexpr int POSE_FRAME_COUNT = 129;
static_assert(ANIMATIONS.back().firstFrame + ANIMATIONS.back().frameCount
    == POSE_FRAME_COUNT);
inline constexpr int IDLE_FRAME = 0;
inline constexpr int WALK_FIRST_FRAME = 1;
inline constexpr int WALK_FRAME_COUNT = 6;
inline constexpr int JUMP_FRAME = 7;
inline constexpr int FALL_FRAME = 10;

inline constexpr const AnimationSpec* animationSpec(std::string_view id)
{
    for (const AnimationSpec& animation : ANIMATIONS)
        if (animation.id == id)
            return &animation;
    return nullptr;
}

struct Canvas
{
    std::string id;
    std::string label;
    int frameCount = 1;

    // Row-major pixels for a FRAME_WIDTH * frameCount by FRAME_HEIGHT strip.
    // Body, head and bow use the same aligned 12x20 coordinate space, so the
    // three atlas frames can be drawn directly on top of one another.
    // Each packed pixel is 0xRRGGBBAA (red in the most significant byte).
    std::vector<std::uint32_t> pixels;
};

struct CustomCharacter
{
    std::string id;
    std::string name;
    std::vector<Canvas> canvases;

    // Editor colors in the same 0xRRGGBBAA representation used by Canvas.
    std::vector<std::uint32_t> palette;
};

class CharacterStore
{
public:
    CharacterStore();

    bool reload(std::string* error = nullptr);
    const std::vector<CustomCharacter>& characters() const { return characters_; }

    CustomCharacter makeCharacter(const std::string& name) const;
    bool upsert(CustomCharacter character, std::string* error = nullptr);
    bool erase(std::string_view id, std::string* error = nullptr);
    const CustomCharacter* find(std::string_view id) const;

    static Canvas* canvas(CustomCharacter& character, std::string_view id);
    static const Canvas* canvas(const CustomCharacter& character, std::string_view id);

    static std::filesystem::path rootDirectory();

private:
    bool persist(const std::vector<CustomCharacter>& characters, std::string* error) const;

    std::vector<CustomCharacter> characters_;
};

} // namespace charactereditor
