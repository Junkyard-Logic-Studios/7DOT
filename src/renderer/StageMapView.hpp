#pragma once
#include <SDL3/SDL_rect.h>
#include <algorithm>



namespace renderer
{

    struct StageMapView
    {
        static constexpr float NATIVE_SIZE = 480.0f;

        float scale = 0.0f;
        SDL_FPoint origin {0.0f, 0.0f};
        SDL_FRect mapRect {0.0f, 0.0f, 0.0f, 0.0f};

        SDL_FPoint worldToScreen(SDL_FPoint point) const
        {
            return {
                origin.x + point.x * scale,
                origin.y + point.y * scale
            };
        }

        SDL_FRect worldRectToScreen(float x, float y, float w, float h) const
        {
            return {
                origin.x + x * scale,
                origin.y + y * scale,
                w * scale,
                h * scale
            };
        }
    };


    inline StageMapView computeStageMapView(const SDL_FRect& viewport, SDL_FPoint cameraCenter)
    {
        StageMapView view;
        if (viewport.w <= 0.0f || viewport.h <= 0.0f)
            return view;

        view.scale = std::max(viewport.w / StageMapView::NATIVE_SIZE,
            viewport.h / StageMapView::NATIVE_SIZE);

        float mapSize = StageMapView::NATIVE_SIZE * view.scale;
        view.origin = {
            viewport.x + viewport.w * 0.5f - cameraCenter.x * view.scale,
            viewport.y + viewport.h * 0.5f - cameraCenter.y * view.scale
        };

        view.origin.x = std::clamp(view.origin.x, viewport.x + viewport.w - mapSize, viewport.x);
        view.origin.y = std::clamp(view.origin.y, viewport.y + viewport.h - mapSize, viewport.y);
        view.mapRect = {view.origin.x, view.origin.y, mapSize, mapSize};

        return view;
    }

};  // end namespace renderer
