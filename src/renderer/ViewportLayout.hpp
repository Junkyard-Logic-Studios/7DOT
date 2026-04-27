#pragma once
#include <SDL3/SDL_rect.h>
#include <algorithm>
#include <array>



namespace renderer
{

    struct ViewportLayout
    {
        float scale = 0.0f;
        SDL_FRect mapRect {0.0f, 0.0f, 0.0f, 0.0f};
        std::array<SDL_FRect, 4> deadSpace {{
            {0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 0.0f},
        }};
    };


    inline ViewportLayout computeViewportLayout(
        int windowWidth,
        int windowHeight,
        int worldWidth,
        int worldHeight)
    {
        ViewportLayout layout;
        if (windowWidth <= 0 || windowHeight <= 0 || worldWidth <= 0 || worldHeight <= 0)
            return layout;

        float scaleX = static_cast<float>(windowWidth) / static_cast<float>(worldWidth);
        float scaleY = static_cast<float>(windowHeight) / static_cast<float>(worldHeight);
        layout.scale = std::min(scaleX, scaleY);

        layout.mapRect.w = static_cast<float>(worldWidth) * layout.scale;
        layout.mapRect.h = static_cast<float>(worldHeight) * layout.scale;
        layout.mapRect.x = (static_cast<float>(windowWidth) - layout.mapRect.w) * 0.5f;
        layout.mapRect.y = (static_cast<float>(windowHeight) - layout.mapRect.h) * 0.5f;

        float left = std::max(0.0f, layout.mapRect.x);
        float top = std::max(0.0f, layout.mapRect.y);
        float right = std::max(0.0f, static_cast<float>(windowWidth) - (layout.mapRect.x + layout.mapRect.w));
        float bottom = std::max(0.0f, static_cast<float>(windowHeight) - (layout.mapRect.y + layout.mapRect.h));

        layout.deadSpace = {{
            {0.0f, 0.0f, left, static_cast<float>(windowHeight)},
            {layout.mapRect.x + layout.mapRect.w, 0.0f, right, static_cast<float>(windowHeight)},
            {layout.mapRect.x, 0.0f, layout.mapRect.w, top},
            {layout.mapRect.x, layout.mapRect.y + layout.mapRect.h, layout.mapRect.w, bottom},
        }};

        return layout;
    }

};  // end namespace renderer
