#pragma once
#include "../fight/stage.hpp"
#include <SDL3/SDL_rect.h>
#include <string>
#include <vector>



namespace renderer
{

    struct StageMapEntry
    {
        fight::Stage stage;
        std::string themeId;
        std::string iconAtlasName;
        SDL_FPoint mapPosition;
    };


    class StageMapCatalog
    {
    public:
        void load(const std::string& xmlPath);
        const StageMapEntry* find(fight::Stage stage) const;
        const std::vector<StageMapEntry>& entries() const;

    private:
        std::vector<StageMapEntry> _entries;
    };

};  // end namespace renderer
